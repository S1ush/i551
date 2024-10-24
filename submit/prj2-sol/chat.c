#include "chat.h"
#include "common.h"
#include "server.h"
#include "str-space.h"

#include <chat-cmd.h>
#include <errors.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
// #include <chat-base.h>

#include <unistd.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>


/***************************** Declarations ****************************/

// fill out ADT structure
struct _Chat {
    int client_to_server[2];  // Pipe for client to server communication
    int server_to_client[2];  // Pipe for server to client communication
    pid_t server_pid;            // PID of the server process
    FILE *out;                   // Output stream for success responses
    FILE *err;                   // Error stream for error messages
};

// prefix for all error messages
#define ERROR "err "

// line to indicate a successful response
#define OKAY "ok\n"

/**************************** Private Routines *************************/

// TODO: static routines local to this file

/******************************* Public API ****************************/

/** Return a new Chat object which creates a server process which uses
 *  the sqlite database located at dbPath.  All commands must be sent
 *  by this client process to the server and handled by the server
 *  using the database. All IPC must use anonymous pipes.  The
 *  returned object should encapsulate all the state needed to
 *  implement the following API.
 *
 *  The client process can use `out` for writing success output for
 *  commands where each output must start with a line containing "ok".
 *
 *  The client process should use `err` for writing error message
 *  lines (must start with "err ERR_CODE: " where ERR_CODE is
 *  BAD_ROOM/BAD_TOPIC for unknown room/topic, or SYS_ERR for non-user
 *  errors).
 *
 *  The server should not use the `in` or `out` streams.  It may use
 *  `stderr` for "logging", but all such logging *must* be turned off
 *  before submission.
 *
 *  If errors are encountered, then this function should return NULL.
 */
Chat *
make_chat(const char *dbPath, FILE *out, FILE *err)
{  
  Chat *chat = malloc(sizeof(Chat));
    if (!chat) {
        fprintf(err, "%serror allocating memory\n", ERROR);
        return NULL;
    }
      // Initialize pipes for IPC
    if (pipe(chat->client_to_server) == -1 || pipe(chat->server_to_client) == -1) {
        fprintf(err, "%spipe creation failed\n", ERROR);
        free(chat);
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        // Fork failed
        fprintf(err, "%sfork failed\n", ERROR);
        close(chat->client_to_server[1]);
        close(chat->client_to_server[0]);
        close(chat->server_to_client[0]);
        close(chat->server_to_client[1]);
        free(chat);
        return NULL;
    } else if (pid == 0) {
        // In the child (server) process
        close(chat->client_to_server[1]);  // Close write end of client to server pipe in the server
        close(chat->server_to_client[0]);  // Close read end of server to client pipe in the server
        // fprintf(err, "%sfork success 1\n", ERROR);
        // do_server();
        // do_server(chat->out, chat->server_to_client[1], dbPath);
        do_server(chat->client_to_server[0], chat->server_to_client[1], dbPath);
        // fprintf(err, "%sfork success2\n", ERROR);
        // exit(0);
          // Server process should exit after handling
    }

    chat->server_pid = pid;
    close(chat->client_to_server[0]);  // Close read end of client to server pipe in the client
    close(chat->server_to_client[1]);  // Close write end of server to client pipe in the client
    chat->out = out;
    chat->err = err;
    fflush(out);
  return chat;  
}


/** free all resources like memory, FILE's and descriptors used by chat
 *  All resources must be freed even after user errors have been detected.
 *  It is okay if resources are not freed after system errors.
 */
void
free_chat(Chat *chat)
{
  //TODO
}


/** perform cmd using chat, wri)ting response to chat's out/err
 *  streams.  It can be assumed that cmd is free of user errors except
 *  for unknown room/topic for QUERY commands.
 *
 *  If the command is an END_CMD command, then ensure that the server
 *  process is shut down cleanly.
 */

void receive_and_print_chat_info(int server_fd,Chat *chat) {
    char buffer[BUFSIZ];
    int res = 0;
    int temp = 0;
    while ((res = read(server_fd, buffer, sizeof(buffer) - 1)) > 0) {
        ChatInfo chatInfo;
        if(( strncmp(buffer, "end", 3)) == 0){
          // fprintf(chat->out, "\n received ok\n");
          break;
        }
      if(temp == 0){
        fprintf(chat->out,"ok\n");
        temp++;
      }
        buffer[sizeof(buffer) - 1] = '\0';  // Ensure null termination
        // res = 0;
        // fprintf(chat->out,"size: %d\n buf : %s\n",res,buffer);
        // // Deserialize the chat info
        if ( deserialize_chat_info(buffer, &chatInfo) != 0) {
            fprintf(chat->err,"Error: Failed to deserialize chat info %d\n",res);
            // continue;
        }
           memset(buffer, 0, sizeof (buffer));

        // // Print the deserialized chat info
        fprintf( chat->out, "%s\n%s %s",chatInfo.timestamp, chatInfo.user, chatInfo.room  );

        // // Print topics if any
        if (chatInfo.nTopics > 0) {
            for (size_t i = 0; i < chatInfo.nTopics; ++i) {
                fprintf(chat->out , " %s", chatInfo.topics[i]);
            }
            fprintf(chat->out,"\n");
        }
        fprintf(chat->out,"%s\n",chatInfo.message);

        // // Free allocated memory for topics
        // for (size_t i = 0; i < chatInfo.nTopics; ++i) {
        //     free((char *)chatInfo.topics[i]);
        // }
        // free((char **)chatInfo.topics);
        // free((char *)chatInfo.user);
        // free((char *)chatInfo.room);
        // free((char *)chatInfo.message);
        memset(buffer, 0, sizeof (buffer));
    }
    
        // fprintf(chat->out,"\n exited %d\n buf : %s\n", res, buffer);
}



