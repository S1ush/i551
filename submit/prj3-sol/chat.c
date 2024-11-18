#include "chat.h"
#include "client.h"
#include "common.h"
#include "server-loop.h"
#include "server.h"

#include <chat-cmd.h>
#include <errors.h>
#include <errno.h>
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <unistd.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

// define ADT
struct _Chat {
  Client client;
  pid_t clientPid;
};

/**************************** Private Routines *************************/

static Chat *do_client(pid_t clientPid, FILE *out, FILE *err, int inPipe[2], int outPipe[2])
{
    FILE *serverIn = NULL;
    FILE *serverOut = NULL;
    Chat *chat = NULL;
    const char *errMsg;

    if (close(inPipe[1]) < 0) {
        errMsg = "client cannot close inPipe[1]";
        goto CLEANUP;
    }
    if (close(outPipe[0]) < 0) {
        errMsg = "client cannot close outPipe[0]";
        goto CLEANUP;
    }

    serverIn = fdopen(inPipe[0], "r");
    if (!serverIn) {
        errMsg = "client cannot fdopen(inPipe[0])";
        goto CLEANUP;
    }

    serverOut = fdopen(outPipe[1], "w");
    if (!serverOut) {
        errMsg = "client cannot fdopen(outPipe[1])";
        goto CLEANUP;
    }

    chat = malloc(sizeof(Chat));
    if (!chat) {
        errMsg = "cannot malloc Chat";
        goto CLEANUP;
    }

    *chat = (Chat) {
        .client = (Client) {
            .out = out,
            .err = err,
            .serverIn = serverIn,
            .serverOut = serverOut,
        },
        .clientPid = clientPid,
    };
    return chat;

CLEANUP:
    if (serverIn) fclose(serverIn);
    if (serverOut) fclose(serverOut);
    if (chat) free(chat);
    errorf(err, "err SYS_ERR: %s", errMsg);
    return NULL;
}

/******************************* Public API ****************************/

/** Return a new Chat object which creates a server process which uses
 *  the sqlite database located at dbPath.  All commands must be sent
 *  by this client process to the server and handled by the server
 *  using the database. All IPC must use anonymous pipes.  The
 *  returned object should encapsulate all the state needed to
 *  implement the following API.
 *
 *  The client process can use out for writing success output for
 *  commands where each output must start with a line containing "ok".
 *
 *  The client process should use err for writing error message
 *  lines (must start with "err ERR_CODE: " where ERR_CODE is
 *  BAD_ROOM/BAD_TOPIC for unknown room/topic, or SYS_ERR for non-user
 *  errors).
 *
 *  The server should not use the in or out streams.  It may use
 *  stderr for "logging", but all such logging must be turned off
 *  before submission.
 *
 *  If errors are encountered, then this function should return NULL.
 */
// Chat *make_chat(const char *serverDir, FILE *out, FILE *err)
// {
//     if (chdir(serverDir) < 0) {
//         errorf(err, "err SYS_ERR: cannot change to directory %s: %s", 
//                serverDir, strerror(errno));
//         return NULL;
//     }
//     pid_t clientPid = getpid();
    
//     // Create private FIFOs for this client
//     char read_fifo[MAX_FIFO_PATH_LEN], write_fifo[MAX_FIFO_PATH_LEN];
//     make_client_read_fifo_path(write_fifo, clientPid);
//     make_client_write_fifo_path(read_fifo, clientPid);
    
//     if (create_fifo(read_fifo) < 0 || create_fifo(write_fifo) < 0) {
//         errorf(err, "err SYS_ERR: cannot create client FIFOs: %s", 
//                strerror(errno));
//         cleanup_client_fifos(clientPid);
//         return NULL;
//     }

//     // Open well-known FIFO with O_RDWR
//     int server_fifo = open_fifo(WELL_KNOWN_FIFO, O_RDWR);
//     if (server_fifo < 0) {
//         errorf(err, "err SYS_ERR: cannot connect to server: %s", 
//                strerror(errno));
//         cleanup_client_fifos(clientPid);
//         return NULL;
//     }

