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
  return NULL;  //TODO
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
  //TODO
}

/** return server's PID */
pid_t
chat_server_pid(const Chat *chat)
{
  //TODO
  return 0;
}
