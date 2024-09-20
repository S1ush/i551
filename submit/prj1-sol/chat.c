#include "chat.h"
#include "errnum.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// #define DO_TRACE
#include <trace.h>

// Define chat ADT
typedef struct {
    char *user;     // pointer to user string
    char *room;     // pointer to room string
    size_t nTopics; // number of topics
    char **topics;  // pointer to array of topic strings
    char *message;  // pointer to message string
} _ChatMsg;

typedef struct _ChatNode {
    struct _ChatNode *next; // pointer to the next node
    _ChatMsg *msg;          // pointer to chat message
} ChatNode;

ChatNode *make_chat(void) {
    return calloc(1, sizeof(ChatNode));  // allocate and initialize memory for ChatNode
}

void add_chat(ChatNode *chat, const char *user, const char *room, const char **topics, size_t nTopics, const char *msg) {
    if (!chat) {
        fprintf(stderr, "Invalid chat pointer\n");
        return;
    }

    // Allocate memory for new chat node
    ChatNode *newChat = malloc(sizeof(ChatNode));
    if (!newChat) {
        perror("Failed to allocate memory for newChat");
        return;
    }

    // Allocate memory for the chat message
    newChat->msg = malloc(sizeof(_ChatMsg));
    if (!newChat->msg) {
        perror("Failed to allocate memory for _ChatMsg");
        free(newChat);
        return;
    }

    // Copy the user string
    newChat->msg->user = strdup(user);
    if (!newChat->msg->user) {
        perror("Failed to allocate memory for user");
        free(newChat->msg);
        free(newChat);
        return;
    }

    // Copy the room string
    newChat->msg->room = strdup(room);
    if (!newChat->msg->room) {
        perror("Failed to allocate memory for room");
        free(newChat->msg->user);
        free(newChat->msg);
        free(newChat);
        return;
    }

    // Allocate and copy topics
    newChat->msg->nTopics = nTopics;
    newChat->msg->topics = malloc(nTopics * sizeof(char *));
    if (!newChat->msg->topics) {
        perror("Failed to allocate memory for topics");
        free(newChat->msg->user);
        free(newChat->msg->room);
        free(newChat->msg);
        free(newChat);
        return;
    }

    // Copy each topic string
    for (size_t i = 0; i < nTopics; i++) {
        newChat->msg->topics[i] = strdup(topics[i]);
        if (!newChat->msg->topics[i]) {
            perror("Failed to allocate memory for topic");
            // Free previously allocated topics
            for (size_t j = 0; j < i; j++) {
                free(newChat->msg->topics[j]);
            }
            free(newChat->msg->topics);
            free(newChat->msg->user);
            free(newChat->msg->room);
            free(newChat->msg);
            free(newChat);
            return;
        }
    }

    // Copy the message string
    newChat->msg->message = strdup(msg);
    if (!newChat->msg->message) {
        perror("Failed to allocate memory for message");
        for (size_t i = 0; i < nTopics; i++) {
            free(newChat->msg->topics[i]);
        }
        free(newChat->msg->topics);
        free(newChat->msg->user);
        free(newChat->msg->room);
        free(newChat->msg);
        free(newChat);
        return;
    }

    // Link the new node into the list
    newChat->next = chat->next;
    chat->next = newChat;
}

void print_chat_messages(ChatNode *head) {
    if (!head) {
        fprintf(stderr, "Invalid chat pointer\n");
        return;
    }

    ChatNode *current = head->next;  // Start from the first actual message (head is a dummy node)
    int count = 0;

    while (current != NULL) {
        count++;
        printf("%s", current->msg->user);
        printf(" %s", current->msg->room);
        for (size_t i = 0; i < current->msg->nTopics; i++) {
            printf(" %s", current->msg->topics[i]);
        }
        printf("\n %s\n", current->msg->message);
        current = current->next;
    }

    if (count == 0) {
        printf("No chat messages found.\n");
    } 
}


