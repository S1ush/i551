#include "server.h"

#include "common.h"

#include <chat-cmd.h>
#include <errors.h>

#include <chat-db.h>


//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>

#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <pthread.h>
#define SYS_ERR 1
#define MAX_CLIENTS 1024
#define BUFFER_SIZE 1024
//TODO: add auxiliary functions
int iterFn(const ChatInfo *result, void *ctx);
int get_client_index(int clientFd);


// Data structure to manage connected clients
typedef struct {
    int fd;
    char user[50];
    char room[50];
} Client;

int get_client_index(int clientFd);
int iterFn(const ChatInfo *result, void *ctx);


static Client clients[MAX_CLIENTS];

void initialize_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }
}

void send_room_history(int clientFd, ChatDb *chatDb, const char *room) {
    int iterFn(const ChatInfo *result, void *ctx) {
        int fd = *(int *)ctx;
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "Message from %s: %s\n", result->user, result->message);
        write_message(fd, message);
        return 0;
    }
    query_chat_db(chatDb, room, 0, NULL, 0, iterFn, &clientFd);
}

int add_client(int clientFd, const char *user, const char *room, ChatDb *chatDb) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = clientFd;
            strncpy(clients[i].user, user, sizeof(clients[i].user) - 1);
            strncpy(clients[i].room, room, sizeof(clients[i].room) - 1);

            // fprintf(stderr, "DEBUG: Added client FD %d with user '%s' to room '%s'\n",
                    // clientFd, user, room);

            char notification[BUFFER_SIZE];
            snprintf(notification, sizeof(notification), "%s has joined the room.\n", user);
            broadcast_message(room, notification, -1);

            send_room_history(clientFd, chatDb, room);
            return i;
        }
    }
    return -1;
}

void remove_client(int clientFd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == clientFd) {
            char notification[BUFFER_SIZE];
            snprintf(notification, sizeof(notification), "%s has left the room.\n", clients[i].user);
            broadcast_message(clients[i].room, notification, -1);

            close(clientFd);
            clients[i].fd = -1;
            clients[i].user[0] = '\0';
            clients[i].room[0] = '\0';
            break;
        }
    }
}

void broadcast_message(const char *room, const char *message, int senderFd) {
    // fprintf(stderr, "DEBUG: Broadcasting message '%s' in room '%s' from sender FD %d\n",
            message, room, senderFd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && strcmp(clients[i].room, room) == 0 && clients[i].fd != senderFd) {
            write_message(clients[i].fd, message);
        }
    }
}

void sanitize_input(char *buffer) {
    char *end = buffer + strlen(buffer) - 1;
    while (end > buffer && (*end == '\n' || *end == '\r' || *end == ' ')) {
        *end = '\0';
        end--;
    }
}

int get_client_index(int clientFd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == clientFd) {
            return i;
        }
    }
    return -1; // Not found
}


int iterFn(const ChatInfo *result, void *ctx) {
    int clientFd = *(int *)ctx;
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Message from %s: %s\n", result->user, result->message);
    write_message(clientFd, buffer);
    return 0; // Continue processing
}

void handle_client_command(int clientFd, const char *buffer, ChatDb *chatDb) {
    int clientIndex = get_client_index(clientFd);
    if (clientIndex < 0) {
        fprintf(stderr, "DEBUG: Client FD %d not found\n", clientFd);
        return;
    }

    MsgArgs msgArgs;
    ChatCmd cmd;
    memset(&msgArgs, 0, sizeof(MsgArgs));

    if (parse_loggedin_cmd(&msgArgs, clients[clientIndex].user,
                           clients[clientIndex].room, &cmd, stderr) != 0) {
        // fprintf(stderr, "DEBUG: Invalid command from client FD %d: %s\n", clientFd, buffer);
        send_structured_message(clientFd, SYS_ERR, "", "", "Invalid command");
        return;
    }

    switch (cmd.type) {
    case ADD_CMD: {
        int rc = add_chat_db(chatDb, cmd.add.user, cmd.add.room, cmd.add.nTopics,
                             cmd.add.topics, cmd.add.message);
        if (rc != 0) {
            fprintf(stderr, "DEBUG: Failed to add message to DB: %s\n", error_chat_db(chatDb));
            send_structured_message(clientFd, SYS_ERR, "", "", "Failed to save message");
        } else {
            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "Message from %s: %s\n", cmd.add.user, cmd.add.message);
            broadcast_message(cmd.add.room, message, clientFd);
        }
        break;
    }
    case QUERY_CMD:
        query_chat_db(chatDb, cmd.query.room, cmd.query.nTopics, cmd.query.topics,
                      cmd.query.count, iterFn, &clientFd);
        break;
    case END_CMD:
        remove_client(clientFd);
        break;
    default:
        send_structured_message(clientFd, SYS_ERR, "", "", "Unknown command");
        break;
    }
}

void do_serve(int serverSockFd, const char *dbPath) {
    fd_set active_fds, read_fds;
    FD_ZERO(&active_fds);
    FD_SET(serverSockFd, &active_fds);

    int max_fd = serverSockFd;
    initialize_clients();

    MakeChatDbResult result;
    if (make_chat_db(dbPath, &result) != 0) {
        fprintf(stderr, "Error initializing chat DB: %s\n", result.err);
        exit(EXIT_FAILURE);
    }
    ChatDb *chatDb = result.chatDb;

    while (true) {
        read_fds = active_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            free_chat_db(chatDb);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == serverSockFd) {
                    int clientFd = accept(serverSockFd, NULL, NULL);
                    if (clientFd < 0) continue;

                    int clientIndex = add_client(clientFd, "default_user", "default_room", chatDb);
                    if (clientIndex < 0) {
                        write_message(clientFd, "Server full\n");
                        close(clientFd);
                        continue;
                    }

                    FD_SET(clientFd, &active_fds);
                    if (clientFd > max_fd) max_fd = clientFd;
                } else {
                    char buffer[BUFFER_SIZE];
                    int bytesRead = read(i, buffer, sizeof(buffer) - 1);
                    if (bytesRead <= 0) {
                        FD_CLR(i, &active_fds);
                        remove_client(i);
                    } else {
                        buffer[bytesRead] = '\0';
                        sanitize_input(buffer);
                        handle_client_command(i, buffer, chatDb);
                    }
                }
            }
        }
    }

    free_chat_db(chatDb);
}