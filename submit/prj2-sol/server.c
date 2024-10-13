#include "server.h"

#include "common.h"

#include <chat-cmd.h>
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

//TODO: provide context for Server
typedef struct {
    int in_fd;
    int out_fd;
    ChatDb *db;
} Server;


// void do_server(){
//     printf('sdwdasdwads');
// }

void
do_server(int from_client_fd, int to_client_fd, const char *dbPath)
{   
        // printf("problem at making chat_db");
    Server server = { from_client_fd, to_client_fd,NULL};
    MakeChatDbResult result;
    if (make_chat_db(dbPath, &result) != 0) {
        // Handle error
        error(result.err);
        // write(to_client_fd, result.err, strlen(result.err));
        return;
    }
    
    server.db = result.chatDb;
    printf("problem at making %s",result.chatDb);
    while (1) {
        ChatCmd cmd;
        if (read(from_client_fd, &cmd, sizeof(ChatCmd)) <= 0) {
            break; // Error or EOF
        }

        switch (cmd.type) {
            case ADD_CMD:
                handle_add_cmd(&server, &cmd.add);
                break;
            case QUERY_CMD:
                handle_query_cmd(&server, &cmd.query);
                break;
            case END_CMD:
                free_chat_db(server.db);
                return;
            default:
                // Handle error
                break;
        }
    }

    // free_chat_db(server.db);

        // Handle ADD, QUERY, etc. commands based on the content of the buffer
        // For example: if (strncmp(buffer, "ADD", 3) == 0) { /* Handle ADD */ }
        
        // TODO: Process command and interact with the database
    

    // Clean up
    close(from_client_fd);
    close(to_client_fd);
    // free_chat_db(ctx.db); // Free the SQLite database connection

}

void handle_add_cmd(Server server, ChatCmd* cmd){
    dprintf(server.out_fd, " We are in add cmd");

}

void handle_query_cmd(Server server, ChatCmd* cmd){
    dprintf(server.out_fd, " We are in querry cmd");
}