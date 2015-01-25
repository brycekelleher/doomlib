#include "doomlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

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

// =============================================================
// triangle code

typedef struct trivert_s
{
	float	xyz[3];
	float	lightlevel;

} trivert_t;

static int	numvertices;
static int	numindicies;
static FILE	*binfile;

static void OpenTriangleModelFile(const char *filename)
{
	binfile = fopen(filename, "wb");
	if(!binfile)
	{
		Error("Couldn't open model file\n");
	}

	// reserve space for the vertex count
	fseek(binfile, 4 , SEEK_SET);
}

static void CloseTriangleModelFile()
{
	// emit the indicies
	numindicies = numvertices;
	fwrite(&numindicies, sizeof(int), 1, binfile);
	for(int i = 0; i < numindicies; i++)
	{
		fwrite(&i, sizeof(int), 1, binfile);
	}

	// write the actual vertex count
	fseek(binfile, 0 , SEEK_SET);
	fwrite(&numvertices, sizeof(int), 1, binfile);

	fclose(binfile);
}

static void EmitBinaryTriangle(trivert_t v0, trivert_t v1, trivert_t v2)
{
	// fixme: replace with EmitFloat?
	fwrite(&v0, sizeof(trivert_t), 1, binfile);
	fwrite(&v1, sizeof(trivert_t), 1, binfile);
	fwrite(&v2, sizeof(trivert_t), 1, binfile);
	fflush(binfile);

	numvertices += 3;
}

// interface to the triangle code
static void AddTriangle(trivert_t v0, trivert_t v1, trivert_t v2)
{
	//printf("adding triangle:\n");
	//printf("(%f %f %f)\n", v0.xyz[0], v0.xyz[1], v0.xyz[2]);
	//printf("(%f %f %f)\n", v1.xyz[0], v1.xyz[1], v1.xyz[2]);
	//printf("(%f %f %f)\n", v2.xyz[0], v2.xyz[1], v2.xyz[2]);

	EmitBinaryTriangle(v0, v1, v2);
}

// =============================================================
// geometry generation code

typedef struct leveldata_s
{
	dvertex_t	*vertices;
	dlinedef_t	*linedefs;
	dsidedef_t	*sidedefs;
	dsector_t	*sectors;
	dseg_t		*segs;
	dssector_t	*ssectors;
	dnode_t		*nodes;

	int		numnodes;

} leveldata_t;

// fixme:
static leveldata_t	leveldatastatic;
static leveldata_t	*leveldata = &leveldatastatic;

static float Fixed16ToFloat(int fixed)
{
	return (float)fixed;
}

static void SidedefsFromLinedef(dsidedef_t *s[2], dlinedef_t *l)
{
	for(int i = 0; i < 2; i++)
	{
		if(l->sidedefs[i] == -1)
		{
			s[i]	= NULL;
			continue;
		}

		s[i] = leveldata->sidedefs + l->sidedefs[i];
	}
}

static dnode_t *NodeFromNum(int num)
{
	return leveldata->nodes + num;
}

static dsector_t *SectorFromNum(int num)
{
	return leveldata->sectors + num;
}

static void GetLevelData(leveldata_t *d, const char *mapname)
{
	int baselump = Doom_LumpNumFromName(mapname);

	if(baselump < 0 || Doom_LumpLength(baselump) != 0)
	{
		Error("Map \"%s\" not found\n", mapname);
		exit(-1);
	}

	d->vertices	= (dvertex_t*)Doom_LumpFromNum(baselump + VERTICES_OFFSET);
	d->linedefs	= (dlinedef_t*)Doom_LumpFromNum(baselump + LINEDEFS_OFFSET);
	d->sidedefs	= (dsidedef_t*)Doom_LumpFromNum(baselump + SIDEDEFS_OFFSET);
	d->sectors	= (dsector_t*)Doom_LumpFromNum(baselump + SECTORS_OFFSET);
	d->segs		= (dseg_t*)Doom_LumpFromNum(baselump + SEGS_OFFSET);
	d->ssectors	= (dssector_t*)Doom_LumpFromNum(baselump + SSECTORS_OFFSET);
	d->nodes	= (dnode_t*)Doom_LumpFromNum(baselump + NODES_OFFSET);
	
	d->numnodes	= Doom_LumpLength(baselump + NODES_OFFSET) / sizeof(dnode_t);
}

