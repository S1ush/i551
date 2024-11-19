#include "chat.h"
#include "server.h"

#include <chat-cmd.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>


//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

// #define CLIENT_WRITE_SEM 0
// #define SERVER_READ_SEM  1


// sem_init(&shm->sems[CLIENT_WRITE_SEM], 1, 1); // Allows client to write
// sem_init(&shm->sems[SERVER_READ_SEM], 1, 0);  // Server waits to read

// Define the Client structure if it isn't defined elsewhere
// typedef struct {
//     FILE *out;
//     FILE *err;
// } Client;



// struct _Chat {
//   //TODO: fill out declaration
//     Client client;
//     pid_t serverPid;
//     Shm *shm;     // Pointer to shared memory segment
// };

struct _Chat {
    Shm *shm;
    pid_t serverPid;
    FILE *out;
    FILE *err;
};
//TODO: add private functions


/** Helper function to set up shared memory and semaphores */
// static Shm *init_shared_memory(size_t shmSize) {
//     Shm *shm = mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//     if (shm == MAP_FAILED) return NULL;

//     // Initialize semaphores in shared memory
//     sem_init(&shm->sems[CLIENT_WRITE_SEM], 1, 1);  // Client can write initially
//     sem_init(&shm->sems[SERVER_READ_SEM], 1, 0);   // Server waits initially
//     shm->shmSize = shmSize;
//     return shm;
// }

// Shm *init_shared_memory(size_t shmSize) {
//     size_t totalSize = sizeof(Shm) + shmSize;
//     Shm *shm = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//     if (shm == MAP_FAILED) {
//         perror("mmap");
//         return NULL;
//     }

//     shm->shmSize = totalSize;
//     shm->bufSize = shmSize;

//     if (sem_init(&shm->sems[0], 1, 0) != 0 ||  // Client-to-server semaphore
//         sem_init(&shm->sems[1], 1, 0) != 0 ||  // Server-to-client semaphore
//         sem_init(&shm->sems[2], 1, 1) != 0) {  // Mutex semaphore
//         perror("sem_init failed");
//         munmap(shm, shm->shmSize);
//         return NULL;
//     }

//     // Debug semaphore values after initialization
//     int val;
//     sem_getvalue(&shm->sems[2], &val);
//     printf("Debug: Mutex semaphore initialized to: %d\n", val);
//     // int val;
//     sem_getvalue(&shm->sems[0], &val);
//     printf("Semaphore [0] (Client-to-Server) initialized to: %d\n", val);
//     sem_getvalue(&shm->sems[1], &val);
//     printf("Semaphore [1] (Server-to-Client) initialized to: %d\n", val);
//     sem_getvalue(&shm->sems[2], &val);
//     printf("Semaphore [2] (Mutex) initialized to: %d\n", val);

//     return shm;
// }


/** Return a new Chat object which creates a server process which uses
 *  the sqlite database located at dbPath.  All commands must be sent
 *  by this client process to the server and handled by the server
 *  using the database. All IPC must use shared memory of size shmSize
 *  with POSIX semaphores used for synchronization.  The returned
 *  object should encapsulate all the state needed to implement the
 *  following API.
 *
 *  The client process must use `out` for writing success output for
 *  commands where each output must start with a line containing "ok".
 *
 *  The client process must use `err` for writing error message
 *  lines. Each line must start with "err ERR_CODE: " where ERR_CODE is
 *  as in your previous project for user errors.  System errors
 *  can result in unclean program termination.
 *
 *  [Note that since a `ChatCmd` is guaranteed to be syntactically
 *  valid, the only user errors which the program will need to detect
 *  will be `BAD_ROOM`/`BAD_TOPIC` for unknown room or topic.  This
 *  will have to be done by the server which will then return an error
 *  response to the client for output.]
 *
 *  The server should not use the `in` or `out` streams.  It may use
 *  `stderr` for "logging", but all such logging *must* be turned off
 *  before submission.
 *
 *  If errors are encountered, then this function should return NULL.
 */
Chat *
make_chat(const char *dbPath, size_t shmSize, FILE *out, FILE *err)
{
  // Allocate shared memory
  
    // size_t totalSize = sizeof(Shm) + shmSize;
    // Shm *shm = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    // if (!shm) {
    //     perror("mmap");
    //     return NULL;
    // }
    // shm->shmSize = totalSize;
    // shm->bufSize = shmSize;

    // // Initialize semaphores
    // if (sem_init(&shm->sems[0], 1, 0) || sem_init(&shm->sems[1], 1, 0) || sem_init(&shm->sems[2], 1, 1)) {
    //     perror("sem_init");
    //     munmap(shm, totalSize);
    //     fprintf(out,"this is initial smeaphore");
    //     return NULL;
    // }

    // // Fork server process
    // pid_t pid = fork();
    // if (pid < 0) {
    //     perror("fork");
    //     munmap(shm, totalSize);
    //     fprintf(out,"fork server process");
    //     return NULL;
    // }

    // if (pid == 0) {
    //     // Server process
    //     do_server(dbPath, shm);
    //     fprintf(out,"server process");
    //     exit(0);
    // }

    // // Client process
    // Chat *chat = malloc(sizeof(Chat));
    // chat->shm = shm;
    // chat->serverPid = pid;
    // chat->out = out;
    // chat->err = err;
    // return chat;


    Chat *chat = malloc(sizeof(Chat));
    if (!chat) {
        fprintf(err, "Error: Failed to allocate memory for Chat\n");
        return NULL;
    }

    // Initialize shared memory
    chat->shm = init_shared_memory(shmSize);
    if (!chat->shm) {
        fprintf(err, "Error: Failed to initialize shared memory\n");
        free(chat);
        return NULL;
    }

    // Fork the server process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        munmap(chat->shm, chat->shm->shmSize);
        free(chat);
        return NULL;
    }

    if (pid == 0) {
        // Server process
        do_server(dbPath, chat->shm);
        exit(0);  // Server process exits after handling commands
    }

    // Client process
    chat->serverPid = pid;
    chat->out = out;
    chat->err = err;

    return chat;


  // return NULL;
}

