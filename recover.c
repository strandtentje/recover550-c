#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#define HEADER_SIZE 12
#define CLOSER_SIZE 2
#define KB 1024
#define MB 1048576
#define TMB 10485760
#define HMB 104857600
#define GB 1073741824
#define BUFFER_SIZE 8000000

#define STATE_NOTHING 0
#define STATE_HEADER 1
#define STATE_BODY1 2
#define STATE_BODY2 3
#define STATE_BODY3 4
#define STATE_BODY4 5
#define STATE_DONE 6

int fileCount = 0;

int tryHeader(char *data, int offset, char *header, int *cursor, int *filePointer) {
	int add;
	int readState;

	add = header[(*cursor)] == data[offset];
	(*cursor) += add;
	(*cursor) *= add;
	readState = add;

	if ((*cursor) == HEADER_SIZE) {
		(*cursor) = 0;
		readState = STATE_BODY1;
		printf("Switched to Body1\n");			
		char filename[127];
		sprintf(filename, "REC_%08d.CR2", fileCount++);
		printf("Writing to %s\n", filename);
		(*filePointer) = open(filename, O_WRONLY | O_CREAT, 0644);			
		if ((*filePointer) == -1) {
			perror("Failed to open write handle");
		}
		if (write((*filePointer), header, HEADER_SIZE) != (HEADER_SIZE)) {
			perror("Failed to write file header");
		}
	}

	return readState;
}

int tryBody(int state, char *data, int offset, char *closer, int *cursor, int filePointer) {
	write(filePointer, &data[offset], 1);

	int add; int sad;

	add = closer[(*cursor)] == data[offset];
	(*cursor) += add;
	(*cursor) *= add;
	sad = (*cursor) == CLOSER_SIZE;
	(*cursor) *= (1 - sad);
	state += sad;
	state %= STATE_DONE;

	if (state == STATE_NOTHING) {
		close(filePointer);
	};

	return state;
}

int main(int argc, char const *argv[])
{
	int headerFilePointer;
	headerFilePointer = open("SIGNATURE.CR2", O_RDONLY);

	if (headerFilePointer == -1) {
		perror("Couldn't read signature");
		return 2;
	}

	char header[HEADER_SIZE];
	int headerCursor = 0;

	if (read(headerFilePointer, &header, HEADER_SIZE) != HEADER_SIZE) {
		perror("Not enough header bytes");
		return 3;
	}

	close(headerFilePointer);

	char closer[] = { 0xff, 0xd9 };
	int closerCursor = 0;

	int readState = STATE_NOTHING;

	char inBytes[BUFFER_SIZE];
	int byteCount;

	long fileSize = 0;

	int targetFilePointer = 0;

	long totalBytes;

	int add; int sad;

	int i;

	while((byteCount = read(STDIN_FILENO, &inBytes, BUFFER_SIZE)) > 0) {
		for(i = 0; i < byteCount; i++)  {
			switch(readState) {
				case STATE_BODY1:
				case STATE_BODY2:
				case STATE_BODY3:
				case STATE_BODY4:
					readState = tryBody(readState, inBytes, i, closer, &closerCursor, targetFilePointer);
				break;
				default:
					readState = tryHeader(inBytes, i, header, &headerCursor, &targetFilePointer);
				break;
			}
		}
	}

	return 0;
}
