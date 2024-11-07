COURSE = cs551


INCLUDE_DIR = $(HOME)/$(COURSE)/include
LIB_DIR = $(HOME)/$(COURSE)/lib

CC = gcc

CFLAGS = -g -Wall -std=gnu17 -I$(INCLUDE_DIR) $(MAIN_BUILD_FLAGS)
LDFLAGS = -L $(LIB_DIR) -Wl,-rpath=$(LIB_DIR)
LDLIBS = -lcs551 -lchat

TARGETS = chatd chatc

# object files needed by client (must include mock.o needed for testing)
CLIENT_OFILES = \
  chatc.o \
  chat.o \
  mock.o \
  client.o \
  common.o \
  utils.o

# object files needed by server (must include mock.o needed for testing)
SERVER_OFILES = \
  chatd.o \
  mock.o \
  server.o \
  server-loop.o \
  common.o \
  utils.o 


#default target
.PHONY:		all
all:		$(TARGETS)

chatc:		$(CLIENT_OFILES)
		$(CC)  $(LDFLAGS) $^  $(LDLIBS) -o $@

chatd:		$(SERVER_OFILES)
		$(CC)  $(LDFLAGS) $^  $(LDLIBS) -o $@


# use "make clean" to remove generated or backup files.
.PHONY:		clean
clean:
		rm -rf  *.o $(TARGETS) :memory: *.bak *~


# use "make DEPEND" to generate dependencies which can be
# pasted in below.
.PHONY:		DEPEND
DEPEND:
		gcc -MM  *.c

chat.o: chat.c chat.h utils.h client.c client.h common.h common.c 
chatc.o: chatc.c chat.h
chatd.o: chatd.c utils.h server.c server.h server-loop.h server-loop.c
mock.o: mock.c
utils.o: utils.c utils.h
