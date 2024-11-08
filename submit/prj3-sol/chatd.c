#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "utils.h"
#include "chat-db.h"
#include "server-loop.h"

// Make process a daemon

static int make_daemon(void) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) exit(0);  // Parent exits
    
    // Create new session
    if (setsid() < 0) return -1;
    
    // Second fork to ensure we're not session leader
    pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) exit(0);
    
    return 0;
}

static void handle_client(const char *db_path, pid_t client_pid) {
    char read_fifo[MAX_FIFO_PATH_LEN], write_fifo[MAX_FIFO_PATH_LEN];
    make_client_read_fifo_path(write_fifo, client_pid);
    make_client_write_fifo_path(read_fifo, client_pid);
    
    // Server should open the client's write FIFO first for reading (this is the FIFO
    // the client writes to, so server reads from it)
    int client_to_server = open(read_fifo, O_RDONLY);
    // fprintf(stderr, "server: opened for reading from %s\n", read_fifo);
    
    if (client_to_server < 0) {
        perror("server: cannot open read FIFO");
        exit(1);
    }

    // Then open the client's read FIFO for writing (this is the FIFO the client
    // reads from, so server writes to it)
    int server_to_client = open(write_fifo, O_WRONLY);
        if (server_to_client < 0) {
        close(client_to_server);
        perror("server: cannot open write FIFO");
        exit(1);
    }
    
    // Set up pipe arrays as expected by do_server

    FILE *serverIn = fdopen(client_to_server, "r");
    if (!serverIn) {
          fprintf(stderr, "fdopen: opened for reading to %s\n", write_fifo);
       
    }

    FILE *serverOut = fdopen(server_to_client, "w");
    if (!serverOut) {
          fprintf(stderr, "fdopen: opened for writing to %s\n", write_fifo);
        // fclose(serverIn);  // This also closes read_fd
        // close(write_fd);
        // errorf(err, "err SYS_ERR: cannot fdopen write FIFO");
      
        // return NULL;
    }
    
    // Call do_server with the FIFOs arranged as pipes
    // int result = do_server(db_path, inPipe, outPipe);
    ChatDb *chatDb = NULL;
    MakeChatDbResult result;
     const char *errMsg = NULL;
        if (make_chat_db(db_path, &result) != 0) {
            errMsg = result.err;
            fprintf(stderr,"handle_client : %s\n", errMsg);
        }
    chatDb = result.chatDb;
    server_loop(chatDb, serverOut, serverIn);
    
    // Cleanup

    close(client_to_server);
    close(server_to_client);
    exit(0);
}



int main(int argc, char *argv[]) {
   if (argc != 3) {
        fprintf(stderr, "usage: %s SERVER_DIR DBFILE_PATH\n", argv[0]);
        exit(1);
    }
    
    // Change to server directory
    if (chdir(argv[1]) < 0) {
        perror("chdir failed");
        exit(1);
    }
    
    // Verify database can be opened
    MakeChatDbResult result;
    if ((make_chat_db(argv[2], &result) != 0)) {
        fprintf(stderr, "Cannot open database\n");
        exit(1);
    }
    free_chat_db(result.chatDb);
    
    // Create and open well-known FIFO
    if (create_fifo(WELL_KNOWN_FIFO) < 0) {
        perror("Cannot create server FIFO");
        exit(1);
    }
    
    // Become a daemon
    if (make_daemon() < 0) {
        perror("Cannot create daemon");
        exit(1);
    }
    
    // Print daemon PID
    printf("%d\n", getpid());
    fflush(stdout);  // Ensure PID is printed before daemon detaches
    
    // Open well-known FIFO for reading. We use O_RDWR to prevent EOF when clients disconnect
    int server_fifo = open(WELL_KNOWN_FIFO, O_RDWR);
    if (server_fifo < 0) {
        perror("Cannot open server FIFO");
        exit(1);
    }
    
    // Main server loop
    while (1) {
        pid_t client_pid;
        ssize_t n = read(server_fifo, &client_pid, sizeof(client_pid));
        
        if (n != sizeof(client_pid)) {
            if (n < 0) perror("read from well-known FIFO failed");
            continue;
        }
        
        // fprintf(stderr, "server: received connection request from client PID %d\n", client_pid);
        
        // Double fork to create worker
        pid_t pid = fork();
        if (pid == 0) {
            pid = fork();
            if (pid == 0) {
                // Grandchild (worker)
                close(server_fifo);
                handle_client(argv[2], client_pid);
            }
            exit(0);  // Child exits
        }
        
        // Parent continues serving
        if (pid > 0) {
            waitpid(pid, NULL, 0);  // Reap child
        }
    }
    
    return 0;
}