/** free all resources like memory used by chat.  All resources must
 *  be freed even after user errors have been detected.  It is okay if
 *  resources are not freed after system errors.
 */
void
free_chat(Chat *chat)
{
  // if (!chat) return;
  //   sem_destroy(&chat->shm->sems[0]);
  //   sem_destroy(&chat->shm->sems[1]);
  //   sem_destroy(&chat->shm->sems[2]);
  //   munmap(chat->shm, chat->shm->shmSize);
  //   free(chat);

  // if (!chat) return;

  //   // Send END_CMD to server
  //   ChatCmd endCmd = { .type = END_CMD };
  //   do_chat_cmd(chat, &endCmd);

  //   // Wait for the server process to terminate
  //   waitpid(chat->serverPid, NULL, 0);

  //   // Clean up shared memory
  //   sem_destroy(&chat->shm->sems[0]);
  //   sem_destroy(&chat->shm->sems[1]);
  //   sem_destroy(&chat->shm->sems[2]);
  //   munmap(chat->shm, chat->shm->shmSize);

  //   free(chat);

  //TODO
}


/** perform cmd using chat, with the client writing response to chat's
 *  out/err streams.  It can be assumed that cmd is free of user
 *  errors except for unknown room/topic for QUERY commands.
 *
 *  If the command is an END_CMD command, then ensure that the server
 *  process has terminated before returning.
 */
void
do_chat_cmd(Chat *chat, const ChatCmd *cmd)
{
  //TODO
  // send_data(chat->shm, false, sizeof(ChatCmd), cmd);  // Send command to server
  //   if (cmd->type == END_CMD) {
  //       waitpid(chat->serverPid, NULL, 0);  // Wait for server to terminate
  //   }

  // Serialize and write cmd to shared memory
  //  printf("Client: Sending command to server...\n");
  //   sem_wait(&chat->shm->sems[2]); // Acquire mutex
  //   printf("Client: Command sent to server: Type: %d\n", cmd->type);
  //   memcpy(chat->shm->buf, cmd, sizeof(ChatCmd)); // Simplified; handle serialization
  //   sem_post(&chat->shm->sems[0]); // Signal server
  //   sem_post(&chat->shm->sems[2]); // Release mutex
  //   char response[chat->shm->bufSize];
  //   receive_data(chat->shm, response, chat->shm->bufSize);
  //   printf("Client: Received response from server: %s\n",response);

  //   // Wait for server response
  //   sem_wait(&chat->shm->sems[1]);
  //   fprintf(chat->out, "%s", response);
  //   fprintf(chat->out, "%s", chat->shm->buf);


  // printf("Client: Sending command to server...\n");
    
  //   // Acquire mutex semaphore before writing to shared memory
  //   sem_wait(&chat->shm->sems[2]);
    
  //   // Serialize cmd into shared memory buffer
  //   size_t serialized_size = serialize_chat_cmd(cmd, chat->shm->buf, chat->shm->bufSize);
  //   // size_t serialized_size = serialize_chat_cmd(cmd, chat->shm->buf, chat->shm->bufSize);
  //   if (serialized_size > chat->shm->bufSize) {
  //       fprintf(chat->err, "err SYS_ERR: Serialized command exceeds buffer size\n");
  //       return;
  //   }
  //   printf("Client: Serialized command size: %zu (Buffer Size: %zu)\n", serialized_size, chat->shm->bufSize);
  //   printf("Client: Serialized command size: %zu\n", serialized_size);
    
  //   sem_post(&chat->shm->sems[0]); // Signal server that data is ready
  //   sem_post(&chat->shm->sems[2]); // Release mutex semaphore
    
  //   // Wait for and receive response
  //   char response[chat->shm->bufSize];
  //   receive_data(chat->shm, response, chat->shm->bufSize);
  //   printf("Client: Received response from server: %s\n", response);

  //   // Write the response to the output stream
  //   fprintf(chat->out, "%s", response);

  printf("Client: Acquiring mutex semaphore...\n");
    if (sem_wait(&chat->shm->sems[2]) != 0) {
        perror("do_chat_cmd: Failed to acquire mutex semaphore");
        return;
    }

    printf("Client: Serializing command...\n");
    size_t serialized_size = serialize_chat_cmd(cmd, chat->shm->buf, chat->shm->bufSize);
    printf("Client: Serialized command size: %zu\n", serialized_size);
    printf("Client: Serialized data (first 64 bytes): %.*s\n", 64, chat->shm->buf);

    printf("Client: Sending command to server...\n");
    if (sem_post(&chat->shm->sems[0]) != 0) {
        perror("do_chat_cmd: Failed to signal server semaphore");
    }

    printf("Client: Releasing mutex semaphore...\n");
    sem_post(&chat->shm->sems[2]);

    char response[chat->shm->bufSize];
    printf("Client: Waiting for server response...\n");
    if (sem_wait(&chat->shm->sems[1]) != 0) {
        perror("do_chat_cmd: Failed to wait for server semaphore");
        return;
    }

    memcpy(response, chat->shm->buf, chat->shm->bufSize);
    printf("Client: Server response: %s\n", response);
    fprintf(chat->out, "%s\n", response);
    
}

/** return server's PID */
pid_t chat_server_pid(const Chat *chat) {
  //TODO
  // return 0;
  return chat->serverPid;
}
