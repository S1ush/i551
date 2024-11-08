#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>
#include <stdio.h>

#include <unistd.h>

#define WELL_KNOWN_FIFO "./chat_server_fifo"
#define CLIENT_FIFO_PREFIX "./chat_client_"
#define MAX_FIFO_PATH_LEN 100

// Create path string for client's read FIFO
void make_client_read_fifo_path(char *path, pid_t pid);

// Create path string for client's write FIFO
void make_client_write_fifo_path(char *path, pid_t pid);

// Create a FIFO with proper permissions
int create_fifo(const char *path);

// Open FIFO with proper flags based on purpose
int open_fifo(const char *path, int flags);

// Clean up FIFOs for a client
void cleanup_client_fifos(pid_t pid);

//TODO
#endif //#ifndef UTILS_H_
