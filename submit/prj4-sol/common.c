#include "common.h"

#include <errors.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>   // For mmap, munmap, and related constants
// mak#include <unistd.h>     // For close(), fork(), etc.
#include <errno.h>      // For error handling

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>
#include <stdlib.h>
#include <fcntl.h>



Shm *init_shared_memory(size_t shmSize) {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return NULL;
    }

    printf("Shared memory opened with fd: %d\n", shm_fd);

    size_t totalSize = sizeof(Shm) + shmSize;

    if (ftruncate(shm_fd, totalSize) == -1) {
        perror("ftruncate failed");
        close(shm_fd);
        return NULL;
    }

    printf("Shared memory truncated to size: %zu bytes\n", totalSize);

    Shm *shm = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);  // File descriptor no longer needed after mmap

    if (sem_init(&shm->sems[0], 1, 0) != 0 ||
        sem_init(&shm->sems[1], 1, 0) != 0 ||
        sem_init(&shm->sems[2], 1, 1) != 0) {
        perror("sem_init failed");
        munmap(shm, totalSize);
        return NULL;
    }

    printf("Semaphores initialized successfully\n");

    shm->shmSize = totalSize;
    shm->bufSize = shmSize;

    return shm;
}

//TODO: define functions useful to both client and server
// void send_data(Shm *shm, bool isServer, size_t nData, const void *data) {
//     if (isServer) sem_wait(&shm->sems[SERVER_READ_SEM]);
//     else sem_wait(&shm->sems[CLIENT_WRITE_SEM]);

//     memcpy(shm->buf, data, nData);

//     if (isServer) sem_post(&shm->sems[CLIENT_WRITE_SEM]);
//     else sem_post(&shm->sems[SERVER_READ_SEM]);
// }

// void receive_data(Shm *shm, bool isServer, size_t nData, void *data) {
//     if (isServer) sem_wait(&shm->sems[CLIENT_WRITE_SEM]);
//     else sem_wait(&shm->sems[SERVER_READ_SEM]);

//     memcpy(data, shm->buf, nData);

//     if (isServer) sem_post(&shm->sems[SERVER_READ_SEM]);
//     else sem_post(&shm->sems[CLIENT_WRITE_SEM]);
// }

// /** 
//  * Initialize shared memory and semaphores. 
//  */

// /** 
//  * Clean up shared memory and semaphores.
//  */
// // void cleanup_shared_memory(Shm *shm) {
// //     if (!shm) return;

// //     sem_destroy(&shm->sems[0]);
// //     sem_destroy(&shm->sems[1]);
// //     sem_destroy(&shm->sems[2]);
// //     munmap(shm, shm->shmSize);
// // }

// /**
//  * Send data to shared memory. 
//  */
// void send_data(Shm *shm, const ChatCmd *cmd, size_t bufSize) {
//     printf("send_data: Acquiring mutex semaphore...\n");
//     sem_wait(&shm->sems[2]); // Acquire mutex
//     printf("send_data: Writing serialized command to shared memory.\n");

//     size_t serialized_size = serialize_chat_cmd(cmd, shm->buf, bufSize);
//     if (serialized_size > bufSize) {
//         printf("send_data: Error - Serialized size exceeds buffer size.\n");
//     } else {
//         printf("send_data: Serialized size: %zu\n", serialized_size);
//     }

//     sem_post(&shm->sems[0]); // Signal server
//     sem_post(&shm->sems[2]); // Release mutex
//     printf("send_data: Data sent to shared memory successfully.\n");
// }

// /**
//  * Receive data from shared memory.
//  */
// void receive_data(Shm *shm, void *buffer, size_t size) {
//     printf("receive_data: Waiting for server semaphore...\n");
//     sem_wait(&shm->sems[1]); // Wait for server to write
//     printf("receive_data: Acquiring mutex semaphore...\n");
//     sem_wait(&shm->sems[2]); // Acquire mutex
//     printf("receive_data: Reading data from shared memory. Expected size: %zu\n", size);
//     memcpy(buffer, shm->buf, size);
//     printf("receive_data: Data received: %s\n", (char *)buffer);
//     sem_post(&shm->sems[2]); // Release mutex
//     printf("receive_data: Data read from shared memory successfully.\n");
// }


// int query_iterator(const ChatInfo *result, void *ctx) {
//     Shm *shm = (Shm *)ctx;

//     // Prepare the response string
//     char buffer[shm->bufSize];
//     size_t offset = 0;

