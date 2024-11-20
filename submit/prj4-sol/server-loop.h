#ifndef SERVER_LOOP_H_
#define SERVER_LOOP_H_

#include <chat-db.h>

#include <stdio.h>

#include "common.h"
#include <chat-db.h>


void server_loop(ChatDb *chatDb, Shm *shm);
void end_server_response(ChatDb *chatDb, ServerStatus status,
                        const char *msg, FILE *out);

#endif //#ifndef SERVER_LOOP_H_
