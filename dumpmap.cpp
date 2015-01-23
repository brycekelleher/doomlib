#include "doomlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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

static float Fixed1616ToFloat(int fixed)
{
	float f;

	f = 0;
	f += (float)(fixed / 0x10000);
	f += (float)(fixed & 0x0000ffff) / 0x10000;
	
	return f;
}

static void DumpVertices(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	// vertices are 16.16 fixed point
	int numvertices		= lumpsize / (2 * sizeof(int));
	int *fixedptr		= (int*)data;

	for(int i = 0; i < numvertices; i++)
	{
		float xy[2];

		xy[0] = Fixed1616ToFloat(fixedptr[0]);
		xy[1] = Fixed1616ToFloat(fixedptr[1]);
		fixedptr += 2;

		printf("vertex %4i: %12.4f, %12.4f\n", i, xy[0], xy[1]);
	}
}

static void DumpLinedefs(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	int numlinedefs		= lumpsize / sizeof(dlinedef_t);
	dlinedef_t *lptr	= (dlinedef_t*)data;

	for(int i = 0; i < numlinedefs; i++)
	{
		printf("linedefs %4i:", i);
		printf("\n\tvertices (%i %i)", lptr->vertices[0], lptr->vertices[1]);
		printf("\n\tsidedefs (%i %i)", lptr->sidedefs[0], lptr->sidedefs[1]);
		printf("\n");

		lptr++;
	}
}  

static void DumpSidedefs(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	int numsidedefs		= lumpsize / sizeof(dsidedef_t);
	dsidedef_t *sptr	= (dsidedef_t*)data;

	for(int i = 0; i < numsidedefs; i++)
	{
		printf("sidedef %4i:", i);
		printf("\n\toffset (%i %i)", sptr->xoffset, sptr->yoffset);
		printf("\n\ttextures (|%-8.8s|%-8.8s|%-8.8s|)", sptr->textures[0], sptr->textures[1], sptr->textures[2]);
		printf("\n\tsector %i", sptr->sector);
		printf("\n");
		
		sptr++;
	}
}

static void DumpSectors(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	int numsectors		= lumpsize / sizeof(dsector_t);
	dsector_t *sptr		= (dsector_t*)data;

	for(int i = 0; i < numsectors; i++)
	{
		printf("sector %4i:", i);
		printf("\n\tfloor %i, ceiling %i", sptr->floor, sptr->ceiling);
		printf("\n\ttextures (|%-.8s|%-.8s|)", sptr->textures[0], sptr->textures[1]);
		printf("\n\tlightlevel %i", sptr->lightlevel);
		printf("\n\tflags %#04x", sptr->flags);
		printf("\n\ttag %i", sptr->tag);
		printf("\n");
		
		sptr++;
	}
}

static void DumpMapData(const char *mapname)
{
	int baselump = Doom_LumpNumFromName(mapname);

	if(baselump < 0 || Doom_LumpLength(baselump) != 0)
	{
		Error("Map \"%s\" not found\n", mapname);
		exit(-1);
	}

	DumpLinedefs(baselump + LINEDEFS_OFFSET);
	DumpSidedefs(baselump + SIDEDEFS_OFFSET);
	DumpVertices(baselump + VERTICES_OFFSET);
	DumpSectors(baselump + SECTORS_OFFSET);
}

static void PrintUsage()
{
	printf("dumplvl <wadfile> <mapname>\n");
}

int main(int argc, const char * argv[])
{
	if(argc == 1)
	{
		PrintUsage();
		exit(0);
	}

	Doom_ReadWadFile(argv[1]);

	DumpMapData(argv[2]);

	Doom_CloseAll();
	
	return 0;
}
