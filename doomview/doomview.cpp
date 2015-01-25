#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <unistd.h>
#include <sys/time.h>

#ifdef WIN32
//#include <windows.h>
#winclude "freeglut/include/GL/freeglut.h"
#else
#include <GL/freeglut.h>
#endif

#define PI 3.14159265358979323846f

static char*	filename;

static int oldtime;
static int realtime;
static int framenum;

// Input
typedef struct input_s
{
	int mousepos[2];
	int moused[2];
	bool lbuttondown;
	bool rbuttondown;
	bool keys[256];

} input_t;

static int mousepos[2];
static input_t input;

// ==============================================
// timing

#ifdef __APPLE__
unsigned int Sys_Milliseconds (void)
{
	struct timeval tp;
	static int		secbase;
	static int	curtime;

	gettimeofday(&tp, NULL);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
	}

	curtime = (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
	
	return curtime;
}

void Sys_Sleep(unsigned int msecs)
{
	usleep(msecs * 1000);
}
#endif

#ifdef WIN32
unsigned int Sys_Milliseconds(void)
{
	static int basetime;
	static int curtime;

	// initialize the base time
	if(!basetime)
	{
		basetime = timeGetTime();
	}

	curtime = timeGetTime() - basetime;

	return curtime;
}

void Sys_Sleep(unsigned int msecs)
{
	Sleep(msecs);
}
#endif

// ==============================================
// memory allocation

#define MEM_ALLOC_SIZE	32 * 1024 * 1024

typedef struct memstack_s
{
	unsigned char mem[MEM_ALLOC_SIZE];
	int allocated;

} memstack_t;

static memstack_t memstack;

void *Mem_Alloc(int numbytes)
{
	unsigned char *mem;
	
	if(memstack.allocated + numbytes > MEM_ALLOC_SIZE)
	{
		printf("Error: Mem: no free space available\n");
		abort();
	}

	mem = memstack.mem + memstack.allocated;
	memstack.allocated += numbytes;

	return mem;
}

void Mem_FreeStack()
{
	memstack.allocated = 0;
}

// ==============================================
// errors and warnings

static void Error(const char *error, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, error);
	vsprintf(buffer, error, valist);
	va_end(valist);

	printf("Error: %s", buffer);
	exit(1);
}

static void Warning(const char *warning, ...)
{
	va_list valist;
	char buffer[2048];

	va_start(valist, warning);
	vsprintf(buffer, warning, valist);
	va_end(valist);

	fprintf(stdout, "Warning: %s", buffer);
}

// ==============================================
// Misc crap

static float Vector_Dot(float a[3], float b[3])
{
	return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

static void Vector_Copy(float *a, float *b)
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
}

static void Vector_Cross(float *c, float *a, float *b)
{
	c[0] = (a[1] * b[2]) - (a[2] * b[1]); 
	c[1] = (a[2] * b[0]) - (a[0] * b[2]); 
	c[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

static void Vector_Normalize(float *v)
{
	float len, invlen;

	len = sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
	invlen = 1.0f / len;

	v[0] *= invlen;
	v[1] *= invlen;
	v[2] *= invlen;
}

static void Vector_Lerp(float *result, float *from, float *to, float t)
{
	result[0] = ((1 - t) * from[0]) + (t * to[0]);
	result[1] = ((1 - t) * from[1]) + (t * to[1]);
	result[2] = ((1 - t) * from[2]) + (t * to[2]);
}

static void MatrixTranspose(float out[4][4], const float in[4][4])
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			out[j][i] = in[i][j];
		}
	}
}

//==============================================
// model code

typedef struct modelvert_s
{
	float xyz[3];
	float lightlevel[3];

} modelvert_t;

typedef unsigned int modelindex_t;

typedef struct trisurf_s
{
	int 			numvertices;
	modelvert_t		*vertices;

	int				numindicies;
	modelindex_t	*indicies;

} trisurf_t;

