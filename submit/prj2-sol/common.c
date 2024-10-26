#include "common.h"
#include <chat-cmd.h>
#include <chat-db.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

//uncomment next line to turn on tracing; use TRACE() with printf-style args
//#define DO_TRACE
#include <trace.h>

// TODO: add code for functions used by both client and server

int serialize_add_cmd(const AddCmd *add, StrSpace *strSpace) {
    init_str_space(strSpace);  // Initialize StrSpace for building the serialized string

    // Format the serialized string with all fields of AddCmd
    if (append_sprintf_str_space(strSpace, 
            "user=%s;room=%s;message=%s;nTopics=%zu;", 
            add->user, add->room, add->message, add->nTopics) != 0) {
        return -1;  // Serialization error
    }

    // Add each topic to the serialized string
    for (size_t i = 0; i < add->nTopics; ++i) {
        if (append_sprintf_str_space(strSpace, "topic[%zu]=%s;", i, add->topics[i]) != 0) {
            return -1;  // Serialization error
        }
    }
    return 0;  // Success
}

int deserialize_add_cmd(const char *input, AddCmd *addCmd) {
    memset(addCmd, 0, sizeof(AddCmd));  // Initialize AddCmd with zeroes
    
    char user[128], room[128];
    size_t nTopics;
    
    // Parse user and room first
    if (sscanf(input, "user=%127[^;];room=%127[^;];", user, room) != 2) {
        return 1;  // Parsing error
    }
    
    // Find message field
    const char *msg_start = strstr(input, "message=");
    if (!msg_start) return -1;
    msg_start += 8;  // Skip "message="
    
    // Find end of message (next semicolon)
    const char *msg_end = strchr(msg_start, ';');
    if (!msg_end) return 2;
    
    // Calculate message length and allocate buffer
    size_t msg_len = msg_end - msg_start;
    char *message = malloc(msg_len + 1);  // +1 for null terminator
    if (!message) return 3;
    
    // Copy message content
    strncpy(message, msg_start, msg_len);
    message[msg_len] = '\0';  // Ensure null termination
    
    // Parse nTopics
    const char *topics_count = strstr(msg_end, "nTopics=");
    if (!topics_count || sscanf(topics_count, "nTopics=%zu;", &nTopics) != 1) {
        free(message);
        return 4;
    }
    
    // Allocate memory for topics
    const char **topics = malloc(nTopics * sizeof(char *));
    if (!topics) {
        free(message);
        return 5;
    }
    
    // Initialize topics array to NULL for safer cleanup on error
    for (size_t i = 0; i < nTopics; i++) {
        topics[i] = NULL;
    }
    
    // Extract topics from the input string
    const char *pos = strstr(input, "topic[0]=");
    for (size_t i = 0; i < nTopics && pos; ++i) {
        char topic[128];
        if (sscanf(pos, "topic[%zu]=%127[^;];", &i, topic) != 2) {
            // Cleanup on error
            for (size_t j = 0; j < i; j++) {
                free((void*)topics[j]);
            }
            free(topics);
            free(message);
            return 6;
        }
        topics[i] = strdup(topic);
        if (!topics[i]) {
            // Cleanup on allocation failure
            for (size_t j = 0; j < i; j++) {
                free((void*)topics[j]);
            }
            free(topics);
            free(message);
            return 7;
        }
        pos = strstr(pos + 1, "topic[");
    }
    
    // Populate the AddCmd struct
    addCmd->user = strdup(user);
    addCmd->room = strdup(room);
    addCmd->message = message;  // Transfer ownership of dynamically allocated message
    addCmd->nTopics = nTopics;
    addCmd->topics = topics;
    
    // Check if any strdup failed
    if (!addCmd->user || !addCmd->room) {
        // Cleanup everything if any allocation failed
        free((void*)addCmd->user);
        free((void*)addCmd->room);
        free(addCmd->message);
        for (size_t i = 0; i < nTopics; i++) {
            free((void*)topics[i]);
        }
        free(topics);
        return 8;
    }
    
    return 0;  // Success
}


int serialize_query_cmd(const QueryCmd *query, StrSpace *strSpace) {
    init_str_space(strSpace);  // Initialize StrSpace for building the serialized string

    // Format the serialized string with all fields of QueryCmd
    if (append_sprintf_str_space(strSpace, 
            "room=%s;count=%zu;nTopics=%zu;", 
            query->room, query->count, query->nTopics) != 0) {
        return -1;  // Serialization error
    }

    // Add each topic to the serialized string
    for (size_t i = 0; i < query->nTopics; ++i) {
        if (append_sprintf_str_space(strSpace, "topic[%zu]=%s;", i, query->topics[i]) != 0) {
            return -1;  // Serialization error
        }
    }
    return 0;  // Success
}


