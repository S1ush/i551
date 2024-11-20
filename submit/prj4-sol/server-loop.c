#include "server-loop.h"
#include "common.h"
#include <chat-db.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ChatDb *chatDb;
    Shm *shm;
} Server;

static void send_server_response(ChatDb *chatDb, ServerStatus status,
                               const char *msg, Shm *shm) {
    fprintf(stderr, "Server: Preparing response status=%d, msg='%s'\n", 
            status, msg ? msg : "NULL");

    size_t msgLen = msg ? strlen(msg) : 0;
    Hdr hdr = {
        .hdrType = 1,  // SERVER_HDR
        .status = status,
        .nBytes = msgLen
    };

    // Send complete response in one go
    size_t totalSize = sizeof(Hdr) + msgLen;
    char *buffer = malloc(totalSize);
    if (!buffer) {
        fprintf(stderr, "Server: Failed to allocate response buffer\n");
        return;
    }

    // Copy header and message into buffer
    memcpy(buffer, &hdr, sizeof(Hdr));
    if (msg && msgLen > 0) {
        memcpy(buffer + sizeof(Hdr), msg, msgLen);
    }

    // Send complete response
    fprintf(stderr, "Server: Sending response (size=%zu)\n", totalSize);
    send_data(shm, true, buffer, totalSize);
    free(buffer);
    
    fprintf(stderr, "Server: Response complete\n");
}

static void do_add_cmd(Server *server, const Hdr *cmdHdr) {
    fprintf(stderr, "Server: Processing ADD command with data size %d\n", cmdHdr->nBytes);

    // Receive the data portion directly
    char *buffer = malloc(cmdHdr->nBytes);
    if (!buffer) {
        fprintf(stderr, "Server: Memory allocation failed\n");
        send_server_response(server->chatDb, SYS_ERR_STATUS, 
                           "Memory allocation failed", server->shm);
        return;
    }

    // Read data from client
    receive_data(server->shm, true, buffer, cmdHdr->nBytes);
    fprintf(stderr, "Server: Received data buffer\n");

    // Parse data
    const char *user = buffer;
    const char *room = user + strlen(user) + 1;
    const char *message = room + strlen(room) + 1;
    const char *topics[cmdHdr->nTopics];
    
    const char *p = message + strlen(message) + 1;
    for (int i = 0; i < cmdHdr->nTopics; i++) {
        topics[i] = p;
        p += strlen(p) + 1;
    }

    // Add to database
    int errCode = add_chat_db(server->chatDb, user, room, cmdHdr->nTopics, 
                             topics, message);

    // Send response
    if (errCode != 0) {
        const char *errMsg = error_chat_db(server->chatDb);
        send_server_response(server->chatDb, SYS_ERR_STATUS, errMsg, server->shm);
    } else {
        send_server_response(server->chatDb, OK_STATUS, NULL, server->shm);
    }

    free(buffer);
    fprintf(stderr, "Server: ADD command complete\n");
}


static int query_iterator(const ChatInfo *info, void *data) {
    Server *server = data;
    if (!info || !server) return 0;

    fprintf(stderr, "Server: Processing query result\n");

    char timestamp_buf[32];
    if (timestamp_to_iso8601(info->timestamp, sizeof(timestamp_buf), timestamp_buf) != 0) {
        fprintf(stderr, "Server: Failed to format timestamp\n");
        send_server_response(server->chatDb, SYS_ERR_STATUS, 
                           "Failed to format timestamp", server->shm);
        return 0;
    }

    // Build the output
    char *output;
    size_t size = snprintf(NULL, 0, "+ @%s %s", info->user, info->room);
    for (size_t i = 0; i < info->nTopics; i++) {
        size += snprintf(NULL, 0, " #%s", info->topics[i]);
    }
    size += snprintf(NULL, 0, " [%s]\n%s\n", timestamp_buf, info->message);
    size++; // For null terminator

    output = malloc(size);
    if (!output) {
        fprintf(stderr, "Server: Memory allocation failed\n");
        send_server_response(server->chatDb, SYS_ERR_STATUS, 
                           "Memory allocation failed", server->shm);
        return 0;
    }

    char *p = output;
    p += sprintf(p, "+ @%s %s", info->user, info->room);
    for (size_t i = 0; i < info->nTopics; i++) {
        p += sprintf(p, " #%s", info->topics[i]);
    }
    sprintf(p, " [%s]\n%s\n", timestamp_buf, info->message);

    fprintf(stderr, "Server: Sending query result: '%s'\n", output);

    // Send response
    Hdr hdr = {
        .hdrType = 1,  // SERVER_HDR
        .status = OK_STATUS,
        .nBytes = strlen(output)
    };

    send_data(server->shm, true, &hdr, sizeof(Hdr));
    send_data(server->shm, true, output, hdr.nBytes);

    free(output);
    fprintf(stderr, "Server: Query result sent\n");
    return 1;
}

