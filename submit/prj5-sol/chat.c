#include "chat.h"
#include "common.h"

#include <chat-cmd.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include <pthread.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

struct _Chat {
  //TODO
    int sockFd;
    FILE *out;
    FILE *err;
};

// prefix for all error messages
#define ERROR "err "

// line to indicate a successful response
#define OKAY "ok\n"


//TODO: add auxiliary functions
/** perform cmd using chat, writing response to chat's out/err
 *  streams.  It can be assumed that cmd is free of user errors except
 *  for unknown room/topic for QUERY commands.
 *
 *  If the command is an END_CMD command, then ensure that the server
 *  process is shut down cleanly.
 */
// Auxiliary function to send a command to the server
static void send_cmd(Chat *chat, const ChatCmd *cmd) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%d %s %s %s\n",
                      cmd->type, cmd->add.user, cmd->add.room, cmd->add.message);
    // printf("DEBUG: Sending command to server: type=%d, user=%s, room=%s, message=%s\n",
       cmd->type, cmd->add.user, cmd->add.room, cmd->add.message);
    write(chat->sockFd, buffer, strlen(buffer));
    }

// Thread to handle incoming messages from the server
static void *read_from_server(void *arg) {
    Chat *chat = (Chat *)arg;
    char buffer[1024];
    while (true) {
        int n = read(chat->sockFd, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;
        buffer[n] = '\0';
        fprintf(chat->out, "%s", buffer);
        fflush(chat->out);
    }
    return NULL;
}


void
do_chat_cmd(Chat *chat, const ChatCmd *cmd)
{
  //TODO
   if (cmd->type == END_CMD) {
        send_cmd(chat, cmd);
        close(chat->sockFd);
        return;
    }
    send_cmd(chat, cmd);
}

/** Return a new Chat object which contacts the chatd server runnning
 *  on params->host and params->port on behalf of user params->user in
 *  room params->room.  All commands must be sent by this client
 *  process to the server and handled using its database. All IPC must
 *  use the network.  The returned object should encapsulate all the
 *  state needed to implement the following API.
 *
 *  The client process must use params->out for writing success output for
 *  commands where each output must start with a line containing "ok".
 *
 *  The client process must use params->err for writing error message
 *  lines. Each line must start with "err ERR_CODE: " where ERR_CODE is
 *  as in your previous project for user errors or SYS_ERR for non-user
 *  errors.

 *  [Note that since a `ChatCmd` is guaranteed to be syntactically
 *  valid, the only user errors which the program will need to detect
 *  will be `BAD_ROOM`/`BAD_TOPIC` for unknown room or topic.  This
 *  will have to be done by the worker process which will then return
 *  an error response to the client for output.]
 *
 *  If errors are encountered, then this function should return NULL.
 */
Chat *
make_chat(const ChatParams *params)
{
  //TODO
  // return NULL;
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) {
        // perror("DEBUG: Error creating socket");
        fprintf(params->err, "err Failed to create socket\n");
        return NULL;
    // } else {
    //     printf("DEBUG: Socket created successfully\n");
    }

    struct sockaddr_in serv_addr;
    struct hostent *server = gethostbyname(params->host);
    if (!server) {
        fprintf(params->err, "err Host not found: %s\n", params->host);
        close(sockFd);
        return NULL;
    // } else {
        // printf("DEBUG: Resolved host %s\n", params->host);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(params->port);

    if (connect(sockFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        // perror("DEBUG: Error connecting to server");
        fprintf(params->err, "err Failed to connect to server\n");
        close(sockFd);
        return NULL;
    // } else {
        // printf("DEBUG: Connected to server at %s:%d\n", params->host, params->port);
    }

    Chat *chat = malloc(sizeof(Chat));
    if (!chat) {
        fprintf(params->err, ERROR "Failed to allocate memory\n");
        close(sockFd);
        return NULL;
    }
    chat->sockFd = sockFd;
    chat->out = params->out;
    chat->err = params->err;

    pthread_t reader_thread;
    if (pthread_create(&reader_thread, NULL, read_from_server, chat) != 0) {
        fprintf(params->err, ERROR "Failed to create thread\n");
        free(chat);
        close(sockFd);
        return NULL;
    }
    pthread_detach(reader_thread);

    return chat;
}

/** free all resources like memory, FILE's and descriptors used by chat
 *  All resources must be freed even after user errors have been detected.
 *  It is okay if resources are not freed after system errors.
 */
void
free_chat(Chat *chat)
{
  //TODO
  if (chat) {
        close(chat->sockFd);
        free(chat);
    }
}