static int numtrisurfs;
static trisurf_t trisurfs[16384];

static trisurf_t *AllocSurface()
{
	trisurf_t *trisurf;

	trisurf = trisurfs + numtrisurfs;
	numtrisurfs++;

	return trisurf;
}

static void FreeAllSurfaces()
{
	numtrisurfs = 0;
}

static int ReadInt(FILE *fp)
{
	int data;
	fread(&data, sizeof(int), 1, fp);

	return data;
}

static unsigned int ReadUnsignedInt(FILE *fp)
{
	unsigned int data;
	fread(&data, sizeof(unsigned int), 1, fp);

	return data;
}

static float ReadFloat(FILE *fp)
{
	float data;
	fread(&data, sizeof(float), 1, fp);

	return data;
}

static void ReadSurfaces()
{
	int i, j;

	// free all trisurfs currently allocated
	FreeAllSurfaces();

	// free any allocated stack memory
	Mem_FreeStack();

	int numtriangles = 0;
	int numvertices = 0;

	FILE *fp = fopen(filename, "rb");
	if(!fp)
	{
		Error("Couldn't open file %s\n", filename);
	}

	int numsurfaces = 1;
	for(i = 0; i < numsurfaces; i++)
	{
		trisurf_t *trisurf = AllocSurface();
		
		trisurf->numvertices = ReadInt(fp);
		trisurf->vertices = (modelvert_t*)Mem_Alloc(trisurf->numvertices * sizeof(modelvert_t));
		for(j = 0; j < trisurf->numvertices; j++)
		{
			trisurf->vertices[j].xyz[0] = ReadFloat(fp);
			trisurf->vertices[j].xyz[1] = ReadFloat(fp);
			trisurf->vertices[j].xyz[2] = ReadFloat(fp);

			float lightlevel = ReadFloat(fp) / 256.0f;
			trisurf->vertices[j].lightlevel[0] = lightlevel;
			trisurf->vertices[j].lightlevel[1] = lightlevel;
			trisurf->vertices[j].lightlevel[2] = lightlevel;
		}

		numvertices += trisurf->numvertices;

		trisurf->numindicies = ReadInt(fp);
		trisurf->indicies = (modelindex_t*)Mem_Alloc(trisurf->numindicies * sizeof(modelindex_t));
		for(j = 0; j < trisurf->numindicies; j++)
		{
			trisurf->indicies[j] = ReadInt(fp);
		}

		numtriangles += (trisurf->numindicies / 3);
	}

	fprintf(stdout, "surface count: %d\n", numsurfaces);
	fprintf(stdout, "vertex count: %d\n", numvertices);
	fprintf(stdout, "triangle count: %d\n", numtriangles);
}

//==============================================
// simulation code

static float viewangles[2];
static float viewpos[3];
static float viewvectors[3][3];

typedef struct tickcmd_s
{
	float	forwardmove;
	float	sidemove;
	float	anglemove[2];

} tickcmd_t;

static tickcmd_t gcmd;

static void VectorsFromSphericalAngles(float vectors[3][3], float angles[2])
{
	float cx, sx, cy, sy, cz, sz;

	cx = 1.0f;
	sx = 0.0f;
	cy = cosf(angles[0]);
	sy = sinf(angles[0]);
	cz = cosf(angles[1]);
	sz = sinf(angles[1]);

	vectors[0][0] = cy * cz;
	vectors[0][1] = sz;
	vectors[0][2] = -sy * cz;

	vectors[1][0] = (-cx * cy * sz) + (sx * sy);
	vectors[1][1] = cx * cz;
	vectors[1][2] = (cx * sy * sz) + (sx * cy);

	vectors[2][0] = (sx * cy * sz) + (cx * sy);
	vectors[2][1] = (-sx * cz);
	vectors[2][2] = (-sx * sy * sz) + (cx * cy);
}

