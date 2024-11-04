#include "utils.h"

#include <chat-cmd.h>
#include <chat-db.h>
#include <errors.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/file.h> 




// Creates the daemon process using double-fork technique
static pid_t make_daemon(void) {
    pid_t pid = fork();
    if (pid < 0) fatal("first fork failed");
    if (pid > 0) exit(0);  // parent exits
    
    // First child continues
    if (setsid() < 0) fatal("setsid failed");
    
    pid = fork();
    if (pid < 0) fatal("second fork failed");
    if (pid > 0) {
        // First child prints daemon PID and exits
        printf("chatd PID: %d\n", pid);
        exit(0);
    }
    
    // Daemon process continues
    // Close inherited file descriptors
    for (int fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++) {
        if (fd != 2) close(fd);  // Keep stderr for logging
    }
    
    umask(0);  // Reset file creation mask
    return getpid();
}

// Handles communication with a single client
static void handle_client(const char* db_path, pid_t client_pid) {
    // Open client-specific FIFOs based on client PID
    char to_client[32], from_client[32];
    snprintf(to_client, sizeof(to_client), "%d.0", client_pid);    // pid.0
    snprintf(from_client, sizeof(from_client), "%d.1", client_pid); // pid.1
    
    // Open FIFOs
    int to_fd = open(to_client, O_WRONLY);
    int from_fd = open(from_client, O_RDONLY);
    if (to_fd < 0 || from_fd < 0) {
        fprintf(stderr, "Failed to open client FIFOs\n");
        exit(1);
    }
    
    // Create streams
    FILE* to_client_stream = fdopen(to_fd, "w");
    FILE* from_client_stream = fdopen(from_fd, "r");
    if (!to_client_stream || !from_client_stream) {
        fprintf(stderr, "Failed to create client streams\n");
        exit(1);
    }
    
    // Open database
    ChatDb* db = chat_db_new(db_path);
    if (!db) {
        fprintf(stderr, "Failed to open database\n");
        exit(1);
    }
    
    // Process client commands using provided server_loop
    server_loop(db, from_client_stream, to_client_stream);
    
    // Cleanup
    chat_db_free(db);
    fclose(to_client_stream);
    fclose(from_client_stream);
    exit(0);
}

// Creates a worker process using double-fork
static void create_worker(const char* db_path, pid_t client_pid) {
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "First fork failed for worker\n");
        return;
    }
    
    if (pid == 0) {  // First child
        pid_t worker_pid = fork();
        if (worker_pid < 0) exit(1);
        
        if (worker_pid == 0) {  // Worker (grandchild)
            handle_client(db_path, client_pid);
        }
        exit(0);  // First child exits
    }
    
    // Parent (daemon) continues
    waitpid(pid, NULL, 0);  // Reap first child
}

// Main daemon loop
static void daemon_loop(const char* db_path) {
    // Create well-known FIFO
    const char* wk_fifo = "chat_server.fifo";
    mkfifo(wk_fifo, 0666);
    
    // Open well-known FIFO for reading and writing to avoid EOF
    int wk_fd = open(wk_fifo, O_RDWR);
    if (wk_fd < 0) {
        fprintf(stderr, "Cannot open well-known FIFO\n");
        exit(1);
    }
    
    FILE* wk_stream = fdopen(wk_fd, "r");
    if (!wk_stream) {
        fprintf(stderr, "Cannot create well-known FIFO stream\n");
        exit(1);
    }
    
    // Read client PIDs from well-known FIFO
    char buf[32];
    while (fgets(buf, sizeof(buf), wk_stream)) {
        pid_t client_pid = atoi(buf);
        if (client_pid > 0) {
            create_worker(db_path, client_pid);
        }
    }
    
    fclose(wk_stream);
}


/** Invoked with two arguments:
 *
 *    SERVER_DIR: the path to the directory in which the server should
 *    run and where all FIFOs will be created.
 *
 *    DBFILE_PATH: path to the sqlite file.  This must be relative to
 *    SERVER_DIR.
 *
 *  The server may use `stderr` for "logging", but all such logging
 *  *must* be turned off before submission.
 */
int
main(int argc, const char *argv[])
{

    if (argc != 3) {
        fprintf(stderr, "usage: %s SERVER_DIR DBFILE_PATH\n", argv[0]);
        exit(1);
    }
    
    // Change to server directory
    if (chdir(argv[1]) < 0) {
        fprintf(stderr, "Cannot change to directory %s\n", argv[1]);
        exit(1);
    }
    
    // Test database connection
    ChatDb* test_db = chat_db_new(argv[2]);
    if (!test_db) {
        fprintf(stderr, "Cannot open chat db %s\n", argv[2]);
        exit(1);
    }
    chat_db_free(test_db);
    
    // Create daemon and start server
    make_daemon();
    daemon_loop(argv[2]);
    
    return 0;
  
}
