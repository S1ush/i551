#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

void make_client_read_fifo_path(char *path, pid_t pid) {
    snprintf(path, MAX_FIFO_PATH_LEN, "%s%d.0", CLIENT_FIFO_PREFIX, pid);
}

void make_client_write_fifo_path(char *path, pid_t pid) {
    snprintf(path, MAX_FIFO_PATH_LEN, "%s%d.1", CLIENT_FIFO_PREFIX, pid);
}

int create_fifo(const char *path) {
    if (mkfifo(path, 0666) == -1) {
        if (errno != EEXIST) {
            return -1;
        }
    }
    return 0;
}

int open_fifo(const char *path, int flags) {
    // printf("/opening fifo");
    // return -1;
    int fd = open(path, flags | O_NONBLOCK);
    if (fd == -1) {
        return -1;
    }
    return fd;
}

void cleanup_client_fifos(pid_t pid) {
    char path[MAX_FIFO_PATH_LEN];
    
    make_client_read_fifo_path(path, pid);
    unlink(path);
    
    make_client_write_fifo_path(path, pid);
    unlink(path);
}