#ifndef CLIENT_H_
#define CLIENT_H_

#include "chat.h"
#include "common.h"
#include <stdio.h>
#include <unistd.h>

// information about client
typedef struct {
  FILE *out;          // for writing success output
  FILE *err;          // for writing error messages
  Shm *shm;          // shared memory segment
} Client;

// prefix for all error messages
#define ERROR "err "
// line to indicate a successful response
#define OKAY "ok\n"

void do_client_cmd(Client *client, const ChatCmd *cmd);

#endif //#ifndef CLIENT_H_