//     // Send our PID to server
//     fprintf(out,"clientpid = %d",clientPid);
//     if (write(server_fifo, &clientPid, sizeof(clientPid)) != sizeof(clientPid)) {
//         errorf(err, "err SYS_ERR: cannot send request to server: %s", 
//                strerror(errno));
//         close(server_fifo);
//         cleanup_client_fifos(clientPid);
//         return NULL;
//     }
//     close(server_fifo);

//     // Set up pipe arrays as expected by do_client
//     int inPipe[2] = { -1, -1 };
//     int outPipe[2] = { -1, -1 };



//     int write_fd = open(write_fifo, O_WRONLY);
//     fprintf(stderr,"client : write %s\n", write_fifo);
//     if (write_fd < 0) {
//         errorf(err, "err SYS_ERR: cannot open write FIFO: %s\n", strerror(errno));
//         cleanup_client_fifos(clientPid);
//         return NULL;
//     }

//     // Then open read FIFO (server will open for writing)
//     int read_fd = open(read_fifo, O_RDONLY);
//     fprintf(stderr,"client : read %s\n", read_fifo);
//     if (read_fd < 0) {
//         close(write_fd);
//         errorf(err, "err SYS_ERR: cannot open read FIFO: %s", strerror(errno));
//         cleanup_client_fifos(clientPid);
//         return NULL;
//     }


//     // Open both FIFOs with O_RDWR to prevent EOF conditions
//     // int read_fd = open_fifo(read_fifo, O_RDWR);
//     //   fprintf(stderr,"\nreading to = %s",read_fifo);
//     // int write_fd = open_fifo(write_fifo, O_RDWR);
//     //   fprintf(stderr,"\nwriting to = %s",write_fifo);

//     // if (read_fd < 0 || write_fd < 0) {
//     //     if (read_fd >= 0) close(read_fd);
//     //     if (write_fd >= 0) close(write_fd);
//     //     errorf(err, "err SYS_ERR: cannot open FIFOs: %s", strerror(errno));
//     //     cleanup_client_fifos(clientPid);
//     //     return NULL;
//     // }

//     // Set up the pipe arrays with the FIFOs
//     // Close the ends we don't need after setting up both FIFOs
//     // inPipe[0] = read_fd;   // Keep read end for reading from server
//     inPipe[1] = dup(read_fd);  // Will be closed by do_client
//     outPipe[0] = dup(write_fd);  // Will be closed by do_client
//     // outPipe[1] = write_fd;  // Keep write end for writing to server
//     inPipe[0] = read_fd;   // Read end for reading from server
//     // inPipe[1] = -1;        // Write end not needed
//     // outPipe[0] = -1;       // Read end not needed
//     outPipe[1] = write_fd; // Write end for writing to server
//     Chat *chat = NULL;
//     FILE *serverIn = NULL;
//     FILE *serverOut = NULL;
//     const char *errMsg;
//        serverIn = fdopen(read_fd, "r");
//     if (!serverIn) {
//         errMsg = "client cannot fdopen(inPipe[0])";
   
//     }

//     serverOut = fdopen(write_fd, "w");
//     if (!serverOut) {
//         errMsg = "client cannot fdopen(outPipe[1])";
      
//     }

//      chat = malloc(sizeof(Chat));
//     if (!chat) {
//         fprintf(stderr,"chat error" );
//     }

//          *chat = (Chat) {
//         .client = (Client) {
//             .out = out,
//             .err = err,
//             .serverIn = serverIn,
//             .serverOut = serverOut,
//         },
//         .clientPid = clientPid,
//     };

//     // Call do_client with our clientPid for proper cleanup
//     // Chat *chat = do_client(clientPid, out, err, inPipe, outPipe);
//     // if (chat == NULL) {
//     //     // do_client will close inPipe[0] and outPipe[1] if it fails
//     //     cleanup_client_fifos(clientPid);
//     //     return NULL;
//     // }
//     fprintf(out, "checking where is breaking \n");

//     return chat;
// }

