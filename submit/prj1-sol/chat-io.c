#include "chat-io.h"

#include "chat.h"
#include "errnum.h"

#include <errors.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


/** This function should read commands from `in` and write successful
 *  responses to `out` and errors to `err` as per the following spec.
 *  It should flush prompt on `out` before each read from `in`.
 *
 *  A word is defined to be a maximal sequence of characters without
 *  whitespace.  We have the following types of words:
 *
 *    USER:  A word starting with a '@' character.
 *    TOPIC: A word starting with a '#' character.
 *    ROOM:  A word starting with a letter.
 *    COUNT: A non-negative integer.
 *
 *  Comparison of all the above words must be case-insensitive.
 *
 *  In the following description, we use regex-style notation:
 *
 *     X?:  Denotes an optional X.
 *     X*:  Denotes 0-or-more occurrences of X.
 *
 *  Errors are of two kinds:
 *
 *    *User Error* caused by incorrect user input.  This should result
 *    in a single line error message being printed on `err` starting
 *    with a specific error code (defined below) followed by a single
 *    ": " followed by an informational message; the informational
 *    message is not defined by this spec but can be any text giving
 *    the details of the error.  The function should continue reading
 *    the next input after outputting the user error.
 *
 *    *System Error* caused by a system error like a memory allocation
 *    failure, an I/O error or an internal error (like an assertion
 *    failure).  The program should terminate (with a non-zero status)
 *    after printing a suitable message on `err` (`stderr` if it is
 *    an assertion failure).
 *
 *  There are two kinds of commands:
 *
 *  1) An ADD command consists of the following sequence of words
 *  starting with the "+" word:
 *
 *    + USER ROOM TOPIC*
 *
 *  followed by *one*-or-more lines constituting a MESSAGE terminated
 *  by a line containing only a '.'.  Note that the terminating line
 *  is not included in the MESSAGE.
 *
 *  A sucessful ADD command will add MESSAGE to chat-room ROOM on
 *  behalf of USER associated with the specified TOPIC's (if any).  It
 *  will succeed silently and will not produce any output on out or
 *  err.
 *
 *  An incorrect ADD command should output a single line on `err`
 *  giving the first error detected from the following possiblities:
 *
 *    BAD_USER:  USER not specified or USER does not start with '@'
 *    BAD_ROOM:  ROOM not specified or ROOM does not start with a
 *               alphabetic character.
 *    BAD_TOPIC: TOPIC does not start with a '#'.
 *    NO_MSG:    message is missing
 *
 *  It is not an error for an ADD command to specify duplicate TOPIC's.
 *
 *  2) A QUERY command consists of a single line starting with the "?"
 *  word:
 *
 *    ? ROOM COUNT? TOPIC*
 *
 *   followed by a terminating line containing only a '.'.
 *
 *   If specified, COUNT must specify a positive integer.  If not
 *   specified, it should default to 1.
 *
 *   A successful QUERY must output (on `out`) up to the last COUNT
 *   messages from chat-room ROOM which match all of the specified
 *   TOPIC's in a LIFO order.
 *
 *   Each matching message must be preceded by a line containing
 *
 *      USER ROOM TOPIC*
 *
 *   where USER is the user who posted the message, ROOM is the
 *   queried ROOM and TOPIC's are all the *distinct* topics associated
 *   with the message in the same order as when added. In case there
 *   are duplicate topics, only the first occurrence of that topic is
 *   shown.  The USER, ROOM and TOPICs must be output in lower-case.  The
 *   message contents must be output with the same whitespace content
 *   as when input.
 *
 *   If there are no messages matching the specified QUERY, then no
 *   output should be produced. If there are fewer than COUNT matching
 *   messages, then those messages should be output without any error.
 *
 *  An incorrect QUERY command should output a single line on `err`
 *  giving the first error detected from the following possiblities:
 *
 *    BAD_ROOM:  ROOM not specified or ROOM does not start with a
 *               alphabetic character.
 *               or room has not been specified in any added message.
 *    BAD_COUNT: COUNT is not a positive integer.
 *    BAD_TOPIC: A TOPIC does not start with a '#" or it
 *               has not been specified in any added message.
 *
 *  If the command is not a ADD or QUERY command then an error with
 *  code BAD_COMMAND should be output on `err`.
 *
 *  A response from either kind of command must always be followed by
 *  a single empty line on `out`.
 *
 *  There should be no hard-coded limits: specifically no limits
 *  beyond available memory on the size of a message, the number of
 *  messages or the number of TOPIC's associated with a message.
 *
 *  Under normal circumstances, the function should return only when
 *  EOF is detected on `in`.  All allocated memory must have been
 *  deallocated before returning.
 *
 *  When a system error is detected, the function should terminate the
 *  program with a non-zero exit status after outputting a suitable
 *  message on err (or stderr for an assertion failure).  Memory need
 *  not be cleaned up.
 *
 */
// void 
// add(const char *prompt , FILE *in , FILE *out){

// }


