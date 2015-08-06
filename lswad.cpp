#include <stdlib.h>
#include <stdio.h>
#include "doomlib.h"

void IterateCallback(int lumpnum, char name[8], int size)
{
	printf("%4i %-12.8s %i / %#x\n", lumpnum, name, size, size);
}

int main(int argc, const char * argv[])
{
	if(argc < 2)
	{
		printf("lswad <wadfile>\n");
		exit(0);
	}

	Doom_ReadWadFile(argv[1]);

	Doom_IterateLumps(IterateCallback);

	Doom_CloseAll();
	
	return 0;
}
