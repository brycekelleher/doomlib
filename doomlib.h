#ifndef __DOOMLIB_H__
#define __DOOMLIB_H__

// wad / lump interface
int Doom_LumpLength(int lumpnum);
int Doom_LumpNumFromName(const char *lumpname);
void *Doom_LumpFromNum(int lumpnum);
void *Doom_LumpFromName(const char *lumpname);
void Doom_ReadWadFile(const char *filename);
void Doom_IterateLumps(void (*callback)(int lumpnum, char name[8], int size));
void Doom_CloseAll();

#define THINGS_OFFSET		1
#define LINEDEFS_OFFSET		2
#define	SIDEDEFS_OFFSET		3
#define VERTICES_OFFSET		4
#define SEGS_OFFSET		5
#define SSECTORS_OFFSET		6
#define NODES_OFFSET		7
#define SECTORS_OFFSET		8
#define REJECT_OFFSET		9
#define BLOCK_OFFSET		10

// file structures
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char	identification[4];
    int		numlumps;
    int		infotableofs;
    
} dwadheader_t;

typedef struct
{
    int		filepos;
    int		size;
    char	name[8];
    
} dfilelump_t;

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
	short	xy[2];

} dvertex_t;

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

typedef struct
{ 
	short	vertices[2];
	short	angle;
	short	linedef;
	short	side;
	short	offset;
} dseg_t;

typedef struct
{
	short	numsegs;
	short	startseg;

} dssector_t;

typedef struct
{
	short	xy[2];
	short	dxdy[2];
	short	bounds[8];
	short	children[2];

} dnode_t;

#endif
