#include "client.h"
#include "common.h"
#include "chat.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void send_add_req(Client *client, const AddCmd *cmd) {
    // fprintf(stderr, "Client: Starting ADD command\n");
    // fprintf(stderr, "Client: User='%s', Room='%s', Message='%s'\n", 
            // cmd->user, cmd->room, cmd->message);
    // fprintf(stderr, "Client: Topics count=%zu\n", cmd->nTopics);

    // Calculate data size
    size_t nBytes = 0;
    nBytes += (strlen(cmd->user) + 1);
    nBytes += (strlen(cmd->room) + 1);
    nBytes += (strlen(cmd->message) + 1);
    for (int i = 0; i < cmd->nTopics; i++) {
        // fprintf(stderr, "Client: Topic[%d]='%s'\n", i, cmd->topics[i]);
        nBytes += strlen(cmd->topics[i]) + 1;
    }

    // First send header
    Hdr hdr = {
        .hdrType = 0,  // client
        .cmdType = ADD_CMD,
        .count = -1,
        .nTopics = cmd->nTopics,
        .nBytes = nBytes
    };

    // fprintf(stderr, "Client: Sending header: type=%d cmd=%d topics=%zu bytes=%d\n",
            // hdr.hdrType, hdr.cmdType, hdr.nTopics, hdr.nBytes);
    send_data(client->shm, false, &hdr, sizeof(Hdr));

    // Then send data
    char *dataBuffer = malloc(nBytes);
    if (!dataBuffer) {
        fprintf(stderr, "Client: Failed to allocate memory\n");
        return;
    }

    char *dataPtr = dataBuffer;
    memcpy(dataPtr, cmd->user, strlen(cmd->user) + 1);
    dataPtr += strlen(cmd->user) + 1;
    
    memcpy(dataPtr, cmd->room, strlen(cmd->room) + 1);
    dataPtr += strlen(cmd->room) + 1;
    
    memcpy(dataPtr, cmd->message, strlen(cmd->message) + 1);
    dataPtr += strlen(cmd->message) + 1;
    
    for (int i = 0; i < cmd->nTopics; i++) {
        memcpy(dataPtr, cmd->topics[i], strlen(cmd->topics[i]) + 1);
        dataPtr += strlen(cmd->topics[i]) + 1;
    }

    // fprintf(stderr, "Client: Sending data (size=%zu)\n", nBytes);
    send_data(client->shm, false, dataBuffer, nBytes);
    free(dataBuffer);

    // fprintf(stderr, "Client: Finished sending ADD data\n");
}

static void receive_res(Client *client) {
    FILE *out = client->out;
    FILE *err = client->err;
    bool didOk = false;

    // fprintf(stderr, "Client: Starting to receive response\n");

    // Read header
    Hdr hdr;
    receive_data(client->shm, false, &hdr, sizeof(Hdr));
    
    // fprintf(stderr, "Client: Received header - status=%d, nBytes=%d\n", 
            // hdr.status, hdr.nBytes);

    // Read data if present
    char *data = NULL;
    if (hdr.nBytes > 0) {
        data = malloc(hdr.nBytes + 1);
        if (data) {
            // fprintf(stderr, "Client: Reading %d bytes of data\n", hdr.nBytes);
            receive_data(client->shm, false, data, hdr.nBytes);
            data[hdr.nBytes] = '\0';
            // fprintf(stderr, "Client: Received data='%s'\n", data);
        }
    }

    // Process response
    if (hdr.status != 0) {
        const char *errStatus = 
            (const char *[]){ "", "SYS_ERR: ", "FATAL_ERR: " }[hdr.status - 1];
        fprintf(err, ERROR "%s%s\n", errStatus, data ? data : "");
        fflush(err);
    } else {
        fprintf(out, OKAY);
        fflush(out);
        if (data) {
            fprintf(out, "%s", data);
            fflush(out);
        }
    }

    if (data) free(data);
    // fprintf(stderr, "Client: Finished receiving response\n");
}

static void send_query_req(Client *client, const QueryCmd *cmd) {
    // fprintf(stderr, "Client: Starting QUERY command\n");
    
    size_t nBytes = 0;
    nBytes += (strlen(cmd->room) + 1);
    for (int i = 0; i < cmd->nTopics; i++) {
        nBytes += strlen(cmd->topics[i]) + 1;
    }

    // First send header
    Hdr hdr = {
        .hdrType = 0,  // client
        .cmdType = QUERY_CMD,
        .count = cmd->count,
        .nTopics = cmd->nTopics,
        .nBytes = nBytes
    };

    // fprintf(stderr, "Client: Sending header with nBytes=%zu\n", nBytes);
    send_data(client->shm, false, &hdr, sizeof(Hdr));

    // Then send data
    char *dataBuffer = malloc(nBytes);
    if (!dataBuffer) {
        fprintf(stderr, "Client: Failed to allocate memory\n");
        return;
    }

    char *dataPtr = dataBuffer;
    memcpy(dataPtr, cmd->room, strlen(cmd->room) + 1);
    dataPtr += strlen(cmd->room) + 1;
    
    for (int i = 0; i < cmd->nTopics; i++) {
        memcpy(dataPtr, cmd->topics[i], strlen(cmd->topics[i]) + 1);
        dataPtr += strlen(cmd->topics[i]) + 1;
    }

    // fprintf(stderr, "Client: Sending query data (size=%zu)\n", nBytes);
    send_data(client->shm, false, dataBuffer, nBytes);
    free(dataBuffer);

    // fprintf(stderr, "Client: Finished sending QUERY request\n");
}

void do_client_cmd(Client *client, const ChatCmd *cmd) {
    // fprintf(stderr, "Client: Processing command type=%d\n", cmd->type);
        switch (cmd->type) {
            case ADD_CMD:
                send_add_req(client, &cmd->add);
                receive_res(client);
                break;
            case QUERY_CMD:
                send_query_req(client, &cmd->query);
                receive_res(client);
                break;
            case END_CMD: {
                // fprintf(stderr, "Client: Sending END command\n");
                Hdr hdr = {
                    .hdrType = 0,  // client
                    .cmdType = END_CMD,
                    .nBytes = 0
                };
                send_data(client->shm, false, &hdr, sizeof(Hdr));
                // fprintf(stderr, "Client: END command sent\n");
                break;
            }
            default:
                // fprintf(stderr, "Client: Unknown command type\n");
                assert(0);
        }
    
    // fprintf(stderr, "Client: Finished processing command\n");
}