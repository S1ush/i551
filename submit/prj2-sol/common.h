#ifndef COMMON_H_
#define COMMON_H_

#include <chat-cmd.h>
#include <str-space.h>

#include <stdio.h>
#include "stdbool.h"

// declarations common between server and client

int deserialize_add_cmd(const char *input, AddCmd *addCmd);

int serialize_add_cmd(const AddCmd *add, StrSpace *strSpace);

int deserialize_query_cmd(const char *input, QueryCmd *queryCmd);

int serialize_query_cmd(const QueryCmd *query, StrSpace *strSpace);


#endif //#ifndef COMMON_H_
