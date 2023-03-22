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

void ListLumps(wadfile_t *wadfile)
{
	printf("%-4s %-12s %-12s %-12s\n", "num", "name", "offset", "size");

	int numlumps = Wad_NumLumps(wadfile);
	for (int i = 0; i < numlumps; i++) {
		int size = Wad_LumpSize(wadfile, i);
		int offset = Wad_LumpOffset(wadfile, i);
		const char *name = Wad_LumpName(wadfile, i);
		printf("%4i %-12.8s %-12i %-12i\n", i, name, offset, size);
	}
}

int main(int argc, const char * argv[])
{
	if(argc < 2)
	{
		printf("lswad <wadfile>\n");
		exit(0);
	}

	wadfile_t *wadfile = Wad_Open(argv[1]);
	if (!wadfile) {
		Error("failed to open wad file \'%s\'\n", argv[1]);
		return 1;
	}

	ListLumps(wadfile);

	Wad_Close(wadfile);
	
	return 0;
}