static int SegSortFunc(const void *a, const void *b)
{
	dseg_t	*sa = *(dseg_t**)a;
	dseg_t	*sb = *(dseg_t**)b;



	int result = sb->vertices[0] - sa->vertices[1];

	//if(result < 0)
	//	result = -result;

	//return sa->vertices[0] - sb->vertices[0];

	return result;
}

static void SortSegs(dssector_t *ss)
{
	dseg_t *segarray[2048];

	dseg_t *seg	= leveldata->segs + ss->startseg;
	for(int i = 0; i < ss->numsegs; i++, seg++)
	{
		segarray[i] = seg;
	}

	qsort(segarray, ss->numsegs, sizeof(dseg_t*), SegSortFunc);

	for(int i = 0; i < ss->numsegs; i++)
	{
		seg = segarray[i];
		printf( "%i, (%i %i), start=(%f, %f), end=(%f, %f), offset=%i\n",
			i,
			seg->vertices[0],
			seg->vertices[1],
			Fixed16ToFloat(leveldata->vertices[seg->vertices[0]].xy[0]),
			Fixed16ToFloat(leveldata->vertices[seg->vertices[0]].xy[1]),
			Fixed16ToFloat(leveldata->vertices[seg->vertices[1]].xy[0]),
			Fixed16ToFloat(leveldata->vertices[seg->vertices[1]].xy[1]),
			seg->offset);
	}
}



//void EmitMiddleWallGeometry()
//void EmitUpperAndLowerWallGeometry()
//void EmitFloorAndCeilingGeometry()

// if single sided, emit middle polygon
// if double sided emit middle, lower and upper polygon
static void SegVertexData(float xy[2][2], dseg_t *seg)
{
	// get the vertex data for the seg
	if(seg->side == 0)
	{
		xy[0][0] = Fixed16ToFloat(leveldata->vertices[seg->vertices[0]].xy[0]);
		xy[0][1] = Fixed16ToFloat(leveldata->vertices[seg->vertices[0]].xy[1]);
		xy[1][0] = Fixed16ToFloat(leveldata->vertices[seg->vertices[1]].xy[0]);
		xy[1][1] = Fixed16ToFloat(leveldata->vertices[seg->vertices[1]].xy[1]);
	}
	else
	{
		xy[1][0] = Fixed16ToFloat(leveldata->vertices[seg->vertices[0]].xy[0]);
		xy[1][1] = Fixed16ToFloat(leveldata->vertices[seg->vertices[0]].xy[1]);
		xy[0][0] = Fixed16ToFloat(leveldata->vertices[seg->vertices[1]].xy[0]); 
		xy[0][1] = Fixed16ToFloat(leveldata->vertices[seg->vertices[1]].xy[1]); 
	}

	float dx, dy;
	dx = xy[1][0] - xy[0][0];
	dy = xy[1][1] - xy[0][1];

	// calculate a normal for the linedef
	float nx, ny;
	nx = dx / sqrtf((dx * dx) + (dy * dy));
	ny = dy / sqrtf((dx * dx) + (dy * dy));

	//xy[0][0] += (nx * seg->offset);
	//xy[0][1] += (ny * seg->offset);

	//xy[0][0] += sqrt((seg->offset * seg->offset) - (dy * dy));
	//xy[0][1] += sqrt((seg->offset * seg->offset) - (dx * dx));
	//float length = sqrtf((dx * dx) + (dy * dy));
	//xy[0][0] += dx * (seg->offset / length);
	//xy[0][1] += dy * (seg->offset / length);
}

