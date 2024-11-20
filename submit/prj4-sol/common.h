#ifndef COMMON_H_
#define COMMON_H_

#include <chat-cmd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

// declarations common between server and client
#define ERROR "err "
#define OKAY "ok\n"

// Header types
#define CLIENT_HDR 0
#define SERVER_HDR 1

enum { MIN_SHM_SIZE = 1024 };

#pragma pack(push, 1)  // Ensure consistent packing
typedef struct {
    int32_t hdrType;     // 0 for client, 1 for server
    int32_t cmdType;     // ADD_CMD, QUERY_CMD, etc.
    int32_t count;       // for query count
    int32_t status;      // for server response
    int32_t nTopics;     // number of topics
    int32_t nBytes;      // size of data following header
} Hdr;
#pragma pack(pop)

typedef struct {
    size_t shmSize;
    size_t bufSize;
    sem_t readySem;
    sem_t clientDataSem;
    sem_t serverDataSem;
    char buf[];
} Shm;

void send_data(Shm *shm, bool isServer, const void *data, size_t size);
void receive_data(Shm *shm, bool isServer, void *data, size_t size);

// Status codes
typedef enum {
    OK_STATUS,
    USER_ERR_STATUS,
    SYS_ERR_STATUS,
    FATAL_ERR_STATUS,
} ServerStatus;

#endif