int deserialize_query_cmd(const char *input, QueryCmd *queryCmd) {
    memset(queryCmd, 0, sizeof(QueryCmd));  // Initialize QueryCmd with zeroes

    // Parse the serialized input to extract fields
    char room[128];
    size_t count, nTopics;

    if (sscanf(input, "room=%127[^;];count=%zu;nTopics=%zu;", room, &count, &nTopics) != 3) {
        return -1;  // Parsing error
    }

    // Allocate memory for topics
    const char **topics = malloc(nTopics * sizeof(char *));
    if (!topics) return -1;  // Memory allocation failure

    // Extract topics from the input string
    const char *pos = strstr(input, "topic[0]=");
    for (size_t i = 0; i < nTopics && pos; ++i) {
        char topic[128];
        if (sscanf(pos, "topic[%zu]=%127[^;];", &i, topic) != 2) {
            free(topics);  // Free memory on error
            return -1;  // Parsing error
        }
        topics[i] = strdup(topic);  // Store the topic
        pos = strstr(pos + 1, "topic[");  // Move to the next topic
    }

    // Populate the QueryCmd struct
    queryCmd->room = strdup(room);
    queryCmd->count = count;
    queryCmd->nTopics = nTopics;
    queryCmd->topics = topics;

    return 0;  // Success
}

int serialize_chat_info(const ChatInfo *chatInfo, StrSpace *strSpace) {
    if (!chatInfo) return -1;  // Error check

    init_str_space(strSpace);  // Initialize dynamic string space

    char timestamp[32];
    timestamp_to_iso8601(chatInfo->timestamp, sizeof(ISO_8601_FORMAT) + 1, timestamp);

    // Serialize basic fields
    if (append_sprintf_str_space(strSpace,
        "user=%s;room=%s;message=%s;timestamp=%s;nTopics=%zu;",
        chatInfo->user, chatInfo->room, chatInfo->message, 
        timestamp, chatInfo->nTopics) != 0) {
        return -1;  // Serialization error
    }

    // Serialize each topic
    for (size_t i = 0; i < chatInfo->nTopics; ++i) {
        if (append_sprintf_str_space(strSpace, "topic[%zu]=%s;", 
            i, chatInfo->topics[i]) != 0) {
            return -1;  // Serialization error
        }
    }

    return 0;  // Success
}


int deserialize_chat_info(const char *input, ChatInfo *chatInfo) {
    if (!input || !chatInfo) return -1;  // Error check
    memset(chatInfo, 0, sizeof(ChatInfo));  // Initialize structure
    
    char user[128], room[128], timestamp_str[32];
    TimeMillis timestamp;
    size_t nTopics;
    
    // First, find the message boundaries
    const char *msg_start = strstr(input, "message=");
    if (!msg_start) return -1;
    msg_start += 8;  // Length of "message="
    
    const char *msg_end = strstr(msg_start, ";timestamp=");
    if (!msg_end) return -1;
    
    // Calculate message length and allocate buffer
    size_t msg_len = msg_end - msg_start;
    char *message = malloc(msg_len + 1);  // +1 for null terminator
    if (!message) return -1;
    
    // Extract message
    strncpy(message, msg_start, msg_len);
    message[msg_len] = '\0';
    
    // Parse other fields using the message end as a reference point
    char format[256];
    snprintf(format, sizeof(format), 
             "user=%%127[^;];room=%%127[^;];message=%%%zu[^;];timestamp=%%31[^;];nTopics=%%zu;",
             msg_len);
    
    if (sscanf(input, format, user, room, message, timestamp_str, &nTopics) != 5) {
        free(message);
        return -1;  // Parsing error
    }
    
    // Allocate memory for topics
    const char **topics = malloc(nTopics * sizeof(char *));
    if (!topics) {
        free(message);
        return -1;  // Memory allocation error
    }
    
    // Parse each topic
    const char *pos = strstr(input, "topic[0]=");  // Start of topics
    for (size_t i = 0; i < nTopics && pos; ++i) {
        char topic[128];
        if (sscanf(pos, "topic[%zu]=%127[^;];", &i, topic) != 2) {
            // Cleanup on failure
            free(message);
            for (size_t j = 0; j < i; j++) {
                free((void*)topics[j]);
            }
            free(topics);
            return -1;
        }
        topics[i] = strdup(topic);  // Store topic
        pos = strstr(pos + 1, "topic[");  // Move to the next topic
    }
    
    // Populate ChatInfo structure
    chatInfo->user = strdup(user);
    chatInfo->room = strdup(room);
    chatInfo->message = message;  // Transfer ownership of dynamically allocated message
    chatInfo->timestamp = timestamp_str;
    chatInfo->nTopics = nTopics;
    chatInfo->topics = topics;
    
    return 0;  // Success
}



void free_query_cmd(QueryCmd *cmd) {
    if (cmd->topics) {
        for (size_t i = 0; i < cmd->nTopics; ++i) {
            free((char *)cmd->topics[i]);  // Free each topic string
        }
        free(cmd->topics);  // Free the topics array
    }
    free((char *)cmd->room);  // Free the room string
}