void
do_chat_cmd(Chat *chat, const ChatCmd *cmd)
{
  // fprintf(chat->out, "Ch÷÷/÷s in here : %d ", cmd->type );
   // Send command to server

  
    // fprintf(chat->out,"is this even working");
    StrSpace strSpace;

     if((write(chat->client_to_server[1], &cmd->type, sizeof(CmdType))) <=  0){
            fprintf(chat->err, ERROR "SYS_ERR: Failed to send command to server : type\n");
            return;
      }
    switch (cmd->type)
    {
    case ADD_CMD:{
          if (serialize_add_cmd(&cmd->add, &strSpace) != 0) {
            fprintf(chat->err, "ERROR: Failed to serialize AddCmd.\n");
            return;
        }
         
        const char *serialized_add = iter_str_space(&strSpace, NULL);
        if (write(chat->client_to_server[1], serialized_add, strlen(serialized_add)) <= 0) {
            fprintf(chat->err, ERROR "SYS_ERR: Failed to send command to server : add_data\n");
            return;
        }

        
      char response[1024];
        ssize_t n = read(chat->server_to_client[0], response, sizeof(response) - 1);
        if (n < 0) {
            fprintf(chat->err, ERROR "SYS_ERR: Failed to read server response\n");
            return;
        }

        response[n] = '\0';

        if (strncmp(response, "ok", 2) == 0) {
            fprintf(chat->out, "%s\n", response);
        } else if (strncmp(response, "err", 3) == 0) {
            fprintf(chat->err, "%s\n", response);
        } else {
            fprintf(chat->err, ERROR "SYS_ERR: Invalid server response, %s\n",response);
        }

        
    
      
       
    }
      /* code */
      break;
    
    case QUERY_CMD:

        if (serialize_query_cmd(&cmd->query, &strSpace) != 0) {
            fprintf(chat->err,"Serialized QueryCmd: %s\n", strSpace.buf);
        }

        const char *serialized_querry = iter_str_space(&strSpace, NULL);
        if (write(chat->client_to_server[1], serialized_querry, strlen(serialized_querry)) <= 0) {
            fprintf(chat->err, ERROR "SYS_ERR: Failed to send command to server : add_data\n");
            return;
        }
        receive_and_print_chat_info(chat->server_to_client[0],chat);
        // fprintf(chat->out, " testing\n");


      break;
    default:
      break;
    }

  
}



/** return server's PID */
pid_t
chat_server_pid(const Chat *chat)
{
  //TODO
 return chat->server_pid;
}


// Random stuff 

// static bool write_string(int fd, const char* str) {
//     size_t len = strlen(str);
//     if (write(fd, &len, sizeof(size_t)) != sizeof(size_t)) return false;
//     if (write(fd, str, len) != len) return false;
//     return true;
// }

// bool send_add_cmd(int fd, const AddCmd* cmd) {
//     CmdType type = ADD_CMD;
//     if (write(fd, &type, sizeof(CmdType)) != sizeof(CmdType)) return false;

//     if (!write_string(fd, cmd->user)) return false;
//     if (!write_string(fd, cmd->room)) return false;
//     if (!write_string(fd, cmd->message)) return false;

//     if (write(fd, &cmd->nTopics, sizeof(size_t)) != sizeof(size_t)) return false;

//     for (size_t i = 0; i < cmd->nTopics; i++) {
//         if (!write_string(fd, cmd->topics[i])) return false;
//     }

//     return true;
// }

// bool send_query_cmd(int fd, const QueryCmd* cmd) {
//     CmdType type = QUERY_CMD;
//     if (write(fd, &type, sizeof(CmdType)) != sizeof(CmdType)) return false;

//     if (!write_string(fd, cmd->room)) return false;
//     if (write(fd, &cmd->count, sizeof(size_t)) != sizeof(size_t)) return false;
//     if (write(fd, &cmd->nTopics, sizeof(size_t)) != sizeof(size_t)) return false;

//     for (size_t i = 0; i < cmd->nTopics; i++) {
//         if (!write_string(fd, cmd->topics[i])) return false;
//     }

//     return true;
// }

// bool send_end_cmd(int fd) {
//     CmdType type = END_CMD;
//     return write(fd, &type, sizeof(CmdType)) == sizeof(CmdType);
// }