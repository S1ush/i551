#ifndef SERVER_H_
#define SERVER_H_

#include "common.h"
#include "server-loop.h"
#include <stdio.h>

void do_server(const char *dbPath, Shm *shm);


#endif //#ifndef SERVER_H_
