#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static const char *palfilename = "PLAYPAL.bin";
static const char *sprfilename = NULL;
//static const char *outfilename = "out.rgba";
static const char *outfilename = NULL;

static FILE* palfp = NULL;
static FILE* sprfp = stdin;
static FILE* outfp = stdout;

static char palette[256][3];

static int w, h, ox, oy;

static unsigned char *surface = NULL;

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



static FILE *OpenFile(const char *filename)
{
	FILE *fp = fopen(filename, "rb");

	if(!fp)
		Error("Failed to open file \"%s\"", filename);

	return fp;
}



static int Read8(FILE *fp)
{
	if (feof(fp))
		return EOF;

	unsigned char b;
	fread(&b, sizeof(unsigned char), 1, fp);
	return b;
}



static int Read16(FILE *fp)
{
	if (feof(fp))
		return EOF;

	unsigned short i;
	fread(&i, sizeof(unsigned short), 1, fp);
	return i;
}



static int Read32(FILE *fp)
{
	if (feof(fp))
		return EOF;

	unsigned int i;
	fread(&i, sizeof(unsigned int), 1, fp);
	return i;
}



static void Write8(int b, FILE *fp)
{
	fwrite(&b, sizeof(unsigned char), 1, fp);
}



static void AllocateSurface()
{
	surface = (unsigned char*)malloc(w * h * 4 * sizeof(unsigned char));
	
	// clear all pixels to transparent black
	for (int i = 0; i < w * h; i++)
	{
		int addr = i * 4;
		surface[addr + 0] = 0;
		surface[addr + 1] = 0;
		surface[addr + 2] = 0;
		surface[addr + 3] = 0;
	}
}



static void FreeSurface()
{
	free(surface);
}



static void PutSurfacePixel(int x, int y, unsigned char rgba[4])
{
	int addr = (x * w * 4) + (y * 4);

	surface[addr + 0] = rgba[0];
	surface[addr + 1] = rgba[1];
	surface[addr + 2] = rgba[2];
	surface[addr + 3] = rgba[3];
}



static void EmitSurface()
{
	if (outfilename)
	{
		outfp = fopen(outfilename, "wb");
		if (!outfp)
			Error("failed to open output file \"%s\"\n", outfilename);
	}

	for (int i = 0; i < w * h; i++)
	{
		int addr = i * 4;

		Write8(surface[addr + 0], outfp);
		Write8(surface[addr + 1], outfp);
		Write8(surface[addr + 2], outfp);
		Write8(surface[addr + 3], outfp);
	}

	fclose(outfp);
}



static void ReadPalette()
{
	palfp = OpenFile(palfilename);
	fread(palette, 3 * sizeof(char), 256, palfp);
}



static void PaletteLookup(unsigned char color, unsigned char rgba[4])
{
	rgba[0] = palette[color][0];
	rgba[1] = palette[color][1];
	rgba[2] = palette[color][2];
	rgba[3] = 255;
}



static void ReadSpriteHeader()
{
	int b;
	w  = Read16(sprfp);
	h  = Read16(sprfp);
	ox = Read16(sprfp);
	oy = Read16(sprfp);
}



static void SkipOffsets()
{
	for (int i = 0; i < w; i++)
		Read32(sprfp);
}


static void DecodePosts()
{
	int b;
	int col = 0;
	
	while ((b = Read8(sprfp)) != EOF)
	{
		if (b == 0xff)
			col += 1;
		else
		{
			int s = b;
			int e = s + Read8(sprfp);

			// eat first byte
			Read8(sprfp);

			// decode run
			for (int row = s; row < e; row++)
			{
				unsigned char color;
				unsigned char rgba[4];

				// lookup the color in the palette, then write it to the surface
				color = Read8(sprfp);
				PaletteLookup(color, rgba);
				PutSurfacePixel(row, col, rgba);
			}

			// eat last byte
			Read8(sprfp);
		}
	}
}



static void ConvertSprite()
{
	ReadPalette();

	ReadSpriteHeader();

	SkipOffsets();

	AllocateSurface();

	DecodePosts();

	EmitSurface();

	FreeSurface();
}



int main(int argc, char *arg[])
{
	//sprfilename = argv[1];

	ConvertSprite();

	return 0;
}