// build a current command from the input state
static void BuildTickCmd()
{
	tickcmd_t *cmd = &gcmd;
	float scale;
	
	// Move forward ~512 units each second (60 * 4.2)
	scale = 4.2f;

	cmd->forwardmove = 0.0f;
	cmd->sidemove = 0.0f;
	cmd->anglemove[0] = 0.0f;
	cmd->anglemove[1] = 0.0f;

	if(input.keys['w'])
	{
		cmd->forwardmove += scale;
	}

	if(input.keys['s'])
	{
		cmd->forwardmove -= scale;
	}

	if(input.keys['d'])
	{
		cmd->sidemove += scale;
	}

	if(input.keys['a'])
	{
		cmd->sidemove -= scale;
	}

	// Handle mouse movement
	if(input.lbuttondown)
	{
		cmd->anglemove[0] = -0.01f * (float)input.moused[0];
		cmd->anglemove[1] = -0.01f * (float)input.moused[1];
	}
}


// apply the tick command to the viewstate
static void DoMove()
{
	tickcmd_t *cmd = &gcmd;

	VectorsFromSphericalAngles(viewvectors, viewangles);

	viewpos[0] += cmd->forwardmove * viewvectors[0][0];
	viewpos[1] += cmd->forwardmove * viewvectors[0][1];
	viewpos[2] += cmd->forwardmove * viewvectors[0][2];

	viewpos[0] += cmd->sidemove * viewvectors[2][0];
	viewpos[1] += cmd->sidemove * viewvectors[2][1];
	viewpos[2] += cmd->sidemove * viewvectors[2][2];

	viewangles[0] += cmd->anglemove[0];
	viewangles[1] += cmd->anglemove[1];

	if(viewangles[1] >= PI / 2.0f)
		viewangles[1] = (PI / 2.0f) - 0.001f;
	if(viewangles[1] <= -PI/ 2.0f)
		viewangles[1] = (-PI / 2.0f) + 0.001f;
}

static void SetupDefaultViewPos()
{
	// look down negative z
	viewangles[0] = PI / 2.0f;
	viewangles[1] = 0.0f;
	
	viewpos[0] = 0.0f;
	viewpos[1] = 0.0f;
	viewpos[2] = 256.0f;
}

// advance the state of everything by one frame
static void Ticker()
{
	BuildTickCmd();
	
	DoMove();
}

static void MainLoop()
{
	// initialize the base time
	if(!oldtime)
	{
		oldtime = Sys_Milliseconds();
	}

	int newtime = Sys_Milliseconds();
	int deltatime = newtime - oldtime;
	oldtime = newtime;

	// wait until some time has elapsed
	if(deltatime < 1)
	{
		Sys_Sleep(1);
		return;
	}
	
	// figure out how many tick s to run?
	// sync frames?
	if(deltatime > 50)
		deltatime = 0;

	// update realtime
	realtime += deltatime;

	// run a tick if enough time has elapsed
	// should realtime be clamped if we're dropping frames?
	//if(realtime > framenum * 16)
	while(realtime > framenum * 16)
	{
		framenum++;

		Ticker();
	}

	// signal a screen redraw (should this be sync'd with simulation updates
	// or free running?) glutPostRedisplay signals the draw callback to be called
	// on the next pass through the glutMainLoop
	// run as fast as possible to capture the mouse movements
	glutPostRedisplay();
}

//==============================================
// OpenGL rendering code
//
// this stuff touches some of the simulation state (viewvectors, viewpos etc)
// guess it should really have an interface to extract that data?

static int renderwidth;
static int renderheight;

static int rendermode = 0;
typedef void (*drawfunc_t)();

static void GL_LoadMatrix(float m[4][4])
{
	glLoadMatrixf((float*)m);
}

static void GL_LoadMatrixTranspose(float m[4][4])
{
	float t[4][4];

	MatrixTranspose(t, m);
	glLoadMatrixf((float*)t);
}

static void GL_MultMatrix(float m[4][4])
{
	glMultMatrixf((float*)m);
}

