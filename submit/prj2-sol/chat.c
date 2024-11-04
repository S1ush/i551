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

#include "common.h"
#include <chat-cmd.h>
#include <chat-db.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

// TODO: add code for functions used by both client and server

int serialize_add_cmd(const AddCmd *add, StrSpace *strSpace) {
    init_str_space(strSpace);  // Initialize StrSpace for building the serialized string

    // Format the serialized string with all fields of AddCmd
    if (append_sprintf_str_space(strSpace, 
            "user=%s;room=%s;message=%s;nTopics=%zu;", 
            add->user, add->room, add->message, add->nTopics) != 0) {
        return -1;  // Serialization error
    }

    // Add each topic to the serialized string
    for (size_t i = 0; i < add->nTopics; ++i) {
        if (append_sprintf_str_space(strSpace, "topic[%zu]=%s;", i, add->topics[i]) != 0) {
            return -1;  // Serialization error
        }
    }
    return 0;  // Success
}

int deserialize_add_cmd(const char *input, AddCmd *addCmd) {
    memset(addCmd, 0, sizeof(AddCmd));  // Initialize AddCmd with zeroes
    
    char user[128], room[128];
    size_t nTopics;
    
    // Parse user and room first
    if (sscanf(input, "user=%127[^;];room=%127[^;];", user, room) != 2) {
        return 1;  // Parsing error
    }
    
    // Find message field
    const char *msg_start = strstr(input, "message=");
    if (!msg_start) return -1;
    msg_start += 8;  // Skip "message="
    
    // Find end of message (next semicolon)
    const char *msg_end = strchr(msg_start, ';');
    if (!msg_end) return 2;
    
    // Calculate message length and allocate buffer
    size_t msg_len = msg_end - msg_start;
    char *message = malloc(msg_len + 1);  // +1 for null terminator
    if (!message) return 3;
    
    // Copy message content
    strncpy(message, msg_start, msg_len);
    message[msg_len] = '\0';  // Ensure null termination
    
    // Parse nTopics
    const char *topics_count = strstr(msg_end, "nTopics=");
    if (!topics_count || sscanf(topics_count, "nTopics=%zu;", &nTopics) != 1) {
        free(message);
        return 4;
    }
    
    // Allocate memory for topics
    const char **topics = malloc(nTopics * sizeof(char *));
    if (!topics) {
        free(message);
        return 5;
    }
    
    // Initialize topics array to NULL for safer cleanup on error
    for (size_t i = 0; i < nTopics; i++) {
        topics[i] = NULL;
    }
    
    // Extract topics from the input string
    const char *pos = strstr(input, "topic[0]=");
    for (size_t i = 0; i < nTopics && pos; ++i) {
        char topic[128];
        if (sscanf(pos, "topic[%zu]=%127[^;];", &i, topic) != 2) {
            // Cleanup on error
            for (size_t j = 0; j < i; j++) {
                free((void*)topics[j]);
            }
            free(topics);
            free(message);
            return 6;
        }
        topics[i] = strdup(topic);
        if (!topics[i]) {
            // Cleanup on allocation failure
            for (size_t j = 0; j < i; j++) {
                free((void*)topics[j]);
            }
            free(topics);
            free(message);
            return 7;
        }
        pos = strstr(pos + 1, "topic[");
    }
    
    // Populate the AddCmd struct
    addCmd->user = strdup(user);
    addCmd->room = strdup(room);
    addCmd->message = message;  // Transfer ownership of dynamically allocated message
    addCmd->nTopics = nTopics;
    addCmd->topics = topics;
    
    // Check if any strdup failed
    if (!addCmd->user || !addCmd->room) {
        // Cleanup everything if any allocation failed
        free((void*)addCmd->user);
        free((void*)addCmd->room);
        free(addCmd->message);
        for (size_t i = 0; i < nTopics; i++) {
            free((void*)topics[i]);
        }
        free(topics);
        return 8;
    }
    
    return 0;  // Success
}


int serialize_query_cmd(const QueryCmd *query, StrSpace *strSpace) {
    init_str_space(strSpace);  // Initialize StrSpace for building the serialized string

    // Format the serialized string with all fields of QueryCmd
    if (append_sprintf_str_space(strSpace, 
            "room=%s;count=%zu;nTopics=%zu;", 
            query->room, query->count, query->nTopics) != 0) {
        return -1;  // Serialization error
    }

    // Add each topic to the serialized string
    for (size_t i = 0; i < query->nTopics; ++i) {
        if (append_sprintf_str_space(strSpace, "topic[%zu]=%s;", i, query->topics[i]) != 0) {
            return -1;  // Serialization error
        }
    }
    return 0;  // Success
}


