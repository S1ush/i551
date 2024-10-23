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
    QueryCmd QueryCmd;
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

           ssize_t n = read(from_client_fd, buffer, sizeof(buffer) - 1);
                if(n <= 0){
                    printf("err reacving the data ");
                }   
        switch (type) {
            case ADD_CMD:     {
             
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

                    if (deserialize_query_cmd(buffer, &QueryCmd) != 0) {
                            printf( "err Failed to parse AddCmd\n");
                            continue;
                        }
                  
                res = handle_query_cmd(&server, &QueryCmd);
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

// Helper function to print each chat result (IterFn)
// Helper function to print each chat result (IterFn)
// static int print_chat_info(const ChatInfo *chatInfo, void *ctx) {
//     if (!chatInfo || !ctx) {
//         return -1;  // Safety check: return error if pointers are invalid
//     }

//     int out_fd = *(int *)ctx;  // Extract the file descriptor from the context

//     // Format the timestamp into ISO-8601 format
//     char timestamp[32];
//     timestamp_to_iso8601(chatInfo->timestamp, sizeof(timestamp), timestamp);

//     // Print chat message details to the client
//     printf( 
//         "User: %s\nRoom: %s\nMessage: %s\nTimestamp: %s\n", 
//         chatInfo->user, chatInfo->room, chatInfo->message, timestamp);

//     // Print topics if present
//     if (chatInfo->nTopics > 0) {
//         printf( "Topics:");
//         for (size_t i = 0; i < chatInfo->nTopics; ++i) {
//             printf( " %s", chatInfo->topics[i]);
//         }
//         printf( "\n");
//     }
//     printf( "-----\n");  // Separator between messages

//     return 0;  // Continue iterating


//     // if (!chatInfo) return -1;  // Safety check

//     // char timestamp[32];
//     // timestamp_to_iso8601(chatInfo->timestamp, sizeof(timestamp), timestamp);

//     // // Print chat message details
//     // // printf(
//     // //     "Timesstamp: %s\nUser: %s\nRoom: %s\nMessage: %s\n",
//     // //     timestamp, chatInfo->user, chatInfo->room, chatInfo->message
//     // // );

//     // // Print topics if any
//     // if (chatInfo->nTopics > 0) {
//     //     printf("Topics:");
//     //     for (size_t i = 0; i < chatInfo->nTopics; ++i) {
//     //         printf(" %s", chatInfo->topics[i]);
//     //     }
//     //     printf("\n");
//     // }
//     // printf("-----\n");

//     // return 0;


// }

static int serialize_and_send_chat_info(const ChatInfo *chatInfo, void *ctx) {
    StrSpace strSpace;
    init_str_space(&strSpace);  // Initialize dynamic string space


    char timestamp[32];
    timestamp_to_iso8601(chatInfo->timestamp, sizeof(ISO_8601_FORMAT) + 1, timestamp);

    // Serialize basic information of the chat
    append_sprintf_str_space(&strSpace,
        "user=%s;room=%s;message=%s;timestamp=%s;nTopics=%zu;",
        chatInfo->user, chatInfo->room, chatInfo->message,
        timestamp, chatInfo->nTopics);

    // Serialize the topics
    for (size_t i = 0; i < chatInfo->nTopics; ++i) {
        append_sprintf_str_space(&strSpace, "topic[%zu]=%s;", i, chatInfo->topics[i]);
    }

    const char *serialized_data = iter_str_space(&strSpace, NULL);
    int out_fd = *(int *)ctx;  // Extract the file descriptor

    // Send the serialized data to the client
    write(out_fd, serialized_data , strlen(serialized_data));
    // fflush(0);
    free_str_space(&strSpace);  // Clean up
    return 0;  // Continue iterating
}


static int handle_query_cmd(Server *server, const QueryCmd *queryCmd) {
    if (!server || !queryCmd || !server->db) {
        dprintf(server->out_fd, "err: Invalid server state or query command\n");
        return -1;
    }

    // Extract query details
    const char *room = queryCmd->room;
    size_t nTopics = queryCmd->nTopics;
    const char **topics = queryCmd->topics;
    size_t maxCount = queryCmd->count;

    // printf("Executing QUERY_CMD: room=%s, nTopics=%zu, count=%zu\n", 
    //        room, nTopics, maxCount);

    // Perform the database query with the callback `print_chat_info'
    // Use StrSpace for error message handling
    StrSpace response;
    init_str_space(&response);

    // int *ctx = server->out_fd;
    int queryResult = query_chat_db(
        server->db, room, nTopics, topics, maxCount, serialize_and_send_chat_info, &server->out_fd
    );

    // Handle the result of the query
    if (queryResult != 0) {
        const char *error = error_chat_db(server->db);
        dprintf(server->out_fd, "err: %s\n", error);
        return -1;
    }

    return 0;  // Success
}

// int filter()


// static int handle_query_cmd(Server *server, const QueryCmd *querycmd) {
//     // TODO: Implement query logic
//     // For now, just send a placeholder response
//     // write(server->out_fd, "ok\n", 3);


//     if (!server || !querycmd || !server->db) {  // Safety checks
//         dprintf(server->out_fd, "err: Invalid server state or query command\n");
//         return -1;
//     }

//     // if (querycmd == NULL) {
//     //     dprintf(server->out_fd, "err: Invalid query command\n");
//     //     return -1;
//     // }

//     const char *room = querycmd->room;
//     size_t nTopics = querycmd->nTopics;
//     const char **topics = querycmd->topics;
//     size_t maxCount = querycmd->count;

//     // Use StrSpace for dynamic error message handling
//     StrSpace response;
//     init_str_space(&response);

//     // Perform the database query
//     int result = query_chat_db(
//         server->db, room, nTopics, topics, maxCount, print_chat_info, &server->out_fd
//     );

//     // Handle success or error
//     if (result != 0) {
//         const char *error = error_chat_db(server->db);
//         append_sprintf_str_space(&response, "err: %s\n", error);
//         dprintf(server->out_fd, "%s", iter_str_space(&response, NULL));
//         // free_str_space(&response);
//         return -1;
//     } else {
//         append_str_space(&response, "ok: Query completed\n");
//         dprintf(server->out_fd, "%s", iter_str_space(&response, NULL));
//     }

//     printf("Received QUERY_CMD: room=%s, nTopics=%zu, count=%zu\n", 
//         querycmd->room, querycmd->nTopics, querycmd->count);

//     // Clean up
//     // free_str_space(&response);
//     return 0;

// }