bool startsWithAlpha(const char *str) {
    return str != NULL && isalpha((unsigned char)*str);
}
void
chat_io(const char *prompt, FILE *in, FILE *out, FILE *err)
{
    fprintf(out, "%s", prompt);
    ChatNode *chat = make_chat();
    MsgArgs *msgArgs = NULL;
    ErrNum errNUm;
    while ((msgArgs = read_msg_args(in, msgArgs, &errNUm)) != NULL) {
        // fprintf(out, " msg is as read please check \n %s \n argvalue =-%zu ", msgArgs->msg, msgArgs->nArgs);
      
           fprintf(out, "%s", prompt); 
           fflush(out);
            // printf("Argument %d: %s\n", 0 + 1, msgArgs->args[0]);
            if (strcmp(msgArgs->args[0], "+") == 0) {

                int size_user = strlen(msgArgs->args[1]);
                char *user = (char *)malloc(size_user+1);
                 strcpy(user,msgArgs->args[1]);
                
                if(user[0] != '@'){
                    fprintf(err,"BAD_USER:  USER not specified or USER does not start with '@'\n");
                    continue;
                }

                if (msgArgs->msg == NULL || strcmp(msgArgs->msg, ".") == 0) {
                    fprintf(err, "NO_MSG: missing message\n");
                    continue;
                }
                const char *message;
                message = strdup(msgArgs->msg);
               //TODO : add to user to the storage 

               //TODO : add room
               char *room = (char *)malloc(strlen(msgArgs->args[2] + 1));
                 strcpy(room, msgArgs->args[2]);
               if(!startsWithAlpha(room)){
                fprintf(err,"BAD_ROOM:  ROOM not specified or ROOM does not start with a alphabetic character.\n");
                continue;
               }
               

               //TODO : add topic to the room 
               int topic_args;
               for(topic_args = 3 ; topic_args < msgArgs->nArgs; topic_args++){
                if(msgArgs->args[topic_args][0] != '#'){
                    
                    fprintf(err, "BAD_TOPIC: TOPIC does not start with a '#'.\n");
                    continue;
                }
               }
                int ntopics = msgArgs -> nArgs - 3 ;
               char **topic[ntopics];
               int i ;
               for(topic_args = 3 ,  i =  0 ; topic_args < msgArgs->nArgs ; topic_args++, i++){
                    topic[i] =(char *) malloc(strlen(msgArgs->args[topic_args]) + 1);
                    strcpy(topic[i], msgArgs->args[topic_args]);
               }

         
                  add_chat(chat, user,room,topic,ntopics,message);
               
                    
                } else if (strcmp(msgArgs->args[0], "?") == 0) {
             
    
                    char *room = (char *)malloc(strlen(msgArgs->args[2] + 1));
                    strcpy(room, msgArgs->args[1]);
      
                    int count;
                    char *strpoint;
                    int countArgs = 0 ;
                    if(msgArgs->args[2][0] != '#'){
                    count = strtol(msgArgs->args[2], &strpoint, 10);
                        if (*strpoint != '\0') {
                            fprintf(err,"BAD_COUNT: bad COUNT arg '%c'\n", *strpoint);
                            continue;
                        }
                    }else{
                        countArgs++;
                    }
      
                      int topic_args;
                        for(topic_args = 3 - countArgs ; topic_args < msgArgs->nArgs; topic_args++){
                            if(msgArgs->args[topic_args][0] != '#'){
                                
                                fprintf(err, "BAD_TOPIC: TOPIC does not start with a '#'.\n");
                                continue;
                            }
                        }
                            int ntopics = msgArgs -> nArgs - 2 ;
                        char **topic[ntopics];
                        int i ;
                        for(topic_args = 3 ,  i =  0 ; topic_args < msgArgs->nArgs ; topic_args++, i++){
                                topic[i] =(char *) malloc(strlen(msgArgs->args[topic_args]) + 1);
                                strcpy(topic[i], msgArgs->args[topic_args]);
                        }

                    
                    ChatNode *querry =  fetch_query_details(chat,room,topic,ntopics,count);
                    if(querry != NULL) print_chat_messages(querry);
             
                } else {
             
             
                }
              
        
    
    }






    // char line[1024];
    // int i;
    //     fprintf(out, "\n %s", prompt);  // Print the prompt
    // while (fgets(line, sizeof(line), in)) {
    //      fprintf(out, "\n %s ", prompt);
    //           if (strcmp(line, ".\n") == 0) {
    //             exit(0);
    //       }


    //     if (line[0] == '+') {
    //         fprintf(out, "insert to Linked list: %s  \n", line );
    //         for(i = 0 ; line[i] != '\0' ; i++){
    //           if(line)
    //         }
    //     } else if (line[0] == '?') {
    //         fprintf(out,"Query finding the data ");
    //     } else {
    //       fprintf(out, " should free up the memeory");
           
    //     }
    // }

}


//#define NO_CHAT_IO_MAIN to allow an alternate main()
#ifndef NO_CHAT_IO_MAIN

#include <unistd.h>

int main(int argc, const char *argv[]) {
  bool isInteractive = isatty(fileno(stdin));
  const char *prompt = isInteractive ? "> " : "";
  FILE *err = stderr;
  chat_io(prompt, stdin, stdout, err);
}

#endif //#ifndef NO_CHAT_IO_MAIN