static void GL_MultMatrixTranspose(float m[4][4])
{
	float t[4][4];

	MatrixTranspose(t, m);
	glMultMatrixf((float*)t);
}

static void DrawAxis()
{
	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(32, 0, 0);

	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 32, 0);

	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 32);
	glEnd();
}

static void DrawTriSurfs(trisurf_t *trisurfs, int numtrisurfs)
{
	for(int i = 0; i < numtrisurfs; i++)
	{
		glVertexPointer(3, GL_FLOAT, sizeof(modelvert_t), trisurfs[i].vertices->xyz);
		glColorPointer(3, GL_FLOAT, sizeof(modelvert_t), trisurfs[i].vertices->lightlevel);

		//glDrawElements(GL_TRIANGLES, trisurfs[i].numindicies, GL_UNSIGNED_INT, trisurfs[i].indicies);
		glDrawArrays(GL_TRIANGLES, 0, trisurfs[i].numvertices);
	}
}

static void DrawSurfacesLit()
{
	//glFrontFace(GL_CW);
	//glEnable(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	DrawTriSurfs(trisurfs, numtrisurfs);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static void DrawSurfacesWireframe()
{
	static float white[] = { 1, 1, 1 };
	static float black[] = { 0, 0, 0 };

	//glFrontFace(GL_CW);
	//glEnable(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);

	glEnableClientState(GL_VERTEX_ARRAY);

	glColor3fv(white);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	DrawTriSurfs(trisurfs, numtrisurfs);

	glColor3fv(black);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -2);
	DrawTriSurfs(trisurfs, numtrisurfs);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisableClientState(GL_VERTEX_ARRAY);
}

void DrawSurfaces()
{
	static drawfunc_t drawfunclist[] =
	{
		DrawSurfacesLit, 
		DrawSurfacesWireframe,
	};

	//drawfunc_t DrawFunc = drawfunclist[rendermode];
	drawfunc_t DrawFunc = DrawSurfacesWireframe;
	
	// call the function to do the drawing
	DrawFunc();
}

static void SetModelViewMatrix()
{
	// matrix to transform from look down x to looking down -z
	static float yrotate[4][4] =
	{
		{ 0, 0, 1, 0 },
		{ 0, 1, 0, 0 },
		{ -1, 0, 0, 0 },
		{ 0, 0, 0, 1 }
	};

	// matrix to convert from doom coordinates to gl coordinates
	static float doomtogl[4][4] = 
	{
		{ 1, 0, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, -1, 0, 0 },
		{ 0, 0, 0, 1 }
	};

	float matrix[4][4];
	matrix[0][0]	= viewvectors[0][0];
	matrix[0][1]	= viewvectors[0][1];
	matrix[0][2]	= viewvectors[0][2];
	matrix[0][3]	= -(viewvectors[0][0] * viewpos[0]) - (viewvectors[0][1] * viewpos[1]) - (viewvectors[0][2] * viewpos[2]);

	matrix[1][0]	= viewvectors[1][0];
	matrix[1][1]	= viewvectors[1][1];
	matrix[1][2]	= viewvectors[1][2];
	matrix[1][3]	= -(viewvectors[1][0] * viewpos[0]) - (viewvectors[1][1] * viewpos[1]) - (viewvectors[1][2] * viewpos[2]);

	matrix[2][0]	= viewvectors[2][0];
	matrix[2][1]	= viewvectors[2][1];
	matrix[2][2]	= viewvectors[2][2];
	matrix[2][3]	= -(viewvectors[2][0] * viewpos[0]) - (viewvectors[2][1] * viewpos[1]) - (viewvectors[2][2] * viewpos[2]);

	matrix[3][0]	= 0.0f;
	matrix[3][1]	= 0.0f;
	matrix[3][2]	= 0.0f;
	matrix[3][3]	= 1.0f;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GL_MultMatrixTranspose(yrotate);
	GL_MultMatrixTranspose(matrix);
	GL_MultMatrixTranspose(doomtogl);
}

