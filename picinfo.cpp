#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static const char *sprfilename = NULL;
static FILE* sprfp = stdin;

static int w, h, ox, oy;

static const char *format = "width: %w\nheight: %h\noffsetx: %ox\noffsety: %oy\n";

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



static int Read16(FILE *fp)
{
	if (feof(fp))
		return EOF;

	unsigned short i;
	fread(&i, sizeof(unsigned short), 1, fp);
	return i;
}



static void ReadSpriteHeader()
{
	int b;
	w  = Read16(sprfp);
	h  = Read16(sprfp);
	ox = Read16(sprfp);
	oy = Read16(sprfp);
}



static void ReadSprite()
{
	ReadSpriteHeader();
}



static void EmitInfo()
{
	while (*format)
	{
		if (format[0] == '%' && format[1] == 'w')
		{
			printf("%i", w);
			format += 2;
		}
		else if (format[0] == '%' && format[1] == 'h')
		{
			printf("%i", h);
			format += 2;
		}
		else if (format[0] == '%' && format[1] == 'o' && format[2] == 'x')
		{
			printf("%i", ox);
			format += 3;
		}
		else if (format[0] == '%' && format[1] == 'o' && format[2] == 'y')
		{
			printf("%i", oy);
			format += 3;
		}
		else
		{
			printf("%c", *format++);
		}
	}
}


int main(int argc, char *argv[])
{
	//sprfilename = argv[1];
	int arg = 1;
	for(; arg < argc && argv[arg][0] == '-'; arg++)
	{
		if (!strcmp(argv[arg], "-f") || !strcmp(argv[arg], "--format"))
			format = argv[arg + 1];
	}

	ReadSprite();

	EmitInfo();

	return 0;
}
