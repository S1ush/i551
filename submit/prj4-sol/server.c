#include "server.h"
#include "common.h"

#include <errors.h>
#include <chat-db.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

//TODO: add server code

// Function to handle each command received by the server
// void handle_chat_cmd(Chat *chatDb, const ChatCmd *cmd) {
//     int result;
//     char response[MAX_RESPONSE_LEN];

//     switch (cmd->type) {
//         case ADD_ROOM_CMD:
//             result = add_room(chatDb, cmd->data.roomName);
//             if (result == 0) {
//                 snprintf(response, sizeof(response), OKAY);
//             } else if (result == ERR_BAD_ROOM) {
//                 snprintf(response, sizeof(response), ERROR "BAD_ROOM: Room name already exists\n");
//             } else {
//                 snprintf(response, sizeof(response), ERROR "SYS_ERR: Error adding room\n");
//             }
//             break;

//         case ADD_TOPIC_CMD:
//             result = add_topic(chatDb, cmd->data.roomName, cmd->data.topicName);
//             if (result == 0) {
//                 snprintf(response, sizeof(response), OKAY);
//             } else if (result == ERR_BAD_ROOM) {
//                 snprintf(response, sizeof(response), ERROR "BAD_ROOM: Room does not exist\n");
//             } else if (result == ERR_BAD_TOPIC) {
//                 snprintf(response, sizeof(response), ERROR "BAD_TOPIC: Topic name invalid\n");
//             } else {
//                 snprintf(response, sizeof(response), ERROR "SYS_ERR: Error adding topic\n");
//             }
//             break;

//         case QUERY_ROOM_CMD:
//             result = query_room(chatDb, cmd->data.roomName, response, sizeof(response));
//             if (result == ERR_BAD_ROOM) {
//                 snprintf(response, sizeof(response), ERROR "BAD_ROOM: Room does not exist\n");
//             } else if (result != 0) {
//                 snprintf(response, sizeof(response), ERROR "SYS_ERR: Error querying room\n");
//             }
//             break;

//         case QUERY_TOPIC_CMD:
//             result = query_topic(chatDb, cmd->data.roomName, cmd->data.topicName, response, sizeof(response));
//             if (result == ERR_BAD_ROOM) {
//                 snprintf(response, sizeof(response), ERROR "BAD_ROOM: Room does not exist\n");
//             } else if (result == ERR_BAD_TOPIC) {
//                 snprintf(response, sizeof(response), ERROR "BAD_TOPIC: Topic does not exist\n");
//             } else if (result != 0) {
//                 snprintf(response, sizeof(response), ERROR "SYS_ERR: Error querying topic\n");
//             }
//             break;

//         case END_CMD:
//             // Handle the end command if needed, usually by setting a flag or breaking out of a loop
//             snprintf(response, sizeof(response), OKAY "Server is shutting down\n");
//             break;

//         default:
//             snprintf(response, sizeof(response), ERROR "SYS_ERR: Unknown command type\n");
//             break;
//     }

//     // Write the response back to shared memory
//     send_data(cmd->shm, true, strlen(response) + 1, response);
// }  // Ensure this function is declared


