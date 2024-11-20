#include "server.h"
#include "common.h"

#include <errors.h>
#include <chat-db.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

//TODO: add server code


void
do_server(const char *dbPath, Shm *shm)
{
  ChatDb *chatDb = NULL;
  MakeChatDbResult result;
  
  if (make_chat_db(dbPath, &result) != 0) {
    // Handle error
    exit(1);
  }
  
  chatDb = result.chatDb;
  server_loop(chatDb, shm);
  
  if (chatDb) free_chat_db(chatDb);
  exit(0);

}