//     offset += snprintf(buffer + offset, shm->bufSize - offset, "%ld\n%s %s",
//                        result->timestamp, result->user, result->room);

//     if (result->nTopics > 0) {
//         offset += snprintf(buffer + offset, shm->bufSize - offset, " ");
//         for (int i = 0; i < result->nTopics; i++) {
//             offset += snprintf(buffer + offset, shm->bufSize - offset, "%s%s",
//                                result->topics[i], (i < result->nTopics - 1) ? " " : "");
//         }
//     }

//     offset += snprintf(buffer + offset, shm->bufSize - offset, "\n%s\n", result->message);

//     // Write the response to shared memory
//     send_data(shm, buffer, offset);

//     return 0; // Continue iteration
// }



// size_t serialize_chat_cmd(const ChatCmd *cmd, char *buffer, size_t buffer_size) {
    
//     size_t offset = 0;
//     printf("Serializing command: Type: %d\n", cmd->type);

//     // Serialize CmdType
//     memcpy(buffer + offset, &cmd->type, sizeof(int));
//     if (offset + sizeof(cmd->type) > buffer_size) {
//         printf("Serialization Error: Buffer too small for CmdType\n");
//         return 0;
//     }
//     printf("Serialized CmdType: %d at offset %zu\n", cmd->type, offset);
//     offset += sizeof(cmd->type);

//     switch (cmd->type) {
//         case ADD_CMD: {
//             const AddCmd *add = &cmd->add;

            

//             // Serialize user
//             size_t user_len = strlen(add->user) + 1; // Include null terminator
//             printf("Serializing user: %s (length: %zu)\n", add->user, user_len);
//             memcpy(buffer + offset, &user_len, sizeof(size_t));
//             offset += sizeof(size_t);
//             memcpy(buffer + offset, add->user, user_len);
//             offset += user_len;

//             // Serialize room
//             size_t room_len = strlen(add->room) + 1;
//             printf("Serializing room: %s (length: %zu)\n", add->room, room_len);
//             memcpy(buffer + offset, &room_len, sizeof(size_t));
//             offset += sizeof(size_t);
//             memcpy(buffer + offset, add->room, room_len);
//             offset += room_len;

//             // Serialize message
//             size_t message_len = strlen(add->message) + 1;
//             printf("Serializing message: %s (length: %zu)\n", add->message, message_len);
//             memcpy(buffer + offset, &message_len, sizeof(size_t));
//             offset += sizeof(size_t);
//             memcpy(buffer + offset, add->message, message_len);
//             offset += message_len;

//             // Serialize topics
//             memcpy(buffer + offset, &add->nTopics, sizeof(size_t));
//             offset += sizeof(size_t);
//             for (size_t i = 0; i < add->nTopics; i++) {
//                 size_t topic_len = strlen(add->topics[i]) + 1;
//                 memcpy(buffer + offset, &topic_len, sizeof(size_t));
//                 offset += sizeof(size_t);
//                 memcpy(buffer + offset, add->topics[i], topic_len);
//                 offset += topic_len;
//             }
//             break;
//         }
//         case QUERY_CMD: {
//             const QueryCmd *query = &cmd->query;

//             // Serialize room
//             size_t room_len = strlen(query->room) + 1;
//             memcpy(buffer + offset, &room_len, sizeof(size_t));
//             offset += sizeof(size_t);
//             memcpy(buffer + offset, query->room, room_len);
//             offset += room_len;

//             // Serialize count
//             memcpy(buffer + offset, &query->count, sizeof(size_t));
//             offset += sizeof(size_t);

//             // Serialize topics
//             memcpy(buffer + offset, &query->nTopics, sizeof(size_t));
//             offset += sizeof(size_t);
//             for (size_t i = 0; i < query->nTopics; i++) {
//                 size_t topic_len = strlen(query->topics[i]) + 1;
//                 memcpy(buffer + offset, &topic_len, sizeof(size_t));
//                 offset += sizeof(size_t);
//                 memcpy(buffer + offset, query->topics[i], topic_len);
//                 offset += topic_len;
//             }
//             break;
//         }
//         case END_CMD:
//             // No additional data for END_CMD
//             break;
//         default:
//             printf("serialize_chat_cmd: Unsupported command type: %d\n", cmd->type);
//             break;
//     }

//     return offset;
// }



// ChatCmd *deserialize_chat_cmd(const char *buffer, size_t buffer_size) {
//     size_t offset = 0;
//     ChatCmd *cmd = malloc(sizeof(ChatCmd));
//     if (!cmd) {
//         printf("Deserialization Error: Failed to allocate memory for ChatCmd.\n");
//         return NULL;
//     }

