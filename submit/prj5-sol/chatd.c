#include "common.h"
#include "server.h"

#include <chat-cmd.h>
#include <chat-db.h>
#include <errors.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


static pid_t
make_daemon(int sockFd, const char *dbPath)
{
  pid_t pid;
  if ((pid = fork()) < 0) {
    return -1;
  }
  else if (pid != 0) { /* parent */
    close(sockFd);
    return pid;
  }
  else { /* child becomes daemon */
    setsid();
    do_serve(sockFd, dbPath); /** provide daemon service */
    assert(0); /** should never exit */
  }
}

/** return a listening streaming server socket bound to INADDR_ANY:port.
 *  Terminate program on error.
 */
static int
openServerSocket(int port)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) fatal("Cannot open socket");

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        fatal("Cannot bind socket");

    if (listen(sockfd, 10) < 0)
        fatal("Cannot listen on socket");

    return sockfd;
  //TODO
  // return -1;
}

/** Invoked with two arguments:
 *
 *    PORT: the port # on which the server will listen.
 *
 *    DBFILE_PATH: path to the sqlite file.
 *
 *  The server may use `stderr` for "logging", but all such logging
 *  *must* be turned off before submission.
 */
int
main(int argc, const char *argv[])
{
  if (argc != 3) fatal("usage: %s PORT DBFILE_PATH", argv[0]);
  const char *portStr = argv[1];
  const char *dbPath = argv[2];

  char *p;
  int port = strtol(portStr, &p, 10);
  if (*p != '\0' || port < 1024 || port > 65535) {
    fatal("invalid port \"%s\"; must be int in [1024, 65535]", portStr);
  }

  // verify open of chat-db
  MakeChatDbResult result;
  if (make_chat_db(dbPath, &result) != 0) {
      fatal("cannot open db at %s: %s", dbPath, result.err);
  }
  ChatDb *chatDb = result.chatDb; // Store the initialized database handle
  free_chat_db(result.chatDb);

  int sockFd = openServerSocket(port);

  pid_t daemonPid = make_daemon(sockFd, dbPath);
  if (daemonPid < 0) {
    fatal("cannot create daemon");
    close(sockFd);
    return 1;
  }
  fprintf(stdout, "%lld\n", (long long)daemonPid);
  return 0;
}
