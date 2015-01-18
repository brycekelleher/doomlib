#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// file structures
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char	identification[4];
    int		numlumps;
    int		infotableofs;
    
} wadheader_t;

typedef struct
{
    int		filepos;
    int		size;
    char	name[8];
    
} filelump_t;

// internal structures
typedef struct
{
    FILE	*fp;
    char        name[8];
    int         filepos;
    int         size;
} lumpinfo_t;

#define MAX_LUMPS	32 * 1024

// internal lump data
static int		numlumps;
static lumpinfo_t	lumpdir[MAX_LUMPS];
static void		*lumpdata[MAX_LUMPS];
// fixme: add list of files

void *Doom_Malloc(int numbytes)
{
	return malloc(numbytes);
}

// actually read the bytes of the lump
void Doom_ReadLump(int lumpnum)
{
	lumpinfo_t	*lumpinfo;

	lumpinfo = lumpdir + lumpnum;

	// don't load the lump if it's already been loaded or if it has zero size
	if(lumpdata[lumpnum])
		return;
	if(!lumpinfo->size)
		return;

	// allocate memory for the lump
	lumpdata[lumpnum] = Doom_Malloc(lumpinfo->size);

	// read the lump data
	fseek(lumpinfo->fp, lumpinfo->filepos, SEEK_SET);
	fread(lumpdata[lumpnum], sizeof(unsigned char), lumpinfo->size, lumpinfo->fp);
}

void Doom_ReadWadFile(const char *filename)
{
	FILE *fp;

	fp = fopen(filename, "rb");

	if(!fp)
	{
		printf("Failed to open wad file\n");
		exit(-1);
	}

	// read the header
	wadheader_t header;
	fread(&header, sizeof(wadheader_t), 1, fp);

	// read the lump info table
	fseek(fp, header.infotableofs, SEEK_SET);

	// iterate through the lumps and add the directory
	for(int i = 0; i < header.numlumps; i++)
	{
		filelump_t	filelump;
		lumpinfo_t	*lumpinfo; 

		// read the lump info
		fread(&filelump, sizeof(filelump_t), 1, fp);

		// allocate an entry from the lump directory
		lumpinfo = lumpdir + numlumps;
		numlumps++;

		lumpinfo->fp        = fp;
		lumpinfo->filepos	= filelump.filepos;
		lumpinfo->size		= filelump.size;
		strncpy(lumpinfo->name, filelump.name, 8);

		//printf("lump %08d: name=%-8s pos=%d size=%d\n", i, filelump.name, filelump.filepos, filelump.size);
	}

	// re-read all the lumps
	for(int i = 0; i < numlumps; i++)
	{
		if(lumpdata[i])
			continue;

		Doom_ReadLump(i);
	}
}

int Doom_LumpLength(int lumpnum)
{
	lumpinfo_t *l = lumpdir + lumpnum;

	return l->size;
}

void *Doom_LumpFromNum(int lumpnum)
{
	// range check the lump number
	if(lumpnum < 0 || lumpnum > numlumps)
		return NULL;

	Doom_ReadLump(lumpnum);
	
	return lumpdata[lumpnum];
}

int Doom_LumpNumFromName(const char *lumpname)
{
	lumpinfo_t	*l;

	for(l = lumpdir; l < lumpdir + numlumps; l++)
	{
		if(!strncmp(lumpname, l->name, 8))
			return (int)(l - lumpdir);
	}

	return -1;
}

void *Doom_LumpFromName(const char *lumpname)
{
	return Doom_LumpFromNum(Doom_LumpNumFromName(lumpname));
}

