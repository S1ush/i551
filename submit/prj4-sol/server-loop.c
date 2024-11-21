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


static int query_iterator(const ChatInfo *info, void *data) ;
static void send_server_response(ChatDb *chatDb, ServerStatus status,
                               const char *msg, Shm *shm) {
    // fprintf(stderr, "Server: Preparing response status=%d, msg='%s'\n", 
            // status, msg ? msg : "NULL");

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
    // fprintf(stderr, "Server: Sending response (size=%zu)\n", totalSize);
    send_data(shm, true, buffer, totalSize);
    free(buffer);
    
    // fprintf(stderr, "Server: Response complete\n");
}

static void do_add_cmd(Server *server, const Hdr *cmdHdr) {
    // fprintf(stderr, "Server: Starting ADD command processing\n");
    
    char *buffer = malloc(cmdHdr->nBytes);
    if (!buffer) {
        // fprintf(stderr, "Server: Memory allocation failed in ADD\n");
        const char *errMsg = "Memory allocation failed";
        Hdr respHdr = {
            .hdrType = 1,
            .status = SYS_ERR_STATUS,
            .nBytes = strlen(errMsg)
        };
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
        send_data(server->shm, true, errMsg, respHdr.nBytes);
        return;
    }

    // fprintf(stderr, "Server: Reading ADD command data\n");
    receive_data(server->shm, true, buffer, cmdHdr->nBytes);

    // Parse and validate data
    const char *user = buffer;
    if (user[0] != '@') {  // Ensure user starts with @
        user++;  // Skip the @ if it's in the input
    }
    // fprintf(stderr, "Server: User='%s'\n", user);

    const char *room = user + strlen(user) + 1;
    // fprintf(stderr, "Server: Room='%s'\n", room/);

    const char *message = room + strlen(room) + 1;
    // fprintf(stderr, "Server: Message='%s'\n", message);

    const char *topics[cmdHdr->nTopics];
    const char *p = message + strlen(message) + 1;
    
    for (int i = 0; i < cmdHdr->nTopics; i++) {
        topics[i] = p;
        if (topics[i][0] == '#') {  // Skip the # if it's in the input
            topics[i]++;
        }
        // fprintf(stderr, "Server: Topic[%d]='%s'\n", i, topics[i]);
        p += strlen(p) + 1;
    }

    // fprintf(stderr, "Server: Adding to database with %d topics\n", cmdHdr->nTopics);
    int errCode = add_chat_db(server->chatDb, user, room, cmdHdr->nTopics,
                             topics, message);
    // fprintf(stderr, "Server: add_chat_db returned %d\n", errCode);

    if (errCode != 0) {
        fprintf(stderr, "Server: ADD failed, sending error\n");
        const char *errMsg = error_chat_db(server->chatDb);
        Hdr respHdr = {
            .hdrType = 1,
            .status = SYS_ERR_STATUS,
            .nBytes = strlen(errMsg)
        };
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
        send_data(server->shm, true, errMsg, respHdr.nBytes);
    } else {
        // fprintf(stderr, "Server: ADD successful, sending OK\n");
        Hdr respHdr = {
            .hdrType = 1,
            .status = OK_STATUS,
            .nBytes = 0
        };
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
    }

    free(buffer);
    // fprintf(stderr, "Server: ADD command complete\n");
}

