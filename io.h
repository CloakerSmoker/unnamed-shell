#ifndef LISHP_IO_H
#define LISHP_IO_H

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

ChildProcess* NewChildProcess(char* CommandPath, char** CommandArguments);
char* ReadFromChildProcessStream(ChildProcess* Child, int StreamNumber, size_t* OutSize);
__unused void WriteToChildProcessStream(ChildProcess* Child, int StreamNumber, char* Data, size_t Size);

#endif //LISHP_IO_H
