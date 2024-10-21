#include "common.h"
#include <chat-cmd.h>

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

    // Parse the serialized input to extract fields (example with sscanf)
    char user[128], room[128], message[256];
    size_t nTopics;

    if (sscanf(input, "user=%127[^;];room=%127[^;];message=%255[^;];nTopics=%zu;", 
               user, room, message, &nTopics) != 4) {
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
            free(topics);
            return -1;  // Parsing error
        }
        topics[i] = strdup(topic);  // Store each topic
        pos = strstr(pos + 1, "topic[");  // Move to the next topic
    }

    // Populate the AddCmd struct
    addCmd->user = strdup(user);
    addCmd->room = strdup(room);
    addCmd->message = strdup(message);
    addCmd->nTopics = nTopics;
    addCmd->topics = topics;

    return 0;  // Success
}