//     // Deserialize CmdType
//     if (offset + sizeof(cmd->type) > buffer_size) {
//         printf("Deserialization Error: Buffer overflow while reading CmdType.\n");
//         free(cmd);
//         return NULL;
//     }
//     memcpy(&cmd->type, buffer + offset, sizeof(int));
//     if (offset + sizeof(cmd->type) > buffer_size) {
//         printf("Deserialization Error: Buffer too small for CmdType\n");
//         free(cmd);
//         return NULL;
//     }   
//     printf("Deserialization: Raw Command Type: %d\n", cmd->type);
//     offset += sizeof(cmd->type);
    

//     switch (cmd->type) {
//         case ADD_CMD: {
//             AddCmd *add = &cmd->add;

//             // Deserialize user
//             size_t user_len;
//             if (offset + sizeof(size_t) > buffer_size) {
//                 printf("Deserialization Error: Buffer overflow while reading user length.\n");
//                 free(cmd);
//                 return NULL;
//             }
//             memcpy(&user_len, buffer + offset, sizeof(size_t));
//             offset += sizeof(size_t);

//             if (offset + user_len > buffer_size) {
//                 printf("Deserialization Error: Buffer overflow while reading user.\n");
//                 free(cmd);
//                 return NULL;
//             }
//             add->user = strndup(buffer + offset, user_len);
//             offset += user_len;
//             printf("Deserialization: User: %s (length: %zu)\n", add->user, user_len);

//             // Deserialize room
//             size_t room_len;
//             if (offset + sizeof(size_t) > buffer_size) {
//                 printf("Deserialization Error: Buffer overflow while reading room length.\n");
//                 free(cmd);
//                 return NULL;
//             }
//             memcpy(&room_len, buffer + offset, sizeof(size_t));
//             offset += sizeof(size_t);

//             if (offset + room_len > buffer_size) {
//                 printf("Deserialization Error: Buffer overflow while reading room.\n");
//                 free(cmd);
//                 return NULL;
//             }
//             add->room = strndup(buffer + offset, room_len);
//             offset += room_len;
//             printf("Deserialization: Room: %s (length: %zu)\n", add->room, room_len);

//             // Deserialize message
//             size_t message_len;
//             if (offset + sizeof(size_t) > buffer_size) {
//                 printf("Deserialization Error: Buffer overflow while reading message length.\n");
//                 free(cmd);
//                 return NULL;
//             }
//             memcpy(&message_len, buffer + offset, sizeof(size_t));
//             offset += sizeof(size_t);

//             if (offset + message_len > buffer_size) {
//                 printf("Deserialization Error: Buffer overflow while reading message.\n");
//                 free(cmd);
//                 return NULL;
//             }
//             add->message = strndup(buffer + offset, message_len);
//             offset += message_len;
//             printf("Deserialization: Message: %s (length: %zu)\n", add->message, message_len);

//             // Deserialize topics
//             if (offset + sizeof(size_t) > buffer_size) {
//                 printf("Deserialization Error: Buffer overflow while reading topic count.\n");
//                 free(cmd);
//                 return NULL;
//             }
//             memcpy(&add->nTopics, buffer + offset, sizeof(size_t));
//             offset += sizeof(size_t);
//             printf("Deserialization: Number of Topics: %zu\n", add->nTopics);

//             add->topics = malloc(add->nTopics * sizeof(char *));
//             if (!add->topics) {
//                 printf("Deserialization Error: Failed to allocate memory for topics.\n");
//                 free(cmd);
//                 return NULL;
//             }

//             for (size_t i = 0; i < add->nTopics; i++) {
//                 size_t topic_len;
//                 if (offset + sizeof(size_t) > buffer_size) {
//                     printf("Deserialization Error: Buffer overflow while reading topic length.\n");
//                     free(cmd);
//                     return NULL;
//                 }
//                 memcpy(&topic_len, buffer + offset, sizeof(size_t));
//                 offset += sizeof(size_t);

//                 if (offset + topic_len > buffer_size) {
//                     printf("Deserialization Error: Buffer overflow while reading topic.\n");
//                     free(cmd);
//                     return NULL;
//                 }
//                 add->topics[i] = strndup(buffer + offset, topic_len);
//                 offset += topic_len;
//                 printf("Deserialization: Topic[%zu]: %s (length: %zu)\n", i, add->topics[i], topic_len);
//             }
//             break;
//         }
//         case QUERY_CMD: {
//             QueryCmd *query = &cmd->query;

