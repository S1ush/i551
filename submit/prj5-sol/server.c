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

static Client clients[MAX_CLIENTS];

// Initialize client array
void initialize_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }
}

void send_room_history(int clientFd, const char *room) {
    query_chat_db(chatDb, room, 0, NULL, 0, iterFn, &clientFd);
}

// Add a new client
int add_client(int clientFd, const char *user, const char *room) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = clientFd;
            strncpy(clients[i].user, user, sizeof(clients[i].user) - 1);
            strncpy(clients[i].room, room, sizeof(clients[i].room) - 1);

            // Notify others in the room
            char notification[BUFFER_SIZE];
            snprintf(notification, sizeof(notification), "%s has joined the room.\n", user);
            broadcast_message(room, notification, -1);

            // Send room history to the new user
            send_room_history(clientFd, room);
            return i;
        }
    }
    return -1; // No space for new client
}


int iterFn(const ChatInfo *result, void *ctx) {
    int clientFd = *(int *)ctx;
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Message from %s: %s\n", result->user, result->message);
    write_message(clientFd, buffer);
    return 0; // Continue processing
}

int get_client_index(int clientFd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == clientFd) {
            return i;
        }
    }
    return -1; // Not found
}

// Remove a client
void remove_client(int clientFd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == clientFd) {
            // Notify others in the room
            char notification[BUFFER_SIZE];
            snprintf(notification, sizeof(notification), "%s has left the room.\n", clients[i].user);
            broadcast_message(clients[i].room, notification, -1);

            // Clean up client data
            close(clientFd);
            clients[i].fd = -1;
            clients[i].user[0] = '\0';
            clients[i].room[0] = '\0';
            break;
        }
    }
}

// Broadcast a message to all clients in the same room
void broadcast_message(const char *room, const char *message, int senderFd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && strcmp(clients[i].room, room) == 0) {
            write_message(clients[i].fd, message);
        }
    }

    // Send "okay" to the sender
    if (senderFd != -1) {
        write_message(senderFd, "okay\n");
    }
}


void sanitize_input(char *buffer) {
    char *end = buffer + strlen(buffer) - 1;

    // Remove trailing newlines and whitespace
    while (end > buffer && (*end == '\n' || *end == '\r' || *end == ' ')) {
        *end = '\0';
        end--;
    }

    // Handle blank inputs or unexpected dots
    if (strcmp(buffer, ".") == 0 || strlen(buffer) == 0) {
        buffer[0] = '\0'; // Ignore input
    }
}


// Handle client commands
// void handle_client_command(int clientFd, const char *buffer, ChatDb *chatDb) {
//     int clientIndex = get_client_index(clientFd);
//     if (clientIndex < 0) {
//         fprintf(stderr, "Client not found\n");
//         return;
//     }

//     MsgArgs msgArgs;
//     ChatCmd cmd; // Declare cmd

//     if (parse_loggedin_cmd(&msgArgs, clients[clientIndex].user,
//                            clients[clientIndex].room, &cmd, stderr) != 0) {
//         send_structured_message(clientFd, SYS_ERR, "", "", "Invalid command");
//         return;
//     }

//     switch (cmd.type) {
//     case ADD_CMD:
//         add_chat_db(chatDb, cmd.add.user, cmd.add.room, cmd.add.nTopics,
//                     cmd.add.topics, cmd.add.message);
//         broadcast_message(cmd.add.room, cmd.add.message);
//         break;

//     case QUERY_CMD:
//         query_chat_db(chatDb, cmd.query.room, cmd.query.nTopics,
//                       cmd.query.topics, cmd.query.count, iterFn, &clientFd);
//         break;

//     case END_CMD:
//         remove_client(clientFd);
//         break;

//     default:
//         send_structured_message(clientFd, SYS_ERR, "", "", "Unknown command");
//         break;
//     }
// }
void handle_client_command(int clientFd, const char *buffer, ChatDb *chatDb) {
    int clientIndex = get_client_index(clientFd);
    if (clientIndex < 0) {
        fprintf(stderr, "Client not found\n");
        return;
    }

    MsgArgs msgArgs;
    ChatCmd cmd;
    memset(&msgArgs, 0, sizeof(MsgArgs));

    if (parse_loggedin_cmd(&msgArgs, clients[clientIndex].user,
                           clients[clientIndex].room, &cmd, stderr) != 0) {
        send_structured_message(clientFd, SYS_ERR, "", "", "Invalid command");
        return;
    }

    switch (cmd.type) {
    case ADD_CMD: {
        // Add message to the database
        add_chat_db(chatDb, cmd.add.user, cmd.add.room, cmd.add.nTopics,
                    cmd.add.topics, cmd.add.message);

        // Broadcast message to others in the room
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "Message from %s: %s\n",
                 cmd.add.user, cmd.add.message);
        broadcast_message(cmd.add.room, message, clientFd);

        // Send "okay" to the sender
        write_message(clientFd, "okay\n");
        break;
    }
    case QUERY_CMD: {
        // Query messages and send to the client
        query_chat_db(chatDb, cmd.query.room, cmd.query.nTopics,
                      cmd.query.topics, cmd.query.count, iterFn, &clientFd);

        // Send "okay" to the sender
        write_message(clientFd, "okay\n");
        break;
    }
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

    // Array to track invalid command counts per client
    static int invalid_command_count[MAX_CLIENTS] = {0};

    while (true) {
        read_fds = active_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == serverSockFd) {
                    // New client connection
                    int clientFd = accept(serverSockFd, NULL, NULL);
                    if (clientFd < 0) continue;

                    int clientIndex = add_client(clientFd);
                    if (clientIndex < 0) {
                        write_message(clientFd, "Server full\n");
                        close(clientFd);
                        continue;
                    }

                    FD_SET(clientFd, &active_fds);
                    if (clientFd > max_fd) max_fd = clientFd;
                } else {
                    // Handle client data
                    char buffer[BUFFER_SIZE];
                    int bytesRead = read(i, buffer, sizeof(buffer) - 1);
                    if (bytesRead <= 0) {
                        // Client disconnected
                        FD_CLR(i, &active_fds);
                        remove_client(i);
                    } else {
                        buffer[bytesRead] = '\0'; // Null-terminate the input
                        sanitize_input(buffer);   // Sanitize input

                        if (strlen(buffer) == 0) {
                            printf("Ignored empty or invalid input from client %d\n", i);
                            continue;
                        }

                        // Handle the command
                        int clientIndex = get_client_index(i);
                        if (clientIndex < 0) {
                            fprintf(stderr, "Client %d not found\n", i);
                            continue;
                        }

                        MsgArgs msgArgs;
                        ChatCmd cmd;
                        memset(&msgArgs, 0, sizeof(MsgArgs));

                        if (parse_loggedin_cmd(&msgArgs, clients[clientIndex].user,
                                               clients[clientIndex].room, &cmd, stderr) != 0) {
                            // Increment invalid command count
                            invalid_command_count[clientIndex]++;
                            fprintf(stderr, "Invalid command from client %d: %s\n", i, buffer);

                            if (invalid_command_count[clientIndex] > 5) {
                                fprintf(stderr, "Disconnecting client %d due to excessive invalid commands\n", i);
                                FD_CLR(i, &active_fds);
                                remove_client(i);
                                continue;
                            }

                            send_structured_message(i, SYS_ERR, "", "", "Invalid command");
                            continue;
                        }

                        // Reset invalid command count on valid command
                        invalid_command_count[clientIndex] = 0;

                        // Process valid command
                        handle_client_command(i, buffer, dbPath);
                    }
                }
            }
        }
    }
}
