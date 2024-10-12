#include "chat.h"
#include "common.h"
#include "server.h"

#include <chat-cmd.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>


/***************************** Declarations ****************************/

// fill out ADT structure
struct _Chat {
    int client_to_server[2];  // Pipe for client to server communication
    int server_to_client[2];  // Pipe for server to client communication
    pid_t server_pid;            // PID of the server process
    FILE *out;                   // Output stream for success responses
    FILE *err;                   // Error stream for error messages
};

// prefix for all error messages
#define ERROR "err "

// line to indicate a successful response
#define OKAY "ok\n"

/**************************** Private Routines *************************/

// TODO: static routines local to this file

/******************************* Public API ****************************/

/** Return a new Chat object which creates a server process which uses
 *  the sqlite database located at dbPath.  All commands must be sent
 *  by this client process to the server and handled by the server
 *  using the database. All IPC must use anonymous pipes.  The
 *  returned object should encapsulate all the state needed to
 *  implement the following API.
 *
 *  The client process can use `out` for writing success output for
 *  commands where each output must start with a line containing "ok".
 *
 *  The client process should use `err` for writing error message
 *  lines (must start with "err ERR_CODE: " where ERR_CODE is
 *  BAD_ROOM/BAD_TOPIC for unknown room/topic, or SYS_ERR for non-user
 *  errors).
 *
 *  The server should not use the `in` or `out` streams.  It may use
 *  `stderr` for "logging", but all such logging *must* be turned off
 *  before submission.
 *
 *  If errors are encountered, then this function should return NULL.
 */
Chat *
make_chat(const char *dbPath, FILE *out, FILE *err)
{  
  Chat *chat = malloc(sizeof(Chat));
    if (!chat) {
        fprintf(err, "%serror allocating memory\n", ERROR);
        return NULL;
    }
      // Initialize pipes for IPC
    if (pipe(chat->client_to_server) == -1 || pipe(chat->server_to_client) == -1) {
        fprintf(err, "%spipe creation failed\n", ERROR);
        free(chat);
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed
        fprintf(err, "%sfork failed\n", ERROR);
        close(chat->client_to_server[1]);
        close(chat->client_to_server[0]);
        close(chat->server_to_client[0]);
        close(chat->server_to_client[1]);
        free(chat);
        return NULL;
    } else if (pid == 0) {
        // In the child (server) process
        close(chat->client_to_server[1]);  // Close write end of client to server pipe in the server
        close(chat->server_to_client[0]);  // Close read end of server to client pipe in the server
        do_server(chat->client_to_server[0], chat->server_to_client[1], dbPath);
        exit(0);  // Server process should exit after handling
    }

    chat->server_pid = pid;
    close(chat->client_to_server[0]);  // Close read end of client to server pipe in the client
    close(chat->server_to_client[1]);  // Close write end of server to client pipe in the client
    chat->out = out;
    chat->err = err;
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
}

/** perform cmd using chat, writing response to chat's out/err
 *  streams.  It can be assumed that cmd is free of user errors except
 *  for unknown room/topic for QUERY commands.
 *
 *  If the command is an END_CMD command, then ensure that the server
 *  process is shut down cleanly.
 */
void
do_chat_cmd(Chat *chat, const ChatCmd *cmd)
{
  fprintf(chat->out, "Checking What is in here : %d ", cmd->type );
  //TODO
}

/** return server's PID */
pid_t
chat_server_pid(const Chat *chat)
{
  //TODO
  return 0;
}
