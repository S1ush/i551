#ifndef COMMON_H_
#define COMMON_H_

#include <chat-cmd.h>

#include <stdio.h>

// TODO declarations common between server and client
int write_message(int sockFd, const char *message);
int send_structured_message(int sockFd, int status, const char *user, const char *room, const char *message);


#endif //#ifndef COMMON_H_
