#include "common.h"
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

Shm *
create_shared_memory(size_t size) 
{
  if (size < MIN_SHM_SIZE) size = MIN_SHM_SIZE;
  
  // Create anonymous shared memory
  Shm *shm = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shm == MAP_FAILED) return NULL;

  // Initialize structure
  shm->shmSize = size;
  shm->bufSize = size - offsetof(Shm, buf);

  // Initialize semaphores with pshared=1 for inter-process sharing
  if (sem_init(&shm->readySem, 1, 1) < 0 ||      // Start ready for writing
      sem_init(&shm->clientDataSem, 1, 0) < 0 ||  // No client data initially
      sem_init(&shm->serverDataSem, 1, 0) < 0) {  // No server data initially
    munmap(shm, size);
    return NULL;
  }

  return shm;
}

void 
destroy_shared_memory(Shm *shm, size_t size)
{
  // Destroy semaphores
  sem_destroy(&shm->readySem);
  sem_destroy(&shm->clientDataSem);
  sem_destroy(&shm->serverDataSem);
  
  // Unmap shared memory
  munmap(shm, size);
}

void send_data(Shm *shm, bool isServer, const void *data, size_t size) {
    if (!shm || !data || size > shm->bufSize) {
        fprintf(stderr, "Send error: Invalid parameters\n");
        return;
    }
    
    fprintf(stderr, "%s: Waiting for readySem to send %zu bytes\n",
            isServer ? "Server" : "Client", size);
    
    sem_wait(&shm->readySem);
    
    fprintf(stderr, "%s: Got readySem, copying data\n",
            isServer ? "Server" : "Client");
    
    memcpy(shm->buf, data, size);
    
    fprintf(stderr, "%s: Posting %s\n",
            isServer ? "Server" : "Client",
            isServer ? "serverDataSem" : "clientDataSem");
    
    sem_post(isServer ? &shm->serverDataSem : &shm->clientDataSem);
}

void receive_data(Shm *shm, bool isServer, void *data, size_t size) {
    if (!shm || !data || size > shm->bufSize) {
        fprintf(stderr, "Receive error: Invalid parameters\n");
        return;
    }
    
    fprintf(stderr, "%s: Waiting for %s to receive %zu bytes\n",
            isServer ? "Server" : "Client",
            isServer ? "clientDataSem" : "serverDataSem",
            size);
    
    sem_wait(isServer ? &shm->clientDataSem : &shm->serverDataSem);
    
    fprintf(stderr, "%s: Got data sem, copying data\n",
            isServer ? "Server" : "Client");
    
    memcpy(data, shm->buf, size);
    
    fprintf(stderr, "%s: Posting readySem\n",
            isServer ? "Server" : "Client");
    
    sem_post(&shm->readySem);
}