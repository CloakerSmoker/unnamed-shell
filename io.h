#ifndef MAL_IO_H
#define MAL_IO_H

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/socket.h>
#include <errno.h>

typedef struct TagStandardStream {
    union {
        int Pipe[2];

        struct {
            int Out;
            int In;
        };
    };
} StandardStream;

typedef struct TagChildProcess {
    union {
        StandardStream Streams[3];

        struct {
            StandardStream StandardInput;
            StandardStream StandardOutput;
            StandardStream StandardError;
        };
    };

    pid_t PID;
} ChildProcess;

ChildProcess* ChildProcess_New(char*, char**);
char* ChildProcess_ReadStream(ChildProcess*, int, int*);
void ChildProcess_WriteStream(ChildProcess*, int, char*, int);

#endif //MAL_IO_H
