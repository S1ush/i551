#include "server.h"
#include "common.h"
#include <chat-cmd.h>
#include <errors.h>
#include <chat-db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int in_fd;
    int out_fd;
    ChatDb *db;
} Server;

static int handle_add_cmd(Server *server, const AddCmd *cmd);
static int handle_query_cmd(Server *server, const QueryCmd *cmd);

void do_server(int from_client_fd, int to_client_fd, const char *dbPath)
{  
    Server server = { from_client_fd, to_client_fd, NULL };
    ChatCmd cmd;
    CmdType type ;
    AddCmd addCmd;
    char buffer[BUFSIZ];
    MakeChatDbResult result;
    int res = -1;


    if (make_chat_db(dbPath, &result) != 0) {
        const char *error_msg = "err SYS_ERR: Failed to create database\n";
        write(to_client_fd, error_msg, strlen(error_msg));
        return;
    }
    server.db = result.chatDb;

    while (1) {
        // printf("infinity\n");
        ssize_t read_bytes = read(from_client_fd, &type, sizeof(CmdType));
        if (read_bytes <= 0) {
            if (read_bytes == 0) break;  // EOF
            const char *error_msg = "err SYS_ERR: Failed to read command\n";
            write(to_client_fd, error_msg, strlen(error_msg));
            continue;
        }
        
        // printf("type : %d",type);
        // sleep(2);
        switch (type) {
            case ADD_CMD:     {
                ssize_t n = read(from_client_fd, buffer, sizeof(buffer) - 1);
                if(n <= 0){
                    printf("err reacving the data ");
                }   
                if (deserialize_add_cmd(buffer, &addCmd) != 0) {
                    printf( "err Failed to parse AddCmd\n");
                    continue;
                }
                // printf('sfuhdkjasn d');
                 res = handle_add_cmd(&server, &addCmd);
                //  printf("response,%d", res);
                 }
                break;
            case QUERY_CMD:
                handle_query_cmd(&server, &cmd.query);
                break;
            case END_CMD:
                free_chat_db(server.db);
                return;
            default:
                {
                    const char *error_msg = "err SYS_ERR: Invalid command type\n";
                    write(to_client_fd, error_msg, strlen(error_msg));
                }
                break;
        }
        printf("respons, %d",res);
        if(res == 0){
        write(to_client_fd, "ok", sizeof(char)*2);
        }
    }

    free_chat_db(server.db);
}

static int handle_add_cmd(Server *server, const AddCmd *addCmd) {


    int addSuccess = add_chat_db(server->db, addCmd->user, addCmd->room,
                addCmd->nTopics, addCmd->topics,
                addCmd->message);

    if(addSuccess != 0){
       printf("err: %s", error_chat_db(server->db));
       return -1;
    }
    // printf("  User: %s\n", addCmd->user);
    // printf("  Room: %s\n", addCmd->room);
    // printf("  Message: %d\n", addSuccess);
    return 0;
}

static int handle_query_cmd(Server *server, const QueryCmd *cmd) {
    // TODO: Implement query logic
    // For now, just send a placeholder response
    // write(server->out_fd, "ok\n", 3);
}