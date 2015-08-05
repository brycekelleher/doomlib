#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "doomlib.h"

static void Error(const char *format, ...)
{
	#define BUFFER_SIZE	1024

        va_list valist;
        char buffer[BUFFER_SIZE];

        va_start(valist, format);
        vsnprintf(buffer, BUFFER_SIZE, format, valist);
        va_end(valist);

        fprintf(stderr, "\x1b[31m");
        fprintf(stderr, "Error: %s", buffer);
        fprintf(stderr, "\x1b[0m");
	fflush(stderr);
        exit(1);
}

void IterateCallback(int lumpnum, char name[8], int size)
{
	printf("%4i \%-12.8s %i / %#x\n", lumpnum, name, size, size);
}

static void PrintUsage()
{
	printf("lswad <wadfile>\n");
}

int main(int argc, const char * argv[])
{
	if(argc == 1)
	{
		PrintUsage();
		exit(0);
	}

	Doom_ReadWadFile(argv[1]);

	Doom_IterateLumps(IterateCallback);

	Doom_CloseAll();
	
	return 0;
}