static void R_SetPerspectiveMatrix(float fov, float aspect, float znear, float zfar)
{
	float r, l, t, b;
	float fovx, fovy;
	float m[4][4];

	// fixme: move this somewhere else
	fovx = fov * (3.1415f / 360.0f);
	float x = (renderwidth / 2.0f) / atan(fovx);
	fovy = atan2(renderheight / 2.0f, x);

	// Calcuate right, left, top and bottom values
	r = znear * fovx; //tan(fovx * (3.1415f / 360.0f));
	l = -r;

	t = znear * fovy; //tan(fovy * (3.1415f / 360.0f));
	b = -t;

	m[0][0] = (2.0f * znear) / (r - l);
	m[1][0] = 0;
	m[2][0] = (r + l) / (r - l);
	m[3][0] = 0;

	m[0][1] = 0;
	m[1][1] = (2.0f * znear) / (t - b);
	m[2][1] = (t + b) / (t - b);
	m[3][1] = 0;

	m[0][2] = 0;
	m[1][2] = 0;
	m[2][2] = -(zfar + znear) / (zfar - znear);
	m[3][2] = -2.0f * zfar * znear / (zfar - znear);

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = -1;
	m[3][3] = 0;

	glMatrixMode(GL_PROJECTION);
	GL_LoadMatrix(m);
}

static void BeginFrame()
{
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
}

static void Draw()
{
	BeginFrame();

	SetModelViewMatrix();

	DrawAxis();

	DrawSurfaces();
}

//==============================================
// GLUT/OS/windowing code

// Called every frame to process the current mouse input state
// We only get updates when the mouse moves so the current mouse
// position is stored and may be used for multiple frames
static void ProcessInput()
{
	// mousepos has current "frame" mouse pos
	input.moused[0] = mousepos[0] - input.mousepos[0];
	input.moused[1] = mousepos[1] - input.mousepos[1];
	input.mousepos[0] = mousepos[0];
	input.mousepos[1] = mousepos[1];
}

static void DisplayFunc()
{
	Draw();

	glutSwapBuffers();
}

static void KeyboardDownFunc(unsigned char key, int x, int y)
{
	input.keys[key] = true;

	if(key == 'r')
	{
		rendermode++;
		if(rendermode == 2)
			rendermode = 0;
	}
}

static void KeyboardUpFunc(unsigned char key, int x, int y)
{
	input.keys[key] = false;
}

static void ReshapeFunc(int w, int h)
{
	renderwidth = w;
	renderheight = h;

	R_SetPerspectiveMatrix(90.0f, (float)w / (float)h, 3, 4096.0f);

	glViewport(0, 0, w, h);
}

static void MouseFunc(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON)
		input.lbuttondown = (state == GLUT_DOWN);
	if(button == GLUT_RIGHT_BUTTON)
		input.rbuttondown = (state == GLUT_DOWN);
}

static void MouseMoveFunc(int x, int y)
{
	mousepos[0] = x;
	mousepos[1] = y;
}

static void MainLoopFunc()
{
	ProcessInput();

	MainLoop();
}

static void ProcessCommandLine(int argc, char *argv[])
{
	int i;

	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-')
			break;
	}

	if(i == argc)
	{
		Error("No input file\n");
	}

	filename = argv[i];
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(400, 400);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("test");

	ProcessCommandLine(argc, argv);

	SetupDefaultViewPos();

	ReadSurfaces();

	glutReshapeFunc(ReshapeFunc);
	glutDisplayFunc(DisplayFunc);
	glutKeyboardFunc(KeyboardDownFunc);
	glutKeyboardUpFunc(KeyboardUpFunc);
	glutMouseFunc(MouseFunc);
	glutMotionFunc(MouseMoveFunc);
	glutPassiveMotionFunc(MouseMoveFunc);
	glutIdleFunc(MainLoopFunc);

	glutMainLoop();

	return 0;
}

