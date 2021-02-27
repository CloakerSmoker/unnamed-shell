#include "io.h"
#include "common.h"

void InitializeStandardStream(StandardStream* Stream) {
	pipe(Stream->Pipe);
}

void ReplaceFileDescriptor(int OriginalDescriptor, int NewDescriptor) {
	while (dup2(NewDescriptor, OriginalDescriptor) == -1 && errno == EINTR) {}
}

ChildProcess* NewChildProcess(char* CommandPath, char** CommandArguments) {
	ChildProcess* Result = alloc(sizeof(ChildProcess));

	InitializeStandardStream(&Result->StandardInput);
	InitializeStandardStream(&Result->StandardOutput);
	InitializeStandardStream(&Result->StandardError);

	pid_t ChildPID = fork();

	if (ChildPID == 0) {
		close(Result->StandardInput.In);
		close(Result->StandardOutput.Out);
		close(Result->StandardError.Out);

		ReplaceFileDescriptor(STDIN_FILENO, Result->StandardInput.Out);
		ReplaceFileDescriptor(STDOUT_FILENO, Result->StandardOutput.In);
		ReplaceFileDescriptor(STDERR_FILENO, Result->StandardError.In);

		execvp(CommandPath, CommandArguments);
	}
	else {
		close(Result->StandardInput.Out);
		close(Result->StandardOutput.In);
		close(Result->StandardError.In);
	}

	Result->PID = ChildPID;

	return Result;
}

char* ReadFromChildProcessStream(ChildProcess* Child, int StreamNumber, size_t* OutSize) {
	StandardStream* Stream = &Child->Streams[StreamNumber];

	size_t Size = 0;
	char Byte;

	while (read(Stream->Out, &Byte, 1) == 0) {}

	ioctl(Stream->Out, FIONREAD, &Size);

	char* Data = alloc(Size + 2);

	Data[0] = Byte;
	read(Stream->Out, &Data[1], Size);

	if (OutSize != NULL) {
		*OutSize = Size;
	}

	return Data;
}
unused void WriteToChildProcessStream(ChildProcess* Child, int StreamNumber, char* Data, size_t Size) {
	StandardStream* Stream = &Child->Streams[StreamNumber];

	write(Stream->In, Data, Size);
}
