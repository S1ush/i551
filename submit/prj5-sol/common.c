#include "common.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN 1024
//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

//TODO add code used by both client and server

/** Write a message to a socket */
int write_message(int sockFd, const char *message) {
    // printf("DEBUG: Writing to socket FD %d: %s\n", sockFd, message);
    ssize_t written = write(sockFd, message, strlen(message));
    if (written < 0) {
        // perror("DEBUG: Error writing to socket");
        return -1;
    }
    return 0;
}

/** Read a message from a socket */
int read_message(int sockFd, char *buffer, size_t bufferSize) {
    ssize_t bytesRead = read(sockFd, buffer, bufferSize - 1);
    if (bytesRead < 0) {
        perror("DEBUG: Error reading from socket");
        return -1;
    }
    buffer[bytesRead] = '\0';
    // printf("DEBUG: Read from socket FD %d: %s\n", sockFd, buffer);
    return 0;
}

/** Helper function to parse headers */
int parse_header(const char *header, int *type, char *user, char *room) {
    int result = sscanf(header, "%d %s %s", type, user, room);
    if (result != 3) {
        fprintf(stderr, "Error parsing header: %s\n", header);
        return -1;
    }
    return 0;
}

/** Helper function to send a structured message */
int send_structured_message(int sockFd, int type, const char *user, const char *room, const char *content) {
    char message[MAX_MSG_LEN];
    snprintf(message, sizeof(message), "%d %s %s %s\n", type, user, room, content);
    return write_message(sockFd, message);
}

/** Close a socket gracefully */
void close_socket(int sockFd) {
    if (sockFd >= 0) {
        close(sockFd);
    }
}



// void write_header(const Hdr *hdr, FILE *out) {
//     fprintf(out, "%d %zu %zu %zu\n", hdr->cmdType, hdr->count, hdr->nTopics, hdr->nBytes);
//     fflush(out);
// }

// int read_header(Hdr *hdr, FILE *in) {
//     return fscanf(in, "%d %zu %zu %zu", &hdr->cmdType, &hdr->count, &hdr->nTopics, &hdr->nBytes) == 4;
// }