int deserialize_query_cmd(const char *input, QueryCmd *queryCmd) {
    memset(queryCmd, 0, sizeof(QueryCmd));  // Initialize QueryCmd with zeroes

    // Parse the serialized input to extract fields
    char room[128];
    size_t count, nTopics;

    if (sscanf(input, "room=%127[^;];count=%zu;nTopics=%zu;", room, &count, &nTopics) != 3) {
        return -1;  // Parsing error
    }

    // Allocate memory for topics
    const char **topics = malloc(nTopics * sizeof(char *));
    if (!topics) return -1;  // Memory allocation failure

    // Extract topics from the input string
    const char *pos = strstr(input, "topic[0]=");
    for (size_t i = 0; i < nTopics && pos; ++i) {
        char topic[128];
        if (sscanf(pos, "topic[%zu]=%127[^;];", &i, topic) != 2) {
            free(topics);  // Free memory on error
            return -1;  // Parsing error
        }
        topics[i] = strdup(topic);  // Store the topic
        pos = strstr(pos + 1, "topic[");  // Move to the next topic
    }

    // Populate the QueryCmd struct
    queryCmd->room = strdup(room);
    queryCmd->count = count;
    queryCmd->nTopics = nTopics;
    queryCmd->topics = topics;

    return 0;  // Success
}



int deserialize_chat_info(const char *input, ChatInfo *chatInfo) {
    if (!input || !chatInfo) return -1;  // Error check
    memset(chatInfo, 0, sizeof(ChatInfo));  // Initialize structure
    
    char user[128], room[128], timestamp_str[32];
    TimeMillis timestamp;
    size_t nTopics;
    
    // First, find the message boundaries
    const char *msg_start = strstr(input, "message=");
    if (!msg_start) return -1;
    msg_start += 8;  // Length of "message="
    
    const char *msg_end = strstr(msg_start, ";timestamp=");
    if (!msg_end) return -1;
    
    // Calculate message length and allocate buffer
    size_t msg_len = msg_end - msg_start;
    char *message = malloc(msg_len + 1);  // +1 for null terminator
    if (!message) return -1;
    
    // Extract message
    strncpy(message, msg_start, msg_len);
    message[msg_len] = '\0';
    
    // Parse other fields using the message end as a reference point
    char format[256];
    snprintf(format, sizeof(format), 
             "user=%%127[^;];room=%%127[^;];message=%%%zu[^;];timestamp=%%31[^;];nTopics=%%zu;",
             msg_len);
    
    if (sscanf(input, format, user, room, message, timestamp_str, &nTopics) != 5) {
        free(message);
        return -1;  // Parsing error
    }
    
    // Allocate memory for topics
    const char **topics = malloc(nTopics * sizeof(char *));
    if (!topics) {
        free(message);
        return -1;  // Memory allocation error
    }
    
    // Parse each topic
    const char *pos = strstr(input, "topic[0]=");  // Start of topics
    for (size_t i = 0; i < nTopics && pos; ++i) {
        char topic[128];
        if (sscanf(pos, "topic[%zu]=%127[^;];", &i, topic) != 2) {
            // Cleanup on failure
            free(message);
            for (size_t j = 0; j < i; j++) {
                free((void*)topics[j]);
            }
            free(topics);
            return -1;
        }
        topics[i] = strdup(topic);  // Store topic
        pos = strstr(pos + 1, "topic[");  // Move to the next topic
    }
    
    // Populate ChatInfo structure
    chatInfo->user = strdup(user);
    chatInfo->room = strdup(room);
    chatInfo->message = message;  // Transfer ownership of dynamically allocated message
    chatInfo->timestamp = timestamp_str;
    chatInfo->nTopics = nTopics;
    chatInfo->topics = topics;
    
    return 0;  // Success
}



void free_query_cmd(QueryCmd *cmd) {
    if (cmd->topics) {
        for (size_t i = 0; i < cmd->nTopics; ++i) {
            free((char *)cmd->topics[i]);  // Free each topic string
        }
        free(cmd->topics);  // Free the topics array
    }
    free((char *)cmd->room);  // Free the room string
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