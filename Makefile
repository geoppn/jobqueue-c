CC=gcc
CFLAGS=-I.

all: jobCommander jobExecutorServer

jobCommander: jobCommander.c
    $(CC) -o jobCommander jobCommander.c $(CFLAGS)

jobExecutorServer: jobExecutorServer.c jobQueue.c
    $(CC) -o jobExecutorServer jobExecutorServer.c jobQueue.c $(CFLAGS)

clean:
    rm -f jobCommander jobExecutorServer