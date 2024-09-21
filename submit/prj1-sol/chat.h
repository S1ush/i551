#ifndef CHAT_H_
#define CHAT_H_

#include "errnum.h"
#include "msgargs.h"

#include <stdbool.h>
#include <stddef.h>

// Forward declaration of ChatNode structure
typedef struct _ChatNode ChatNode;

// Function to create a new chat list
ChatNode *make_chat(void);

// Function to add a chat message to the chat list
void add_chat(ChatNode *chat, const char *user, const char *room, const char **topics, size_t nTopics, const char *msg);
void print_chat_messages(ChatNode *head);

ChatNode* fetch_query_details(ChatNode* head, const char* room, const char  **topic, size_t nTopics ,int count);

#endif // #ifndef CHAT_H_