static void do_query_cmd(Server *server, const Hdr *cmdHdr) {
    fprintf(stderr, "Server: Processing QUERY command with data size %d\n", cmdHdr->nBytes);

    // Read the data
    char *buffer = malloc(cmdHdr->nBytes);
    if (!buffer) {
        fprintf(stderr, "Server: Memory allocation failed\n");
        send_server_response(server->chatDb, SYS_ERR_STATUS, 
                           "Memory allocation failed", server->shm);
        return;
    }

    receive_data(server->shm, true, buffer, cmdHdr->nBytes);

    // Parse data
    const char *room = buffer;
    const char *topics[cmdHdr->nTopics];
    const char *p = room + strlen(room) + 1;
    
    for (int i = 0; i < cmdHdr->nTopics; i++) {
        topics[i] = p;
        fprintf(stderr, "Server: Topic[%d]='%s'\n", i, topics[i]);
        p += strlen(p) + 1;
    }

    // Execute query
    fprintf(stderr, "Server: Executing query\n");
    int errCode = query_chat_db(server->chatDb, room, cmdHdr->nTopics, topics, 
                               cmdHdr->count, query_iterator, server);
    fprintf(stderr, "Server: Query complete with result: %d\n", errCode);

    // Send final response
    if (errCode != 0) {
        const char *errMsg = error_chat_db(server->chatDb);
        fprintf(stderr, "Server: Sending error response: %s\n", errMsg);
        send_server_response(server->chatDb, SYS_ERR_STATUS, errMsg, server->shm);
    } else {
        fprintf(stderr, "Server: Sending final OK response\n");
        send_server_response(server->chatDb, OK_STATUS, NULL, server->shm);
    }

    free(buffer);
    fprintf(stderr, "Server: QUERY command complete\n");
}

void server_loop(ChatDb *chatDb, Shm *shm) {
    if (!chatDb || !shm) return;

    Server server = { .chatDb = chatDb, .shm = shm };
    fprintf(stderr, "Server: Starting server loop\n");

    while (1) {
        // Read command header
        Hdr cmdHdr;
        fprintf(stderr, "Server: Waiting for next command\n");
        receive_data(shm, true, &cmdHdr, sizeof(Hdr));
        fprintf(stderr, "Server: Received command type=%d\n", cmdHdr.cmdType);

        if (cmdHdr.cmdType == END_CMD) {
            fprintf(stderr, "Server: Received END command, breaking loop\n");
            break;
        }

        switch (cmdHdr.cmdType) {
            case ADD_CMD:
                do_add_cmd(&server, &cmdHdr);
                break;
            case QUERY_CMD:
                do_query_cmd(&server, &cmdHdr);
                break;
            default:
                fprintf(stderr, "Server: Unknown command type: %d\n", cmdHdr.cmdType);
                send_server_response(chatDb, SYS_ERR_STATUS, 
                                   "Unknown command type", shm);
                break;
        }
    }
    
    fprintf(stderr, "Server: Server loop ended\n");
}