ChatNode* fetch_query_details(ChatNode* head, const char* room, const char **topics, size_t nTopics, int count) {

    if (!head || !room) {
        fprintf(stderr, "Invalid input parameters\n");
        return NULL;
    }

    ChatNode* result = make_chat(); // Create a dummy head for the result list
    if (!result) {
        fprintf(stderr, "Failed to create result ChatNode\n");
        return NULL;
    }

    ChatNode* current = head->next; // Start from the first actual message
    ChatNode** matchedMessages = NULL;
    int matchedCount = 0;
    int actualCount = (count == 0) ? 1 : count; // If count is 0, we'll fetch just the last message


    matchedMessages = malloc(actualCount * sizeof(ChatNode*));
    if (!matchedMessages) {
        fprintf(stderr, "Failed to allocate memory for matchedMessages\n");
        free(result);
        return NULL;
    }

    bool anyTopicMatches = false;

    // First pass: find all matching messages
    while (current != NULL) {
        bool roomMatch = strcmp(current->msg->room, room) == 0;
        bool topicMatch = false;

        for (size_t i = 0; i < current->msg->nTopics; i++) {
            printf("%s ", current->msg->topics[i]);
        }
        printf("\n");

        // Check topics only if topics are provided and nTopics > 0
        if (nTopics > 0 && topics != NULL) {
            for (size_t i = 0; i < current->msg->nTopics && !topicMatch; i++) {
                for (size_t j = 0; j < nTopics; j++) {
                    if (strcmp(current->msg->topics[i], topics[j]) == 0) {
                        topicMatch = true;
                        anyTopicMatches = true;
                        break;
                    }
                }
            }
        }


        // Match condition: room must match, and (nTopics <= 0 or topicMatch)
        if (roomMatch && (nTopics <= 0 || topicMatch)) {
            if (matchedCount < actualCount) {
                matchedMessages[matchedCount++] = current;
            } else {
                // Shift the array to make room for the new match
                for (int i = 0; i < actualCount - 1; i++) {
                    matchedMessages[i] = matchedMessages[i + 1];
                }
                matchedMessages[actualCount - 1] = current;
            }
        }

        current = current->next;
    }


    // If no topic matches were found and nTopics > 0, retry with room-only matches
    if (matchedCount == 0 && nTopics > 0 && !anyTopicMatches) {
        current = head->next;
        while (current != NULL) {
            if (strcmp(current->msg->room, room) == 0) {
                if (matchedCount < actualCount) {
                    matchedMessages[matchedCount++] = current;
                } else {
                    for (int i = 0; i < actualCount - 1; i++) {
                        matchedMessages[i] = matchedMessages[i + 1];
                    }
                    matchedMessages[actualCount - 1] = current;
                }
            }
            current = current->next;
        }
    }

    // Second pass: create the result list with the last 'actualCount' matches
    for (int i = matchedCount - 1; i >= 0; i--) {
        ChatNode* newNode = malloc(sizeof(ChatNode));
        if (!newNode) {
            fprintf(stderr, "Failed to allocate memory for new node\n");
            free(matchedMessages);
            free_query_results(result);
            return NULL;
        }

        newNode->msg = malloc(sizeof(_ChatMsg));
        if (!newNode->msg) {
            fprintf(stderr, "Failed to allocate memory for new message\n");
            free(newNode);
            free(matchedMessages);
            free_query_results(result);
            return NULL;
        }

        // Deep copy message details
        newNode->msg->user = strdup(matchedMessages[i]->msg->user);
        newNode->msg->room = strdup(matchedMessages[i]->msg->room);
        newNode->msg->nTopics = matchedMessages[i]->msg->nTopics;
        newNode->msg->topics = malloc(newNode->msg->nTopics * sizeof(char*));
        for (size_t j = 0; j < newNode->msg->nTopics; j++) {
            newNode->msg->topics[j] = strdup(matchedMessages[i]->msg->topics[j]);
        }
        newNode->msg->message = strdup(matchedMessages[i]->msg->message);

        // Add the new node to the beginning of the result list
        newNode->next = result->next;
        result->next = newNode;

    }

    free(matchedMessages);

    if (matchedCount == 0) {
        fprintf(stderr, "No matching messages found\n");
        free(result);
        return NULL;
    }

    return result;
}

// Helper function to free the result of fetch_query_details
void free_query_results(ChatNode* results) {
    if (!results) return;
    
    ChatNode* current = results->next; // Start from the first actual node (skip dummy head)
    while (current) {
        ChatNode* temp = current;
        current = current->next;
        
        free(temp->msg->user);
        free(temp->msg->room);
        for (size_t i = 0; i < temp->msg->nTopics; i++) {
            free(temp->msg->topics[i]);
        }
        free(temp->msg->topics);
        free(temp->msg->message);
        free(temp->msg);
        free(temp);
    }
    free(results); // Free the dummy head
}