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

static int search = 0;

void do_server(int from_client_fd, int to_client_fd, const char *dbPath)
{  
    Server server = { from_client_fd, to_client_fd, NULL };
    CmdType type;
    AddCmd addCmd;
    QueryCmd queryCmd;
    char buffer[BUFSIZ*2];
    MakeChatDbResult result;
    int res = -1;

    if (make_chat_db(dbPath, &result) != 0) {
        const char *error_msg = "err SYS_ERR: Failed to create database\n";
        write(to_client_fd, error_msg, strlen(error_msg));
        return;
    }
    server.db = result.chatDb;

    while (1) {
        ssize_t read_bytes = read(from_client_fd, &type, sizeof(CmdType));
        if (read_bytes <= 0) {
            if (read_bytes == 0) break;  // EOF
            const char *error_msg = "err SYS_ERR: Failed to read command\n";
            write(to_client_fd, error_msg, strlen(error_msg));
            continue;
        }

        ssize_t n = read(from_client_fd, buffer, sizeof(buffer) - 1);
        if(n <= 0) {
            continue;
        }
        buffer[n] = '\0';

        switch (type) {
            case ADD_CMD: {
                int res = 0;
                if ((res = deserialize_add_cmd(buffer, &addCmd)) != 0) {
                    continue;
                }
                res = handle_add_cmd(&server, &addCmd);
                if(res == 0) {
                    write(to_client_fd, "ok", sizeof(char)*2);
                }
                break;
            }
            case QUERY_CMD:
                if (deserialize_query_cmd(buffer, &queryCmd) != 0) {
                    printf("err Failed to parse AddCmd\n");
                    continue;
                }
                res = handle_query_cmd(&server, &queryCmd);
                break;
            case END_CMD:
                if (server.db) {
                    free_chat_db(server.db);
                }
                return;
            default: {
                const char *error_msg = "err SYS_ERR: Invalid command type\n";
                write(to_client_fd, error_msg, strlen(error_msg));
                break;
            }
        }
    }

    if (server.db) {
        free_chat_db(server.db);
    }
}

static int handle_add_cmd(Server *server, const AddCmd *addCmd) {
    if (!server || !addCmd || !server->db) {
        return -1;
    }

    int addSuccess = add_chat_db(server->db, addCmd->user, addCmd->room,
                                addCmd->nTopics, addCmd->topics,
                                addCmd->message);

    if(addSuccess != 0) {
        return -1;
    }
    return 0;
}

static void serialize_and_send_chat_info(const ChatInfo *chatInfo, void *ctx) {
    if (!chatInfo || !ctx) {
        return;
    }

    StrSpace strSpace;
    init_str_space(&strSpace);
    int out_fd = *(int *)ctx;

    char timestamp[32];
    timestamp_to_iso8601(chatInfo->timestamp, sizeof(ISO_8601_FORMAT) + 1, timestamp);

    append_sprintf_str_space(&strSpace,
        "user=%s;room=%s;message=%s;timestamp=%s;nTopics=%zu;",
        chatInfo->user, chatInfo->room, chatInfo->message,
        timestamp, chatInfo->nTopics);

    for (size_t i = 0; i < chatInfo->nTopics; ++i) {
        append_sprintf_str_space(&strSpace, "topic[%zu]=%s;", i, chatInfo->topics[i]);
    }

    const char *serialized_data = iter_str_space(&strSpace, NULL);
    uint32_t data_len = htonl(strlen(serialized_data));

    // Send data length as a 4-byte integer
    write(out_fd, &data_len, sizeof(data_len));
    
    // Send the actual serialized data
    write(out_fd, serialized_data, strlen(serialized_data));
    free_str_space(&strSpace);
}


static int handle_query_cmd(Server *server, const QueryCmd *queryCmd) {
    if (!server || !queryCmd || !server->db) {
        dprintf(server->out_fd, "err: Invalid server state or query command\n");
        return -1;
    }

    const char *room = queryCmd->room;
    size_t nTopics = queryCmd->nTopics;
    const char **topics = queryCmd->topics;
    size_t maxCount = queryCmd->count;

    size_t count = 0;
    if (count_room_chat_db(server->db, room, &count) != 0 || count == 0) {
        write(server->out_fd, "room", strlen("room")*sizeof(char));
        return -1;
    }

    if (nTopics > 0) {
        size_t topic_count = 0;
        for (size_t i = 0; i < nTopics; i++) {
            if (count_topic_chat_db(server->db, topics[i], &topic_count) != 0 || topic_count == 0) {
                write(server->out_fd, "topic", strlen("topic")*sizeof(char));
                return -1;
            }
        }
    }

    search = 0;
    int queryResult = query_chat_db(
        server->db, room, nTopics, topics, maxCount, 
        serialize_and_send_chat_info, &server->out_fd
    );

    if (search == 0) {
        write(server->out_fd, "searched", strlen("searched")*sizeof(char));
        sleep(0.5);
    }

    if (queryResult == 0) {
        write(server->out_fd, "end", strlen("end")*sizeof(char));
    }

    return 0;
}