void
do_server(const char *dbPath, Shm *shm)
{
    // ChatDb *chatDb = NULL;
    // // const char *errMsg = NULL;
    // MakeChatDbResult result;

    // // Open the database
    // if (make_chat_db(dbPath, &result) != 0) {
    //     // errMsg = result.err;
    //     // goto CLEANUP;
    //     fprintf(stderr, "SYS_ERR: %s\n", result.err);
    //     exit(1);
    // }
    // chatDb = result.chatDb;

    // // Server main loop
    // while (1) {
    //     // Wait for a command from the client
    //     sem_wait(&shm->sems[0]);

    //     // Read the command from shared memory
    //     ChatCmd cmd;
    //     memcpy(&cmd, shm->buf, sizeof(ChatCmd));

    //     if (cmd.type == END_CMD) {
    //         break; // Exit the loop on END_CMD
    //     }

    //     // Process the command
    //     switch (cmd.type) {
    //         case ADD_CMD: {
    //             const AddCmd *add = &cmd.add;

    //         // Debug: Print incoming AddCmd details
    //         printf("Server: Received ADD_CMD\n");
    //         printf("Server: User: %s, Room: %s, Message: %s\n", add->user, add->room, add->message);
    //         printf("Server: Topics count: %d\n", add->nTopics);
    //         for (int i = 0; i < add->nTopics; i++) {
    //             printf("Server: Topic[%d]: %s\n", i, add->topics[i]);
    //         }

    //         // Attempt to add the chat data to the database
    //         printf("Server: Adding data to database...\n");
    //         int errCode = add_chat_db(chatDb, add->user, add->room, add->nTopics,
    //                                 (const char **)add->topics, add->message);

    //         // Debug: Print database operation result
    //         if (errCode != 0) {
    //             printf("Server: add_chat_db failed with error: %s\n", error_chat_db(chatDb));
    //             snprintf(shm->buf, shm->bufSize, "err SYS_ERR: %s\n", error_chat_db(chatDb));
    //         } else {
    //             printf("Server: add_chat_db succeeded\n");
    //             snprintf(shm->buf, shm->bufSize, "ok\n");
    //         }

    //         // Debug: Print shared memory response
    //         printf("Server: Response written to shared memory: %s\n", shm->buf);
    //         break;
    //         }
    //         case QUERY_CMD: {
    //             const QueryCmd *query = &cmd.query;
    //             size_t count;
    //             int errCode = count_room_chat_db(chatDb, query->room, &count);

    //             if (errCode != 0 || count == 0) {
    //                 snprintf(shm->buf, shm->bufSize, "err BAD_ROOM: unknown room\n");
    //             } else {
    //                 // Respond with query results using query iterator
    //                 errCode = query_chat_db(chatDb, query->room, query->nTopics,
    //                                         (const char **)query->topics, query->count,
    //                                         query_iterator, shm);
    //                 if (errCode != 0) {
    //                     snprintf(shm->buf, shm->bufSize, "err SYS_ERR: %s\n", error_chat_db(chatDb));
    //                 } else {
    //                     snprintf(shm->buf, shm->bufSize, "ok\n");
    //                 }
    //             }
    //             break;
    //         }
    //         default:
    //             snprintf(shm->buf, shm->bufSize, "err SYS_ERR: unknown command\n");
    //             break;
    //     }

    //     // Signal client that a response is ready
    //     sem_post(&shm->sems[1]);
    // }

    // // CLEANUP:
    // // if (chatDb) {
    // //     free_chat_db(chatDb);
    // // }
    // // exit(0);
    // exit(0);

    //  printf("Server: Starting main loop...\n");
    // ChatDb *chatDb = NULL;
    // MakeChatDbResult result;

    // // Open the database
    // if (make_chat_db(dbPath, &result) != 0) {
    //     fprintf(stderr, "SYS_ERR: %s\n", result.err);
    //     exit(1);
    // }
    // chatDb = result.chatDb;

    // // Server main loop
    // while (1) {
    //     // Wait for a command from the client
    //     printf("Server: Waiting for client semaphore...\n");
    //     sem_wait(&shm->sems[0]);
    //     printf("Server: Detected command from client.\n");
    //     printf("Server: Acquiring mutex semaphore...\n");
    //     sem_wait(&shm->sems[2]); // Acquire mutex semaphore

    //     printf("Server: Raw buffer content (first 16 bytes): ");
    //     for (size_t i = 0; i < 16 && i < shm->bufSize; i++) {
    //         printf("%02x ", (unsigned char)shm->buf[i]);
    //     }
    //     printf("\n");
    //     // Deserialize the command
    //     printf("Server: beShared memory content before deserialization: ");
    //     for (size_t i = 0; i < 16 && i < shm->bufSize; i++) {
    //         printf("%02x ", (unsigned char)shm->buf[i]);
    //     }
    //     printf("\n");
    //     printf("Server: Deserializing command...\n");
    //     ChatCmd *cmd = deserialize_chat_cmd(shm->buf, shm->bufSize);
    //     printf("Server: Shared memory content before deserialization: ");
    //     for (size_t i = 0; i < 16 && i < shm->bufSize; i++) {
    //         printf("%02x ", (unsigned char)shm->buf[i]);
    //     }
    //     printf("\n");
    //     if (!cmd) {
    //         printf("Server: Failed to deserialize command. Sending error response.\n");
    //         snprintf(shm->buf, shm->bufSize, "err SYS_ERR: Deserialization failed\n");
    //         sem_post(&shm->sems[1]); // Signal client that a response is ready
    //         continue;
    //     }


    //     printf("Server: Command Type: %d\n", cmd->type);

    //     if (cmd->type == END_CMD) {
    //         printf("Server: Received END_CMD. Shutting down.\n");
    //         free(cmd);
    //         break;
    //     }
    //     printf("Server: Processing command...\n");

    //     // Process the command...
    //     switch (cmd->type) {
    //         case ADD_CMD: {
    //             const AddCmd *add = &cmd->add;
    //             int errCode = add_chat_db(chatDb, add->user, add->room, add->nTopics,
    //                                       (const char **)add->topics, add->message);
    //             snprintf(shm->buf, shm->bufSize, errCode == 0 ? "ok\n" : "err SYS_ERR: %s\n", error_chat_db(chatDb));
    //             break;
    //         }
    //         case QUERY_CMD: {
    //             const QueryCmd *query = &cmd->query;
    //             size_t count;
    //             int errCode = count_room_chat_db(chatDb, query->room, &count);
    //             if (errCode != 0 || count == 0) {
    //                 snprintf(shm->buf, shm->bufSize, "err BAD_ROOM: unknown room\n");
    //             } else {
    //                 errCode = query_chat_db(chatDb, query->room, query->nTopics, 
    //                                         (const char **)query->topics, query->count, query_iterator, shm);
    //                 snprintf(shm->buf, shm->bufSize, errCode == 0 ? "ok\n" : "err SYS_ERR: %s\n", error_chat_db(chatDb));
    //             }
    //             break;
    //         }
    //         default:
    //             snprintf(shm->buf, shm->bufSize, "err SYS_ERR: unknown command\n");
    //             break;
    //     }

    //     free(cmd);

    //     // Signal client that response is ready
    //     sem_post(&shm->sems[1]);
    // }

    // // free_chat_db(chatDb);
    //  printf("Server: Exiting main loop.\n");
    // exit(0);


    printf("Server: Starting main loop...\n");

    ChatDb *db = NULL;
    MakeChatDbResult result;
    if (make_chat_db(dbPath, &result) != 0) {
        fprintf(stderr, "Server: Failed to initialize database: %s\n", result.err);
        return;
    }
    db = result.chatDb;

    while (1) {
        printf("Server: Waiting for client semaphore...\n");
        if (sem_wait(&shm->sems[0]) != 0) {
            perror("do_server: Failed to wait on client semaphore");
            break;
        }

        printf("Server: Acquiring mutex semaphore...\n");
        if (sem_wait(&shm->sems[2]) != 0) {
            perror("do_server: Failed to acquire mutex semaphore");
            break;
        }

        printf("Server: Deserializing command...\n");
        ChatCmd *cmd = deserialize_chat_cmd(shm->buf, shm->bufSize);

        if (cmd->type == END_CMD) {
            printf("Server: Received END_CMD. Exiting.\n");
            sem_post(&shm->sems[2]);
            break;
        }

        printf("Server: Processing command...\n");
        process_command(shm, cmd, db);

        printf("Server: Signaling client semaphore...\n");
        if (sem_post(&shm->sems[1]) != 0) {
            perror("do_server: Failed to signal client semaphore");
        }

        sem_post(&shm->sems[2]);  // Release mutex
    }

    if (db) free_chat_db(db);
    printf("Server: Exiting main loop.\n");


}