//             // Deserialize room
//             size_t room_len;
//             memcpy(&room_len, buffer + offset, sizeof(size_t));
//             offset += sizeof(size_t);
//             query->room = strndup(buffer + offset, room_len);
//             offset += room_len;

//             // Deserialize count
//             memcpy(&query->count, buffer + offset, sizeof(size_t));
//             offset += sizeof(size_t);

//             // Deserialize topics
//             memcpy(&query->nTopics, buffer + offset, sizeof(size_t));
//             offset += sizeof(size_t);
//             query->topics = malloc(query->nTopics * sizeof(char *));
//             for (size_t i = 0; i < query->nTopics; i++) {
//                 size_t topic_len;
//                 memcpy(&topic_len, buffer + offset, sizeof(size_t));
//                 offset += sizeof(size_t);
//                 query->topics[i] = strndup(buffer + offset, topic_len);
//                 offset += topic_len;
//             }
//             break;
//         }
//         case END_CMD:
//             printf("Deserialization: END_CMD - No additional data.\n");
//             break;
//         default:
//             printf("Deserialization Error: Unknown command type.\n");
//             free(cmd);
//             return NULL;
//     }
//     printf("Deserialization: Command successfully deserialized.\n");
//     return cmd;
// }


size_t serialize_chat_cmd(const ChatCmd *cmd, char *buffer, size_t buffer_size) {
    size_t offset = 0;

    // Serialize the CmdType
    memcpy(buffer + offset, &cmd->type, sizeof(cmd->type));
    offset += sizeof(cmd->type);

    // Use StrSpace for additional serialization
    StrSpace strSpace;
    init_str_space(&strSpace);

    switch (cmd->type) {
        case ADD_CMD:
            if (serialize_add_cmd(&cmd->add, buffer + offset, buffer_size - offset) != 0) {
                fprintf(stderr, "Error: Failed to serialize AddCmd\n");
                return -1;  // Handle serialization failure
            }
            break;
        case QUERY_CMD:
            if (serialize_query_cmd(&cmd->query, &strSpace) != 0) {
                fprintf(stderr, "Failed to serialize QueryCmd\n");
                free_str_space(&strSpace);
                return 0;
            }
            break;
        default:
            fprintf(stderr, "Unknown command type\n");
            free_str_space(&strSpace);
            return 0;
    }

    // Copy serialized data from StrSpace into the buffer
    const char *serialized_data = iter_str_space(&strSpace, NULL);
    size_t data_length = strlen(serialized_data);
    if (data_length + offset > buffer_size) {
        fprintf(stderr, "Serialized data exceeds buffer size\n");
        free_str_space(&strSpace);
        return 0;
    }

    memcpy(buffer + offset, serialized_data, data_length);
    offset += data_length;

    free_str_space(&strSpace);  // Clean up StrSpace
    return offset;
}

ChatCmd *deserialize_chat_cmd(const char *buffer, size_t buffer_size)
{
    size_t offset = 0;
    ChatCmd *cmd = malloc(sizeof(ChatCmd));

    // Deserialize CmdType
    memcpy(&cmd->type, buffer + offset, sizeof(cmd->type));
    offset += sizeof(cmd->type);

    // Deserialize based on CmdType
    switch (cmd->type) {
        case ADD_CMD:
            deserialize_add_cmd(buffer + offset, &cmd->add);
            break;
        case QUERY_CMD:
            deserialize_query_cmd(buffer + offset, &cmd->query);
            break;
        default:
            break;
    }

    return cmd;
}

int serialize_add_cmd(const AddCmd *add, char *buffer, size_t buffer_size) {
    size_t offset = 0;

    // Serialize user
    size_t user_len = strlen(add->user) + 1;
    if (offset + user_len > buffer_size) return 0;
    memcpy(buffer + offset, add->user, user_len);
    offset += user_len;

    // Serialize room
    size_t room_len = strlen(add->room) + 1;
    if (offset + room_len > buffer_size) return 0;
    memcpy(buffer + offset, add->room, room_len);
    offset += room_len;

    // Serialize message
    size_t message_len = strlen(add->message) + 1;
    if (offset + message_len > buffer_size) return 0;
    memcpy(buffer + offset, add->message, message_len);
    offset += message_len;

    // Serialize number of topics
    if (offset + sizeof(size_t) > buffer_size) return 0;
    memcpy(buffer + offset, &add->nTopics, sizeof(size_t));
    offset += sizeof(size_t);

    // Serialize each topic
    for (size_t i = 0; i < add->nTopics; i++) {
        size_t topic_len = strlen(add->topics[i]) + 1;
        if (offset + topic_len > buffer_size) return 0;
        memcpy(buffer + offset, add->topics[i], topic_len);
        offset += topic_len;
    }

    return offset;
}