// fixme: could reverse test
static void EmitMiddleWallGeometry(dseg_t *seg)
{
	dlinedef_t *ld		= leveldata->linedefs + seg->linedef;
	dsidedef_t *sd		= leveldata->sidedefs + ld->sidedefs[seg->side];
	dsector_t *sector	= leveldata->sectors + sd->sector;

	if(sd->textures[2][0] != '-')
	{
		// get the seg vertex data
		float xy[2][2];
		SegVertexData(xy, seg);
		
		// build the 4 triverts that we need
		trivert_t	trivert[4];

		trivert[0].xyz[0]	= xy[0][0];
		trivert[0].xyz[1]	= xy[0][1];
		trivert[0].xyz[2]	= sector->ceiling;
		trivert[0].lightlevel	= sector->lightlevel;

		trivert[1].xyz[0]	= xy[0][0];
		trivert[1].xyz[1]	= xy[0][1];
		trivert[1].xyz[2]	= sector->floor;
		trivert[1].lightlevel	= sector->lightlevel;

		trivert[2].xyz[0]	= xy[1][0];
		trivert[2].xyz[1]	= xy[1][1];
		trivert[2].xyz[2]	= sector->ceiling;
		trivert[2].lightlevel	= sector->lightlevel;

		trivert[3].xyz[0]	= xy[1][0];
		trivert[3].xyz[1]	= xy[1][1];
		trivert[3].xyz[2]	= sector->floor;
		trivert[3].lightlevel	= sector->lightlevel;

		// this is the order for a triangle strip
		//AddTriangle(trivert[0], trivert[1], trivert[2]);
		//AddTriangle(trivert[1], trivert[2], trivert[3]);

		// triangle soup, counter clockwise wound
		AddTriangle(trivert[0], trivert[1], trivert[2]);
		AddTriangle(trivert[3], trivert[2], trivert[1]);
	}

}

static void EmitUpperAndLowerWallGeometry(dseg_t *seg)
{
	dsidedef_t *sd[2];
	dsector_t *sectors[2];

	// only emit if this is a double sided line
	dlinedef_t *ld		= leveldata->linedefs + seg->linedef;

	if(ld->sidedefs[1] == -1)
		return;

	if(seg->side == 0)
	{
		sd[0]		= leveldata->sidedefs + ld->sidedefs[0];
		sd[1]		= leveldata->sidedefs + ld->sidedefs[1];
	}
	else
	{
		sd[1]		= leveldata->sidedefs + ld->sidedefs[0];
		sd[0]		= leveldata->sidedefs + ld->sidedefs[1];
	}

	sectors[0]	= leveldata->sectors + sd[0]->sector;
	sectors[1]	= leveldata->sectors + sd[1]->sector;

	if((sectors[0]->floor < sectors[1]->floor)) // && sd[0]->textures[1][0] != '-')
	{
		// build the 4 triverts that we need
		trivert_t	trivert[4];

		// get the seg vertex data
		float xy[2][2];
		SegVertexData(xy, seg);

		trivert[0].xyz[0]	= xy[0][0];
		trivert[0].xyz[1]	= xy[0][1];
		trivert[0].xyz[2]	= sectors[1]->floor;
		trivert[0].lightlevel	= sectors[0]->lightlevel;

		trivert[1].xyz[0]	= xy[0][0];
		trivert[1].xyz[1]	= xy[0][1];
		trivert[1].xyz[2]	= sectors[0]->floor;
		trivert[1].lightlevel	= sectors[0]->lightlevel;

		trivert[2].xyz[0]	= xy[1][0];
		trivert[2].xyz[1]	= xy[1][1];
		trivert[2].xyz[2]	= sectors[1]->floor;
		trivert[2].lightlevel	= sectors[0]->lightlevel;

		trivert[3].xyz[0]	= xy[1][0];
		trivert[3].xyz[1]	= xy[1][1];
		trivert[3].xyz[2]	= sectors[0]->floor;
		trivert[3].lightlevel	= sectors[0]->lightlevel;

		//AddTriangle(trivert[0], trivert[1], trivert[2]);
		//AddTriangle(trivert[1], trivert[2], trivert[3]);

		// triangle soup, counter clockwise wound
		AddTriangle(trivert[0], trivert[1], trivert[2]);
		AddTriangle(trivert[3], trivert[2], trivert[1]);
	}

	if((sectors[0]->ceiling > sectors[1]->ceiling)) // && sd[0]->textures[0][0] != '-')
	{
		// build the 4 triverts that we need
		trivert_t	trivert[4];

		// get the seg vertex data
		float xy[2][2];
		SegVertexData(xy, seg);

		trivert[0].xyz[0]	= xy[0][0];
		trivert[0].xyz[1]	= xy[0][1];
		trivert[0].xyz[2]	= sectors[1]->ceiling;
		trivert[0].lightlevel	= sectors[0]->lightlevel;

		trivert[1].xyz[0]	= xy[0][0];
		trivert[1].xyz[1]	= xy[0][1];
		trivert[1].xyz[2]	= sectors[0]->ceiling;
		trivert[1].lightlevel	= sectors[0]->lightlevel;

		trivert[2].xyz[0]	= xy[1][0];
		trivert[2].xyz[1]	= xy[1][1];
		trivert[2].xyz[2]	= sectors[1]->ceiling;
		trivert[2].lightlevel	= sectors[0]->lightlevel;

		trivert[3].xyz[0]	= xy[1][0];
		trivert[3].xyz[1]	= xy[1][1];
		trivert[3].xyz[2]	= sectors[0]->ceiling;
		trivert[3].lightlevel	= sectors[0]->lightlevel;

		//AddTriangle(trivert[0], trivert[1], trivert[2]);
		//AddTriangle(trivert[1], trivert[2], trivert[3]);

		// triangle soup, counter clockwise wound
		AddTriangle(trivert[0], trivert[1], trivert[2]);
		AddTriangle(trivert[3], trivert[2], trivert[1]);
	}
}