Chat *make_chat(const char *serverDir, FILE *out, FILE *err)
{
    if (chdir(serverDir) < 0) {
        errorf(err, "err SYS_ERR: cannot change to directory %s: %s", 
               serverDir, strerror(errno));
        return NULL;
    }
    pid_t clientPid = getpid();
    
    // Create private FIFOs for this client
    char read_fifo[MAX_FIFO_PATH_LEN], write_fifo[MAX_FIFO_PATH_LEN];
    // Fixed: These were swapped - now corrected
    make_client_read_fifo_path(read_fifo, clientPid);
    //    fprintf(stderr, "client: opened for read_fifo to %s\n", read_fifo);
    make_client_write_fifo_path(write_fifo, clientPid);
    //    fprintf(stderr, "client: opened for write_fifo to %s\n", write_fifo);
    
    
    if (create_fifo(read_fifo) < 0 || create_fifo(write_fifo) < 0) {
        errorf(err, "err SYS_ERR: cannot create client FIFOs: %s", 
               strerror(errno));
        cleanup_client_fifos(clientPid);
        return NULL;
    }

    // Open well-known FIFO - only need WRONLY for client
    int server_fifo = open(WELL_KNOWN_FIFO, O_WRONLY);
    if (server_fifo < 0) {
        errorf(err, "err SYS_ERR: cannot connect to server: %s", 
               strerror(errno));
        cleanup_client_fifos(clientPid);
        return NULL;
    }

    // Send our PID to server
    if (write(server_fifo, &clientPid, sizeof(clientPid)) != sizeof(clientPid)) {
        errorf(err, "err SYS_ERR: cannot send request to server: %s", 
               strerror(errno));
        close(server_fifo);
        cleanup_client_fifos(clientPid);
        return NULL;
    }
    close(server_fifo);

    // First open write FIFO
    int write_fd = open(write_fifo, O_RDWR);
    // fprintf(stderr, "client: opened for writing to %s\n", write_fifo);
    if (write_fd < 0) {
        errorf(err, "err SYS_ERR: cannot open write FIFO: %s", strerror(errno));
        cleanup_client_fifos(clientPid);
        return NULL;
    }

    // Then open read FIFO
    int read_fd = open(read_fifo, O_RDWR);
    // fprintf(stderr, "client: opened for reading from %s\n", read_fifo);
    if (read_fd < 0) {
        close(write_fd);
        errorf(err, "err SYS_ERR: cannot open read FIFO: %s", strerror(errno));
        cleanup_client_fifos(clientPid);
        return NULL;
    }

    // Create FILE* streams from the file descriptors
    FILE *serverIn = fdopen(read_fd, "r");
    if (!serverIn) {
        close(read_fd);
        close(write_fd);
        errorf(err, "err SYS_ERR: cannot fdopen read FIFO");
        cleanup_client_fifos(clientPid);
        return NULL;
    }

    FILE *serverOut = fdopen(write_fd, "w");
    if (!serverOut) {
        fclose(serverIn);  // This also closes read_fd
        close(write_fd);
        errorf(err, "err SYS_ERR: cannot fdopen write FIFO");
        cleanup_client_fifos(clientPid);
        return NULL;
    }

    // Create and initialize chat structure
    Chat *chat = malloc(sizeof(Chat));
    if (!chat) {
        fclose(serverIn);   // These fclose calls will close
        fclose(serverOut);  // the underlying file descriptors too
        errorf(err, "err SYS_ERR: cannot allocate Chat");
        cleanup_client_fifos(clientPid);
        return NULL;
    }

    *chat = (Chat) {
        .client = (Client) {
            .out = out,
            .err = err,
            .serverIn = serverIn,
            .serverOut = serverOut,
        },
        .clientPid = clientPid,
    };

    return chat;
}



/** free all resources like memory, FILE's and descriptors used by chat
 *  All resources must be freed even after user errors have been detected.
 *  It is okay if resources are not freed after system errors.
 */
void free_chat(Chat *chat)
{
    if (chat) {
        fclose(chat->client.serverOut);
        fclose(chat->client.serverIn);
        cleanup_client_fifos(chat->clientPid);
        free(chat);
    }
}
/** perform cmd using chat, with the client writing response to chat's
 *  out/err streams.  It can be assumed that cmd is free of user
 *  errors except for unknown room/topic for QUERY commands.
 *
 *  If the command is an END_CMD command, then ensure that the server
 *  process is shut down cleanly.
 */
void
do_chat_cmd(Chat *chat, const ChatCmd *cmd)
{
  do_client_cmd(&chat->client, cmd);
}

/** return server's PID */
pid_t
chat_server_pid(const Chat *chat)
{
  return chat->clientPid;
}