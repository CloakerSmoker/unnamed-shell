#include "io.h"
#include "common.h"

StandardStream* StandardStream_Initialize(StandardStream* Stream) {
	pipe(Stream->Pipe);
}

void StandardStream_Replace(int OriginalStream, int NewStream) {
	while (dup2(NewStream, OriginalStream) == -1 && errno == EINTR) {}
}

ChildProcess* ChildProcess_New(char* CommandPath, char** CommandArguments) {
	ChildProcess* Result = alloc(sizeof(ChildProcess));

	StandardStream_Initialize(&Result->StandardInput);
	StandardStream_Initialize(&Result->StandardOutput);
	StandardStream_Initialize(&Result->StandardError);

	pid_t ChildPID = fork();

	if (ChildPID == 0) {
		close(Result->StandardInput.In);
		close(Result->StandardOutput.Out);
		close(Result->StandardError.Out);

		StandardStream_Replace(STDIN_FILENO, Result->StandardInput.Out);
		StandardStream_Replace(STDOUT_FILENO, Result->StandardOutput.In);
		StandardStream_Replace(STDERR_FILENO, Result->StandardError.In);

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

char* ChildProcess_ReadStream(ChildProcess* Child, int StreamNumber, int* OutSize) {
	StandardStream* Stream = &Child->Streams[StreamNumber];

	int Size = 0;
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
//void ChildProcess_WriteStream(ChildProcess* Child, int StreamNumber, char* Data, int Size) {
//	StandardStream* Stream = &Child->Streams[StreamNumber];
//
//	write(Stream->In, Data, Size);
//}