static void ProcessSubSector(dssector_t *ss)
{
	int i;

#if 0
	{
		printf("ssector: ------------------------------------\n");
		dseg_t *seg	= leveldata->segs + ss->startseg;
		for(i = 0; i < ss->numsegs; i++, seg++)
		{
			dlinedef_t *ld = leveldata->linedefs + seg->linedef;
			printf("seg: %i: vertices=(%i %i) levertices(%i %i) offset=%i\n",
					i,
					seg->vertices[0],
					seg->vertices[1],
					ld->vertices[0],
					ld->vertices[1],
					seg->offset);
		}
	}
#endif	

	{
		dseg_t *seg	= leveldata->segs + ss->startseg;
		for(i = 0; i < ss->numsegs; i++, seg++)
		{
			EmitMiddleWallGeometry(seg);
		}
	}

	{
		dseg_t *seg	= leveldata->segs + ss->startseg;
		for(i = 0; i < ss->numsegs; i++, seg++)
		{
			EmitUpperAndLowerWallGeometry(seg);
		}
	}

}

static void WalkNodesRecursive(short nodenum)
{
	if(nodenum & 0x8000)
	{
		//SortSegs(leveldata->ssectors + (nodenum & 0x7fff));

		// this is a subsector
		ProcessSubSector(leveldata->ssectors + (nodenum & 0x7fff));

		return;
	}

	dnode_t	*n = leveldata->nodes + nodenum;

	WalkNodesRecursive(n->children[0]);
	WalkNodesRecursive(n->children[1]);
}

static void WalkNodes()
{
	//WalkNodesRecursive(0);
	
	WalkNodesRecursive(leveldata->numnodes - 1);
}

static void PrintUsage()
{
	printf("dumptri <wadfile> <mapname>\n");
}

int main(int argc, const char * argv[])
{
	if(argc == 1)
	{
		PrintUsage();
		exit(0);
	}

	Doom_ReadWadFile(argv[1]);

	GetLevelData(leveldata, argv[2]);

	{
		OpenTriangleModelFile("tris.mdl");
		
		WalkNodes();

		CloseTriangleModelFile();
	}

	Doom_CloseAll();
	
	return 0;
}