static void do_query_cmd(Server *server, const Hdr *cmdHdr) {
    char *buffer = malloc(cmdHdr->nBytes);
    if (!buffer) {
        const char *errMsg = "Memory allocation failed";
        Hdr respHdr = {
            .hdrType = 1,
            .status = SYS_ERR_STATUS,
            .nBytes = strlen(errMsg)
        };
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
        send_data(server->shm, true, errMsg, respHdr.nBytes);
        return;
    }

    receive_data(server->shm, true, buffer, cmdHdr->nBytes);

    // Validate room
    const char *room = buffer;
    size_t count;
    int errCode = count_room_chat_db(server->chatDb, room, &count);
    if (errCode != 0) {
        const char *errMsg = error_chat_db(server->chatDb);
        Hdr respHdr = {
            .hdrType = 1,
            .status = SYS_ERR_STATUS,
            .nBytes = strlen(errMsg)
        };
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
        send_data(server->shm, true, errMsg, respHdr.nBytes);
        free(buffer);
        return;
    }
    else if (count == 0) {
        const char *errMsg = "BAD_ROOM: unknown room";
        Hdr respHdr = {
            .hdrType = 1,
            .status = USER_ERR_STATUS,
            .nBytes = strlen(errMsg)
        };
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
        send_data(server->shm, true, errMsg, respHdr.nBytes);
        free(buffer);
        return;
    }

    // Parse and validate topics
    const char *topics[cmdHdr->nTopics];
    if (cmdHdr->nTopics > 0) {
        const char *p = room + strlen(room) + 1;
        for (int i = 0; i < cmdHdr->nTopics; i++) {
            topics[i] = p;
            if (topics[i][0] == '#') {
                topics[i]++;
            }
            p += strlen(p) + 1;

            // Validate each topic
            errCode = count_topic_chat_db(server->chatDb, topics[i], &count);
            if (errCode != 0) {
                const char *errMsg = error_chat_db(server->chatDb);
                Hdr respHdr = {
                    .hdrType = 1,
                    .status = SYS_ERR_STATUS,
                    .nBytes = strlen(errMsg)
                };
                send_data(server->shm, true, &respHdr, sizeof(Hdr));
                send_data(server->shm, true, errMsg, respHdr.nBytes);
                free(buffer);
                return;
            }
            else if (count == 0) {
                const char *errMsg = "BAD_TOPIC: unknown topic";
                Hdr respHdr = {
                    .hdrType = 1,
                    .status = USER_ERR_STATUS,
                    .nBytes = strlen(errMsg)
                };
                send_data(server->shm, true, &respHdr, sizeof(Hdr));
                send_data(server->shm, true, errMsg, respHdr.nBytes);
                free(buffer);
                return;
            }
        }
    }

    // Send initial OK message
    Hdr respHdr = {
        .hdrType = 1,
        .status = OK_STATUS,
        .nBytes = 1  // Send a dummy byte to indicate there might be more data
    };
    send_data(server->shm, true, &respHdr, sizeof(Hdr));
    send_data(server->shm, true, "", 1);  // Send dummy data

    // Execute query
    errCode = query_chat_db(server->chatDb, room, cmdHdr->nTopics,
                           cmdHdr->nTopics > 0 ? topics : NULL,
                           cmdHdr->count, query_iterator, server);

    if (errCode != 0) {
        const char *errMsg = error_chat_db(server->chatDb);
        respHdr.status = SYS_ERR_STATUS;
        respHdr.nBytes = strlen(errMsg);
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
        send_data(server->shm, true, errMsg, respHdr.nBytes);
    } else {
        // Send final empty message
        respHdr.nBytes = 0;
        send_data(server->shm, true, &respHdr, sizeof(Hdr));
    }

    free(buffer);
}
static int query_iterator(const ChatInfo *info, void *data) {
    Server *server = data;
    if (!info || !server) return 0;
   
    // fprintf(stderr, "Server: Processing query result\n");
   
    char timestamp_buf[32];
    timestamp_to_iso8601(info->timestamp, sizeof(timestamp_buf), timestamp_buf);
    // fprintf(stderr, "Server: Timestamp formatted\n");

    // Calculate size and allocate buffer
    char *output;
    size_t size = snprintf(NULL, 0, "%s\n", timestamp_buf);
    size += snprintf(NULL, 0, "%s %s", info->user, info->room);
    for (size_t i = 0; i < info->nTopics; i++) {
        size += snprintf(NULL, 0, " #%s", info->topics[i]);
    }
    size += snprintf(NULL, 0, "\n%s", info->message);
    size++; // For null terminator

    // fprintf(stderr, "Server: Allocating buffer of size %zu\n", size);
    output = malloc(size);
    if (!output) {
        fprintf(stderr, "Server: Memory allocation failed in query_iterator\n");
        return 0;
    }

    // Build the message
    char *p = output;
    p += sprintf(p, "%s\n", timestamp_buf);
    p += sprintf(p, "%s %s", info->user, info->room);
    for (size_t i = 0; i < info->nTopics; i++) {
        p += sprintf(p, " #%s", info->topics[i]);
    }
    sprintf(p, "\n%s", info->message);

    // fprintf(stderr, "Server: Sending result data: '%s'\n", output);
    // Send result
    Hdr hdr = {
        .hdrType = 1,
        .status = OK_STATUS,
        .nBytes = strlen(output)
    };
    
    send_data(server->shm, true, &hdr, sizeof(Hdr));
    send_data(server->shm, true, output, hdr.nBytes);
    
    // free(output);
    // fprintf(stderr, "Server: Result sent\n");
    return 0;
}


void server_loop(ChatDb *chatDb, Shm *shm) {
    if (!chatDb || !shm) return;

    Server server = { .chatDb = chatDb, .shm = shm };
    // fprintf(stderr, "Server: Starting server loop\n");

    while (1) {
        // Read command header
        Hdr cmdHdr;
        // fprintf(stderr, "Server: Waiting for next command\n");
        receive_data(shm, true, &cmdHdr, sizeof(Hdr));
        // fprintf(stderr, "Server: Received command type=%d\n", cmdHdr.cmdType);

        if (cmdHdr.cmdType == END_CMD) {
            // fprintf(stderr, "Server: Received END command, breaking loop\n");
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
                // fprintf(stderr, "Server: Unknown command type: %d\n", cmdHdr.cmdType);
                send_server_response(chatDb, SYS_ERR_STATUS, 
                                   "Unknown command type", shm);
                break;
        }
    }
    
    // fprintf(stderr, "Server: Server loop ended\n");
}