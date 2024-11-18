#ifndef SERVER_H_
#define SERVER_H_

#include "common.h"

#include <stdio.h>

void do_server(const char *dbPath, Shm *shm);
// void handle_chat_cmd(Chat *chatDb, const ChatCmd *cmd); 
int query_iterator(const ChatInfo *result, void *ctx);
void process_command(Shm *shm, const ChatCmd *cmd, ChatDb *db);

#endif //#ifndef SERVER_H_
