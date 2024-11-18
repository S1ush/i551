#ifndef COMMON_H_
#define COMMON_H_

#include <chat-cmd.h>
#include <chat-db.h>
#include <semaphore.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include<str-space.h>

// declarations common between server and client

#define ERROR "err "
#define OKAY "ok\n"


#define N_SEM 2
#define CLIENT_WRITE_SEM 0
#define SERVER_READ_SEM  1

enum { MIN_SHM_SIZE = 1024 };

// Define shared memory structure
typedef struct {
    size_t shmSize;           // Specified size of the shared memory
    size_t bufSize;           // Max # of bytes in buf[]
    sem_t sems[N_SEM];        // Array of semaphores for synchronization
    char buf[];               // Buffer for data transfer
} Shm;

//TODO: add declarations useful to both server and client

#ifndef SEM_TRACE
#define SEM_TRACE 1
#endif

#if SEM_TRACE
#define SEM_VALUE(prg, state, sem, posixName) \
  do { \
    int sval; \
    if (sem_getvalue(sem, &sval) < 0) { \
      fatal("cannot get value for semaphore %s:", posixName); \
    } \
    fprintf(stderr, "%s: %s value of semaphore %s is %d\n", \
            prg, state, posixName, sval);                   \
  } while (0)
#else
#define SEM_VALUE(prg, state, sem, posixName) do { } while (0)
#endif

// Function prototypes for shared memory communication
// void send_data(Shm *shm, bool isServer, size_t nData, const void *data);
// void receive_data(Shm *shm, bool isServer, size_t nData, void *data);
Shm *init_shared_memory(size_t shmSize);
void send_data(Shm *shm, const void *data, size_t size);
void receive_data(Shm *shm, void *buffer, size_t size);
void cleanup_shared_memory(Shm *shm);
int query_iterator(const ChatInfo *result, void *ctx);
size_t serialize_chat_cmd(const ChatCmd *cmd, char *buffer, size_t buffer_size);
ChatCmd *deserialize_chat_cmd(const char *buffer, size_t buffer_size);
// void receive_data(Shm *shm, void *buffer, size_t size);
int serialize_add_cmd(const AddCmd *add, StrSpace *strSpace);
int serialize_query_cmd(const QueryCmd *query, StrSpace *strSpace);

int deserialize_add_cmd(const char *input, AddCmd *addCmd);
int deserialize_query_cmd(const char *input, QueryCmd *queryCmd);



#endif //#ifndef COMMON_H_
