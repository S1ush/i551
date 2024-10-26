#include "chat.h"
#include "common.h"
#include "server.h"
#include "str-space.h"

#include <chat-cmd.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ERROR "err "
#define OKAY "ok\n"

struct _Chat {
    int client_to_server[2];  // Pipe for client to server communication
    int server_to_client[2];  // Pipe for server to client communication
    pid_t server_pid;         // PID of the server process
    FILE *out;               // Output stream for success responses
    FILE *err;               // Error stream for error messages
};

Chat *
make_chat(const char *dbPath, FILE *out, FILE *err)
{  
    Chat *chat = malloc(sizeof(Chat));
    if (!chat) {
        fprintf(err, "%serror allocating memory\n", ERROR);
        return NULL;
    }
    
    // Initialize pipes for IPC
    if (pipe(chat->client_to_server) == -1 || pipe(chat->server_to_client) == -1) {
        fprintf(err, "%spipe creation failed\n", ERROR);
        free(chat);
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed
        fprintf(err, "%sfork failed\n", ERROR);
        close(chat->client_to_server[1]);
        close(chat->client_to_server[0]);
        close(chat->server_to_client[0]);
        close(chat->server_to_client[1]);
        free(chat);
        return NULL;
    } else if (pid == 0) {
        // In the child (server) process
        close(chat->client_to_server[1]);  // Close write end of client to server pipe
        close(chat->server_to_client[0]);  // Close read end of server to client pipe
        
        do_server(chat->client_to_server[0], chat->server_to_client[1], dbPath);
        exit(0);
    }

    chat->server_pid = pid;
    close(chat->client_to_server[0]);  // Close read end of client to server pipe
    close(chat->server_to_client[1]);  // Close write end of server to client pipe
    chat->out = out;
    chat->err = err;
    return chat;  
}

void
free_chat(Chat *chat)
{
    if (!chat) return;

    // Send END_CMD to server
    CmdType end_cmd = END_CMD;
    write(chat->client_to_server[1], &end_cmd, sizeof(CmdType));

    // Close remaining pipe ends
    close(chat->client_to_server[1]);
    close(chat->server_to_client[0]);

    // Wait for server process to terminate
    int status;
    waitpid(chat->server_pid, &status, 0);

    // Free the chat structure itself
    free(chat);
}

void receive_and_print_chat_info(int server_fd, Chat *chat) {
    char buffer[BUFSIZ*2];
    int res = 0;
    int temp = 0;
    
    while ((res = read(server_fd, buffer, sizeof(buffer) - 1)) > 0) {
        ChatInfo chatInfo;
        
        if (strncmp(buffer, "end", 3) == 0) {
            break;
        }
        
        if (temp == 0) {
            temp++;
        }
        
        if (strncmp(buffer, "searched", 8) == 0) {
            fprintf(chat->out, "ok\n");
            memset(buffer, 0, sizeof(buffer));
            continue;
        }
        
        if (strncmp(buffer, "room", 4) == 0) {
            fprintf(chat->out, "err BAD_ROOM\n");
            memset(buffer, 0, sizeof(buffer));
            break;
        }
        
        if (strncmp(buffer, "topic", 5) == 0) {
            fprintf(chat->out, "err BAD_TOPIC\n");
            memset(buffer, 0, sizeof(buffer));
            break;
        }
        
        buffer[sizeof(buffer) - 1] = '\0';
        
        if (deserialize_chat_info(buffer, &chatInfo) != 0) {
            fprintf(chat->out, "Error: Failed to deserialize chat info %d\n", res);
        }
        
        memset(buffer, 0, sizeof(buffer));

        fprintf(chat->out, "%s\n%s %s", chatInfo.timestamp, chatInfo.user, chatInfo.room);

        if (chatInfo.nTopics > 0) {
            for (size_t i = 0; i < chatInfo.nTopics; ++i) {
                fprintf(chat->out, " %s", chatInfo.topics[i]);
            }
            fprintf(chat->out, "\n");
        }
        fprintf(chat->out, "%s", chatInfo.message);
        
        memset(buffer, 0, sizeof(buffer));
    }
}

void
do_chat_cmd(Chat *chat, const ChatCmd *cmd)
{
    StrSpace strSpace;
    init_str_space(&strSpace);

    if (write(chat->client_to_server[1], &cmd->type, sizeof(CmdType)) <= 0) {
        fprintf(chat->err, ERROR "SYS_ERR: Failed to send command to server : type\n");
        free_str_space(&strSpace);
        return;
    }

    switch (cmd->type) {
        case ADD_CMD: {
            if (serialize_add_cmd(&cmd->add, &strSpace) != 0) {
                fprintf(chat->err, "ERROR: Failed to serialize AddCmd.\n");
                free_str_space(&strSpace);
                return;
            }
            
            const char *serialized_add = iter_str_space(&strSpace, NULL);
            if (write(chat->client_to_server[1], serialized_add, strlen(serialized_add)) <= 0) {
                fprintf(chat->err, ERROR "SYS_ERR: Failed to send command to server : add_data\n");
                free_str_space(&strSpace);
                return;
            }

            char response[1024];
            ssize_t n = read(chat->server_to_client[0], response, sizeof(response) - 1);
            if (n < 0) {
                fprintf(chat->err, ERROR "SYS_ERR: Failed to read server response\n");
                free_str_space(&strSpace);
                return;
            }

            response[n] = '\0';
            fprintf(chat->out, "%s\n", response);
            break;
        }
        
        case QUERY_CMD: {
            if (serialize_query_cmd(&cmd->query, &strSpace) != 0) {
                fprintf(chat->err,"Serialized QueryCmd: %s\n", strSpace.buf);
            }

            const char *serialized_query = iter_str_space(&strSpace, NULL);
            if (write(chat->client_to_server[1], serialized_query, strlen(serialized_query)) <= 0) {
                fprintf(chat->err, ERROR "SYS_ERR: Failed to send command to server : add_data\n");
                free_str_space(&strSpace);
                return;
            }
            receive_and_print_chat_info(chat->server_to_client[0], chat);
            break;
        }
        
        case END_CMD: {
            // Send END_CMD to server and cleanup will happen in free_chat
            break;
        }
    }

    free_str_space(&strSpace);
}

pid_t
chat_server_pid(const Chat *chat)
{
    return chat->server_pid;
}