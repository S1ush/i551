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
  if (sem_init(&shm->readySem, 1, 1) < 0 ||
    sem_init(&shm->clientDataSem, 1, 0) < 0 ||
    sem_init(&shm->serverDataSem, 1, 0)) {
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
    if (!shm || !data || size == 0) {
        fprintf(stderr, "Send error: Invalid parameters\n");
        return;
    }

    // Send data in chunks
    const char *dataPtr = data;
    size_t remaining = size;
    size_t chunkSize = shm->bufSize;

    while (remaining > 0) {
        // Calculate size of this chunk
        size_t currentChunk = (remaining < chunkSize) ? remaining : chunkSize;

        // Wait for buffer to be available
        sem_wait(&shm->readySem);

        // Copy chunk
        memcpy(shm->buf, dataPtr, currentChunk);

        // Signal data is available
        sem_post(isServer ? &shm->serverDataSem : &shm->clientDataSem);

        // Update pointers and remaining size
        dataPtr += currentChunk;
        remaining -= currentChunk;
    }
}

void receive_data(Shm *shm, bool isServer, void *data, size_t size) {
    if (!shm || !data || size == 0) {
        fprintf(stderr, "Receive error: Invalid parameters\n");
        return;
    }

    // Receive data in chunks
    char *dataPtr = data;
    size_t remaining = size;
    size_t chunkSize = shm->bufSize;

    while (remaining > 0) {
        // Calculate size of this chunk
        size_t currentChunk = (remaining < chunkSize) ? remaining : chunkSize;

        // Wait for data to be available
        sem_wait(isServer ? &shm->clientDataSem : &shm->serverDataSem);

        // Copy chunk
        memcpy(dataPtr, shm->buf, currentChunk);

        // Signal buffer is available
        sem_post(&shm->readySem);

        // Update pointers and remaining size
        dataPtr += currentChunk;
        remaining -= currentChunk;
    }
}