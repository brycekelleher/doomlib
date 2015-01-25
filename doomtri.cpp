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

typedef struct leveldata_s
{
	dvertex_t	*vertices;
	dlinedef_t	*linedefs;
	dsidedef_t	*sidedefs;
	dsector_t	*sectors;
	dseg_t		*segs;
	dssector_t	*ssectors;
	dnode_t		*nodes;

} leveldata_t;

// fixme:
static leveldata_t	leveldatastatic;
static leveldata_t	*leveldata = &leveldatastatic;

static float Fixed1616ToFloat(int fixed)
{
	float f;

	f = 0;
	f += (float)(fixed / 0x10000);
	f += (float)(fixed & 0x0000ffff) / 0x10000;
	
	return f;
}

static void GetSidedefsForLinedef(dsidedef_t *s[2], dlinedef_t *l)
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
}

typedef struct trivert_s
{
	float	xyz[3];
	float	lightlevel;

} trivert_t;

// interface to the triangle code
static void AddTriangle(trivert_t v0, trivert_t v1, trivert_t v2)
{
	printf("adding triangle:\n");
	printf("(%f %f %f)\n", v0.xyz[0], v0.xyz[1], v0.xyz[2]);
	printf("(%f %f %f)\n", v1.xyz[0], v1.xyz[1], v1.xyz[2]);
	printf("(%f %f %f)\n", v2.xyz[0], v2.xyz[1], v2.xyz[2]);
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
			Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[0]),
			Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[1]),
			Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[0]),
			Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[1]),
			seg->offset);
	}
}

//void EmitMiddleWallGeometry()
//void EmitUpperAndLowerWallGeometry()
//void EmitFloorAndCeilingGeometry()

// if single sided, emit middle polygon
// if double sided emit middle, lower and upper polygon
static void ProcessSubSector(dssector_t *ss)
{
	//printf("processing subsector:\n");

	dseg_t *seg	= leveldata->segs + ss->startseg;
		
	for(int i = 0; i < ss->numsegs; i++, seg++)
	{
		//printf( "%i, (%i %i), start=(%f, %f), end=(%f, %f), offset=%i\n",
		//	i,
		//	seg->vertices[0],
		//	seg->vertices[1],
		//	Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[0]),
		//	Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[1]),
		//	Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[0]),
		//	Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[1]),
		//	seg->offset);

		dlinedef_t *ld		= leveldata->linedefs + seg->linedef;
		dsidedef_t *sd		= leveldata->sidedefs + ld->sidedefs[seg->side];
		dsector_t *sector	= leveldata->sectors + sd->sector;

		bool doublesided = (ld->sidedefs[1] != -1);

		// middle wall
		if(sd->textures[2][0] != '-')
		{
			// emit middle polygons
			float xy[2][2];

			// get the vertex data for the seg
			if(seg->side == 0)
			{
				xy[0][0] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[0]);
				xy[0][1] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[1]);
				xy[1][0] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[0]);
				xy[1][1] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[1]);
			}
			else
			{
				xy[1][0] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[0]);
				xy[1][1] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[0]].xy[1]);
				xy[0][0] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[0]); 
				xy[0][1] = Fixed1616ToFloat(leveldata->vertices[seg->vertices[1]].xy[1]); 
			}

			float dx, dy;
			dx = xy[1][0] - xy[0][0];
			dy = xy[1][1] - xy[0][1];
			
			// calculate a normal for the linedef
			float nx, ny;
			nx = dx / sqrtf((dx * dx) + (dy * dy));
			ny = dy / sqrtf((dx * dx) + (dy * dy));

			xy[0][0] += (nx * seg->offset);
			xy[0][1] += (ny * seg->offset);
	
			float lightlevel	= sector->lightlevel;
			// build the 4 triverts that we need
			trivert_t	trivert[4];

			trivert[0].xyz[0]	= xy[0][0];
			trivert[0].xyz[1]	= xy[0][1];
			trivert[0].xyz[2]	= sector->ceiling;
			trivert[0].lightlevel	= lightlevel;

			trivert[1].xyz[0]	= xy[1][0];
			trivert[1].xyz[1]	= xy[1][1];
			trivert[1].xyz[2]	= sector->floor;
			trivert[1].lightlevel	= lightlevel;

			trivert[2].xyz[0]	= xy[0][0];
			trivert[2].xyz[1]	= xy[0][1];
			trivert[2].xyz[2]	= sector->ceiling;
			trivert[2].lightlevel	= lightlevel;

			trivert[3].xyz[0]	= xy[1][0];
			trivert[3].xyz[1]	= xy[1][1];
			trivert[3].xyz[2]	= sector->floor;
			trivert[3].lightlevel	= lightlevel;

			AddTriangle(trivert[0], trivert[1], trivert[2]);
			AddTriangle(trivert[1], trivert[2], trivert[3]);
		}

		if(doublesided)
		{
			// if this sectors floor height is lower than the other side emit a lower polygon
			// if this sectors ceiling height is greater than the other side emit an upper polygon
		}
	}

	printf("\n");
}

static void WalkNodes(short nodenum)
{
	if(nodenum & 0x8000)
	{
		//SortSegs(leveldata->ssectors + (nodenum & 0x7fff));

		// this is a subsector
		ProcessSubSector(leveldata->ssectors + (nodenum & 0x7fff));
		return;
	}

	dnode_t	*n = leveldata->nodes + nodenum;

	WalkNodes(n->children[0]);
	WalkNodes(n->children[1]);
}

#if 0
static void WalkNodes(dnode_t *n)
{
	if(!(n->children[0] & (0x8000)))
	{
		WalkNodes(leveldata->nodes + (n->children[0] & 0x7fff));
	}
	if(!(n->children[0] & (0x8000)))
	{
		WalkNodes(leveldata->nodes + (n->children[0] & 0x7fff));
	}

	// this is a sub-sector
}
#endif

#if 0
// walk all the sectors as we want to generate data in order to maxmise the possibility of stripping
static void ProcessLinedefs(dsector_t* s)
{
	linedef_t *ld;
	sidedef_t *sd[2];
	

	l->sidedefs

	if(!sd[1])
	{
		// emit a wall triangle
	}
}

static void GenerateData()
{
	void ProcessSector(s);
}
#endif

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

	WalkNodes(0);
	
	Doom_CloseAll();
	
	return 0;
}

