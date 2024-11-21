#include "chat.h"
#include "client.h"
#include "common.h"
#include "server.h"

#include <chat-cmd.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h> // for waitpid


//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

// define ADT
struct _Chat {
  Client client;
  pid_t serverPid;
  Shm *shm;          // shared memory pointer
  size_t shmSize;    // size of shared memory
};

/**************************** Private Routines *************************/

static Chat *
do_client(pid_t serverPid, Shm *shm, size_t shmSize, FILE *out, FILE *err)
{
  Chat *chat = NULL;
  
  chat = malloc(sizeof(struct _Chat));
  if (!chat) {
    errorf(err, ERROR "SYS_ERR: cannot malloc Chat:");
    return NULL;
  }

  *chat = (struct _Chat) {
    .client = (Client) {
      .out = out,
      .err = err,
      .shm = shm,
    },
    .serverPid = serverPid,
    .shm = shm,
    .shmSize = shmSize
  };
  
  return chat;
}

/******************************* Public API ****************************/

Chat *
make_chat(const char *dbPath, size_t shmSize, FILE *out, FILE *err)
{
  // Create shared memory segment
  Shm *shm = mmap(NULL, shmSize, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shm == MAP_FAILED) {
    errorf(err, ERROR "SYS_ERR: cannot create shared memory:");
    return NULL;
  }

  // Initialize shared memory
  shm->shmSize = shmSize;
  shm->bufSize = shmSize - offsetof(Shm, buf);

  // Initialize semaphores
  if (sem_init(&shm->readySem, 1, 1) < 0 ||  
      sem_init(&shm->clientDataSem, 1, 0) < 0 ||
      sem_init(&shm->serverDataSem, 1, 0) < 0) {
    munmap(shm, shmSize);
    errorf(err, ERROR "SYS_ERR: cannot initialize semaphores:");
    return NULL;
  }

  // Fork server process
  pid_t serverPid = fork();
  if (serverPid < 0) {
    munmap(shm, shmSize);
    errorf(err, ERROR "SYS_ERR: cannot fork server:");
    return NULL;
  }

  if (serverPid == 0) {  // server process
    do_server(dbPath, shm);
    exit(0);  // server should never return
  }
  else {  // client process
    Chat *chat = do_client(serverPid, shm, shmSize, out, err);
    if (chat == NULL) {
      munmap(shm, shmSize);
    }
    return chat;
  }

  return NULL;  // should never reach here
}

void
free_chat(Chat *chat)
{
  if (chat == NULL) return;

  // Clean up semaphores
  sem_destroy(&chat->shm->readySem);
  sem_destroy(&chat->shm->clientDataSem);
  sem_destroy(&chat->shm->serverDataSem);

  // Unmap shared memory
  munmap(chat->shm, chat->shmSize);

  // Free chat structure
  free(chat);
}

void
do_chat_cmd(Chat *chat, const ChatCmd *cmd)
{
  do_client_cmd(&chat->client, cmd);
  
  if (cmd->type == END_CMD) {
    int status;
    waitpid(chat->serverPid, &status, 0);  // Wait for server to terminate
  }
}

pid_t
chat_server_pid(const Chat *chat)
{
  return chat->serverPid;
}


