#include <stdio.h>

void Doom_ReadWadFile(const char *filename);

int Doom_LumpLength(int lumpnum);
void *Doom_LumpFromNum(int lumpnum);

int Doom_LumpNumFromName(const char *lumpname);
void *Doom_LumpFromName(const char *lumpname);

#define THINGS_OFFSET		1
#define LINEDEFS_OFFSET		2
#define	SIDEDEFS_OFFSET		3
#define VERTICES_OFFSET		4
#define SSEGS_OFFSET		5
#define SSECTORS_OFFSET		6
#define NODES_OFFSET		7
#define SECTORS_OFFSET		8
#define REJECT_OFFSET		9
#define BLOCK_OFFSET		10

typedef struct
{
	short	x;
	short	y;
	short	angle;
	short	type;
	short	flags;
} dthing_t;

typedef struct
{
	short	vertices[2];
	short	pad0;
	short	pad1;
	short	pad2;
	short	sidedefs[2];

} dlinedef_t;

typedef struct
{
	short	xoffset;
	short	yoffset;
	char	textures[3][8];
	short	sector;

} dsidedef_t;

typedef struct
{
	short	floor;
	short	ceiling;
	char	textures[2][8];
	short	lightlevel;
	short	flags;
	short	tag;

} dsector_t;

float Fixed1616ToFloat(int fixed)
{
	float f;

	f = 0;
	f += (float)(fixed / 0x10000);
	f += (float)(fixed & 0x0000ffff) / 0x10000;
	
	return f;
}

void TryDumpVertices(int lumpnum)
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

		printf("vertex %-6i: %12.4f, %12.4f\n", i, xy[0], xy[1]);
	}
}

void TryDumpLinedefs(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	int numlinedefs		= lumpsize / sizeof(dlinedef_t);
	dlinedef_t *lptr	= (dlinedef_t*)data;

	for(int i = 0; i < numlinedefs; i++)
	{
		printf("linedefs %-6i:", i);
		printf("\n\tvertices (%i %i)", lptr->vertices[0], lptr->vertices[1]);
		printf("\n\tsidedefs (%i %i)", lptr->sidedefs[0], lptr->sidedefs[1]);
		printf("\n");

		lptr++;
	}
}  

void TryDumpSidedefs(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	int numsidedefs		= lumpsize / sizeof(dsidedef_t);
	dsidedef_t *sptr	= (dsidedef_t*)data;

	for(int i = 0; i < numsidedefs; i++)
	{
		printf("sidedef %-6i:", i);
		printf("\n\t offset (%i %i)", sptr->xoffset, sptr->yoffset);
		printf("\n\t textures (|%-8.8s|%-8.8s|%-8.8s|)", sptr->textures[0], sptr->textures[1], sptr->textures[2]);
		printf("\n\t sector %i", sptr->sector);
		printf("\n");
		
		sptr++;
	}
}

void TryDumpSectors(int lumpnum)
{
	void	*data;
	int 	lumpsize;

	data			= Doom_LumpFromNum(lumpnum);
	lumpsize		= Doom_LumpLength(lumpnum);

	int numsectors		= lumpsize / sizeof(dsector_t);
	dsector_t *sptr		= (dsector_t*)data;

	for(int i = 0; i < numsectors; i++)
	{
		printf("sector %-6i:", i);
		printf("\n\t floor %i, ceiling %i", sptr->floor, sptr->ceiling);
		printf("\n\t textures (|%-.8s|%-.8s|)", sptr->textures[0], sptr->textures[1]);
		printf("\n\t lightlevel %i", sptr->lightlevel);
		printf("\n\t flags %#04x", sptr->flags);
		printf("\n\t tag %i", sptr->tag);
		printf("\n");
		
		sptr++;
	}
}

int main(int argc, const char * argv[])
{
	const char *wadfilename = "doom1.wad";

	if(argv[1])
		wadfilename = argv[1];

	Doom_ReadWadFile(wadfilename);
	
	int baselump = Doom_LumpNumFromName("E1M1");
	TryDumpLinedefs(baselump + LINEDEFS_OFFSET);
	TryDumpSidedefs(baselump + SIDEDEFS_OFFSET);
	TryDumpVertices(baselump + VERTICES_OFFSET);
	TryDumpSectors(baselump + SECTORS_OFFSET);
	
	return 0;
}
