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

static void PrintUsage()
{
	printf("dumpwad <wadfile> <lumpname>\n");
}

static void DumpLump(int lumpnum)
{
	FILE *fp = stdout;

	int length = Doom_LumpLength(lumpnum);
	if(!length || length == -1)
		Error("invalid lump number\n");

	unsigned char *data = (unsigned char*)Doom_LumpFromNum(lumpnum);
	if(!data)
		Error("couldn't read lump data\n");

	while(length--)
		fputc(*data++, fp);
}

int main(int argc, const char * argv[])
{
	if(argc == 1)
	{
		PrintUsage();
		exit(0);
	}

	Doom_ReadWadFile(argv[1]);

	DumpLump(atoi(argv[2]));

	Doom_CloseAll();
	
	return 0;
}
