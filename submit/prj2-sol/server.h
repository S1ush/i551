#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>

//server specific declarations

// void do_server();
void do_server(int in_fd, int out_fd, const char *dbPath);

#endif //#ifndef SERVER_H_