int serialize_query_cmd(const QueryCmd *query, StrSpace *strSpace) {
    // Initialize string space
    init_str_space(strSpace);

    // Serialize QueryCmd fields
    append_sprintf_str_space(strSpace, 
        "room=%s;count=%zu;nTopics=%zu;", 
        query->room, query->count, query->nTopics);

    for (size_t i = 0; i < query->nTopics; i++) {
        append_sprintf_str_space(strSpace, "topic[%zu]=%s;", i, query->topics[i]);
    }

    return 0;  // Serialization successful
}

int deserialize_add_cmd(const char *buffer, AddCmd *add) {
    // memset(addCmd, 0, sizeof(AddCmd));  // Initialize AddCmd structure

    // char user[128], room[128];
    // size_t nTopics;

    // if (sscanf(input, "user=%127[^;];room=%127[^;];", user, room) != 2) {
    //     return -1;  // Parsing error
    // }

    // const char *message_start = strstr(input, "message=");
    // if (!message_start) return -1;

    // message_start += strlen("message=");
    // const char *message_end = strchr(message_start, ';');
    // if (!message_end) return -1;

    // size_t message_len = message_end - message_start;
    // char *message = malloc(message_len + 1);
    // if (!message) return -1;

    // strncpy(message, message_start, message_len);
    // message[message_len] = '\0';

    // addCmd->user = strdup(user);
    // addCmd->room = strdup(room);
    // addCmd->message = message;

    // return 0;

    size_t offset = 0;

    // Deserialize user
    add->user = strdup(buffer + offset);
    offset += strlen(add->user) + 1;

    // Deserialize room
    add->room = strdup(buffer + offset);
    offset += strlen(add->room) + 1;

    // Deserialize message
    add->message = strdup(buffer + offset);
    offset += strlen(add->message) + 1;

    // Deserialize number of topics
    memcpy(&add->nTopics, buffer + offset, sizeof(size_t));
    offset += sizeof(size_t);

    // Deserialize each topic
    add->topics = malloc(add->nTopics * sizeof(char *));
    for (size_t i = 0; i < add->nTopics; i++) {
        add->topics[i] = strdup(buffer + offset);
        offset += strlen(add->topics[i]) + 1;
    }

    return 0;
}


void send_data(Shm *shm, const void *data, size_t size) {
    if (!shm || !data) {
        fprintf(stderr, "send_data: Invalid arguments\n");
        return;
    }

    printf("send_data: Acquiring mutex semaphore...\n");
    if (sem_wait(&shm->sems[2]) != 0) {  // Attempt to acquire mutex
        perror("send_data: Failed to acquire mutex semaphore");
        return;
    }

    printf("send_data: Writing data to shared memory (size: %zu)...\n", size);
    if (size > shm->bufSize) {
        fprintf(stderr, "send_data: Data size exceeds buffer capacity\n");
        sem_post(&shm->sems[2]);  // Release mutex
        return;
    }

    memcpy(shm->buf, data, size);  // Write data to shared memory

    printf("send_data: Signaling server semaphore...\n");
    if (sem_post(&shm->sems[0]) != 0) {  // Signal server
        perror("send_data: Failed to signal server semaphore");
    }

    printf("send_data: Releasing mutex semaphore...\n");
    sem_post(&shm->sems[2]);  // Release mutex
}
void receive_data(Shm *shm, void *buffer, size_t size) {
    if (!shm || !buffer) {
        fprintf(stderr, "receive_data: Invalid arguments\n");
        return;
    }

    printf("receive_data: Waiting for server semaphore...\n");
    if (sem_wait(&shm->sems[1]) != 0) {  // Wait for server
        perror("receive_data: Failed to wait on server semaphore");
        return;
    }

    printf("receive_data: Acquiring mutex semaphore...\n");
    if (sem_wait(&shm->sems[2]) != 0) {  // Acquire mutex
        perror("receive_data: Failed to acquire mutex semaphore");
        return;
    }

    printf("receive_data: Reading data from shared memory (size: %zu)...\n", size);
    memcpy(buffer, shm->buf, size);  // Read data from shared memory

    printf("receive_data: Releasing mutex semaphore...\n");
    sem_post(&shm->sems[2]);  // Release mutex
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