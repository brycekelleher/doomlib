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

static void DumpLump(wadfile_t *wadfile, int lumpnum)
{
	FILE *fp = stdout;

	int size = Wad_LumpSize(wadfile, lumpnum);
	if(!size || size == -1) {
		Error("invalid lump size\n");
	}

	// read the data block
	unsigned char *data = (unsigned char*)Wad_ReadLump(wadfile, lumpnum);
	if(!data) {
		Error("couldn't read lump data\n");
	}

	// dump the data out
	unsigned char *p = data;
	while(size--) {
		fputc(*p++, fp);
	}

	Wad_FreeLump(data);
}

int main(int argc, const char * argv[])
{
	if(argc < 3) {
		printf("dumpwad <wadfile> <lumpname>\n");
		exit(0);
	}

	//Doom_ReadWadFile(argv[1]);
	// open the wad file
	wadfile_t *wadfile = Wad_Open(argv[1]);
	if (!wadfile) {
		Error("failed to open wad file \'%s\'\n", argv[1]);
		return 1;
	}

	for (int i = 2; i < argc; i++) {
		int lumpnum = Wad_LumpNumFromName(wadfile, argv[i]);
		if (lumpnum == -1) {
			Error("couldn't find lump %s\n", argv[i]);
		}

		DumpLump(wadfile, lumpnum);
	}

	Wad_Close(wadfile);
	
	return 0;
}