void process_command(Shm *shm, const ChatCmd *cmd, ChatDb *db) {
    if (!shm || !cmd || !db) {
        fprintf(stderr, "process_command: Invalid arguments\n");
        snprintf(shm->buf, shm->bufSize, "err SYS_ERR: Invalid arguments\n");
        return;
    }

    switch (cmd->type) {
        case ADD_CMD: {
            printf("Server: Handling ADD_CMD...\n");
            const AddCmd *add = &cmd->add;
            int result = add_chat_db(db,
                                     add->user,
                                     add->room,
                                     add->nTopics,
                                     add->topics,
                                     add->message);
            if (result == 0) {
                snprintf(shm->buf, shm->bufSize, "ok\n");
            } else {
                snprintf(shm->buf, shm->bufSize, "err SYS_ERR: Failed to add data\n");
            }
            break;
        }
        case QUERY_CMD: {
            printf("Server: Handling QUERY_CMD...\n");
            const QueryCmd *query = &cmd->query;
            int result = query_chat_db(db,
                                       query->room,
                                       query->nTopics,
                                       query->topics,
                                       query->count,
                                       query_iterator,
                                       shm);
            if (result == 0) {
                snprintf(shm->buf, shm->bufSize, "ok\n");
            } else {
                snprintf(shm->buf, shm->bufSize, "err SYS_ERR: Query failed\n");
            }
            break;
        }
        default:
            snprintf(shm->buf, shm->bufSize, "err SYS_ERR: Unknown command type\n");
            break;
    }
}





// int do_sem_op(SemSet *semSet, SemOp semOps[], int nSemOps) {
//     pthread_mutex_lock(&semSet->mutex);

//     // Check if all operations can proceed
//     while (!can_proceed(semSet, semOps, nSemOps)) {
//         pthread_cond_wait(&semSet->cond, &semSet->mutex);
//     }

//     // Perform the operations atomically
//     for (int i = 0; i < nSemOps; i++) {
//         semSet->semaphores[semOps[i].semIndex] += semOps[i].op;
//     }

//     // Signal other waiting threads that operations are complete
//     pthread_cond_broadcast(&semSet->cond);
//     pthread_mutex_unlock(&semSet->mutex);

//     return 0;
// }

int query_iterator(const ChatInfo *result, void *ctx) {
    Shm *shm = (Shm *)ctx;

    // Prepare the response string
    char buffer[shm->bufSize];
    size_t offset = 0;

    offset += snprintf(buffer + offset, shm->bufSize - offset, "%ld\n%s %s",
                       result->timestamp, result->user, result->room);

    if (result->nTopics > 0) {
        offset += snprintf(buffer + offset, shm->bufSize - offset, " ");
        for (int i = 0; i < result->nTopics; i++) {
            offset += snprintf(buffer + offset, shm->bufSize - offset, "%s%s",
                               result->topics[i], (i < result->nTopics - 1) ? " " : "");
        }
    }

    offset += snprintf(buffer + offset, shm->bufSize - offset, "\n%s\n", result->message);

    // Write the response to shared memory
    send_data(shm, buffer, offset);

    return 0; // Continue iteration
}