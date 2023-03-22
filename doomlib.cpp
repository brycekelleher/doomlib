#include "doomlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// internal structures
typedef struct
{
    FILE        *fp;
    char        name[8];
    int         filepos;
    int         size;
} lumpinfo_t;

#define MAX_LUMPS       32 * 1024
#define MAX_FILES        1 * 1024

static const char* wadname;

// internal lump data
static int              numlumps;
static lumpinfo_t       lumpdir[MAX_LUMPS];
static void             *lumpdata[MAX_LUMPS];
static int              numfiles;
static FILE             *files[MAX_FILES];

static void *Doom_Malloc(int numbytes)
{
        return malloc(numbytes);
}

static int Doom_AddLump(char name[8], FILE *fp, int filepos, int size)
{
        lumpinfo_t      *lumpinfo;

        // allocate an entry from the lump directory
        lumpinfo = lumpdir + numlumps;
        numlumps++;

        lumpinfo->fp            = fp;
        lumpinfo->filepos       = filepos;
        lumpinfo->size          = size;
        strncpy(lumpinfo->name, name, 8);

        return numlumps - 1;
}
        

// actually read the bytes of the lump
static void Doom_ReadLump(int lumpnum)
{
        lumpinfo_t      *lumpinfo;

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

int Doom_LumpLength(int lumpnum)
{
        lumpinfo_t *l = lumpdir + lumpnum;

        return l->size;
}

int Doom_LumpNumFromName(const char *lumpname)
{
        lumpinfo_t      *l;

        for(l = lumpdir; l < lumpdir + numlumps; l++)
        {
                if(!strncmp(lumpname, l->name, 8))
                        return (int)(l - lumpdir);
        }

        return -1;
}

void *Doom_LumpFromNum(int lumpnum)
{
        // range check the lump number
        if(lumpnum < 0 || lumpnum > numlumps)
                return NULL;

        Doom_ReadLump(lumpnum);
        
        return lumpdata[lumpnum];
}

void *Doom_LumpFromName(const char *lumpname)
{
        return Doom_LumpFromNum(Doom_LumpNumFromName(lumpname));
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

        wadname = filename;

        files[numfiles] = fp;
        numfiles++;

        // read the header
        dwadheader_t header;
        fread(&header, sizeof(dwadheader_t), 1, fp);

        // read the lump info table
        fseek(fp, header.infotableofs, SEEK_SET);

        // iterate through the lumps and add the directory
        for(int i = 0; i < header.numlumps; i++)
        {
                dfilelump_t     filelump;
                lumpinfo_t      *lumpinfo; 

                // read the lump info
                fread(&filelump, sizeof(dfilelump_t), 1, fp);

                // allocate an entry from the lump directory
                lumpinfo = lumpdir + numlumps;
                numlumps++;

                lumpinfo->fp        = fp;
                lumpinfo->filepos       = filelump.filepos;
                lumpinfo->size          = filelump.size;
                strncpy(lumpinfo->name, filelump.name, 8);
        }

        // re-read all the lumps
        for(int i = 0; i < numlumps; i++)
        {
                if(lumpdata[i])
                        continue;

                Doom_ReadLump(i);
        }
}

void Doom_IterateLumps(void (*callback)(int lumpnum, char name[8], int size))
{
        lumpinfo_t      *l;

        for(l = lumpdir; l < lumpdir + numlumps; l++)
                callback(l - lumpdir, l->name, l->size);
}

void Doom_CloseAll()
{
        int     i;

        for(i = 0; i < numfiles; i++)
        {
                fclose(files[i]);
        }

        for(i = 0; i < numlumps; i++)
        {
                if(!lumpdata[i])
                        continue;

                free(lumpdata[i]);
        }
}



typedef struct wadfile_s
{
        FILE            *fp;
        int             numlumps;
        lumpinfo_t      *lumpinfo;
} wadfile_t;


static int32_t ReadInt32(FILE *fp)
{
        int data;
        fread(&data, sizeof(int32_t), 1, fp);

        return data;
}

static uint32_t ReadBytes(void *buffer, int count, FILE *fp)
{
    return fread(buffer, count, 1, fp);
}

wadfile_t *Wad_Open(const char *filename)
{
        // open the wad file
        FILE *fp = fopen(filename, "rb");
        if(!fp) {
                return NULL;
        }

        // read the header
        char id[4];
        ReadBytes(&id, 4, fp);

        if (!strncmp(id, "iwad", 4) || !strncmp(id, "pwad", 4)) {
                fclose(fp);
                return NULL;
        }

        int numlumps = ReadInt32(fp);
        int infotableofs = ReadInt32(fp);

        // allocate the wad file pointer
        wadfile_t *wadfile = (wadfile_t*)malloc(sizeof(wadfile_t));
        wadfile->fp = fp;
        wadfile->lumpinfo = (lumpinfo_t*)malloc(sizeof(lumpinfo_t) * numlumps);
        wadfile->numlumps = numlumps;
        
        // read the info table
        fseek(fp, infotableofs, SEEK_SET);

        for(int i = 0; i < numlumps; i++) {
                // allocate an entry from the lump directory
                lumpinfo_t *lumpinfo = wadfile->lumpinfo + i;
 
                // read the lumpinfo data
                lumpinfo->filepos       = ReadInt32(fp);
                lumpinfo->size          = ReadInt32(fp);
                ReadBytes(lumpinfo->name, 8, fp);
        }

        return wadfile;
}

void Wad_Close(wadfile_t *wadfile)
{
        fclose(wadfile->fp);
        free(wadfile->lumpinfo);
        free(wadfile);
}

int Wad_NumLumps(wadfile_t *wadfile)
{
        return wadfile->numlumps;
}

int Wad_LumpSize(wadfile_t *wadfile, int lumpnum)
{
        return wadfile->lumpinfo[lumpnum].size;
}

int Wad_LumpOffset(wadfile_t *wadfile, int lumpnum)
{
        return wadfile->lumpinfo[lumpnum].filepos;
}

const char* Wad_LumpName(wadfile_t *wadfile, int lumpnum)
{
        return wadfile->lumpinfo[lumpnum].name;
}

int Wad_LumpNumFromName(wadfile_t *wadfile, const char *lumpname)
{
        for(int i = 0; i < wadfile->numlumps; i++) {
                if(!strncmp(lumpname, wadfile->lumpinfo[i].name, 8)) {
                        return i;
                }
        }

        return -1;
}

void* Wad_ReadLump(wadfile_t *wadfile, int lumpnum)
{
        lumpinfo_t* lumpinfo = wadfile->lumpinfo + lumpnum;

        // allocate memory for the lump
        void* buffer = malloc(lumpinfo->size);

        // seek to the data position
        fseek(wadfile->fp, lumpinfo->filepos, SEEK_SET);

        // copy it out from the file
        fread(buffer, lumpinfo->size, 1, wadfile->fp);
        
        return buffer;
}

void Wad_FreeLump(unsigned char *data)
{
        free(data);
}

