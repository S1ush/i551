#ifndef SERVER_H_
#define SERVER_H_

#include <chat-db.h>

#include <stdio.h>

/** Using the chat-db specified by dbPath, accept connections
 *  on serverSockFd.  Connections must be processed
 *  concurrently, either by using a separate thread per
 *  connection or by using select (the former is recommended).
 */
void do_serve(int serverSockFd, const char *dbPath);
void broadcast_message(const char *room, const char *message, int senderFd);
void send_room_history(int clientFd, ChatDb *chatDb, const char *room);

#endif //#ifndef SERVER_H_
