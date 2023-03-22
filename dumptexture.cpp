#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "doomlib.h"

static wadfile_t *wadfile;

static uint8_t pal[256][3];
static uint32_t *pnames_table;

static uint16_t tex_w;
static uint16_t tex_h;
static uint16_t tex_patch_count;
static uint8_t *tex_surface;

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

static void DumpLump(wadfile_t *wadfile, int lumpnum)
{
	FILE *fp = stdout;

	int size = Wad_LumpSize(wadfile, lumpnum);
	if(!size || size == -1) {
		Error("invalid lump size\n");
	}

	// read the data block
	unsigned char *data = (unsigned char*)Wad_ReadLump(wadfile, lumpnum);
	if(!data) {
		Error("couldn't read lump data\n");
	}

	// dump the data out
	unsigned char *p = data;
	while(size--) {
		fputc(*p++, fp);
	}

	Wad_FreeLump(data);
}

// FIXME: pass in patch data?
static void DrawPatch(int32_t patch_origin_x, int32_t patch_origin_y, uint32_t patch_num)
{
	uint8_t *patch_data = (uint8_t*)Wad_ReadLump(wadfile, pnames_table[patch_num]);
	uint16_t w = *(uint16_t*)(patch_data + 0);
	uint16_t h = *(uint16_t*)(patch_data + 2);
	uint16_t offset_x = *(uint16_t*)(patch_data + 4);
	uint16_t offset_y = *(uint16_t*)(patch_data + 6);
	uint32_t *col_offsets = (uint32_t*)(patch_data + 8);

	for (int32_t x = 0; x < w; x++) {
		uint8_t *y_data = patch_data + col_offsets[x];

		while (*y_data != 0xff) {
			uint8_t y_start = *(y_data + 0);
			uint8_t length = *(y_data + 1);
			for (int i = 0; i < length; i++)
			{
				int32_t pixel_x = patch_origin_x + x;
				int32_t pixel_y = patch_origin_y + y_start + i;

				// clamp pixel locations
				if (pixel_x < 0 || pixel_x > tex_w - 1) {
					continue;
				}
				if (pixel_y < 0 || pixel_y > tex_h - 1) {
					continue;
				}

				// get the color and write it to the surface
				uint8_t color = *(y_data + 3 + i);
				uint8_t *pixel = tex_surface + (pixel_y * tex_w * 3) + (pixel_x * 3);
				*pixel++ = pal[color][0];
				*pixel++ = pal[color][1];
				*pixel++ = pal[color][2];
			}

			y_data += 4 + length;
		}
	}
}

int main(int argc, const char * argv[])
{
	//if(argc < 3) {
	//	printf("dumpwad <wadfile> <lumpname>\n");
	//	exit(0);
	//}

	// open the wad file
	wadfile = Wad_Open(argv[1]);
	if (!wadfile) {
		Error("failed to open wad file \'%s\'\n", argv[1]);
		return 1;
	}

	// load the palette data
	uint32_t pal_lump = Wad_LumpNumFromName(wadfile, "PLAYPAL");
	uint8_t *pal_data = (uint8_t*)Wad_ReadLump(wadfile, pal_lump);
	for (int i = 0; i < 256; i++) {
		pal[i][0] = *pal_data++;
		pal[i][1] = *pal_data++;
		pal[i][2] = *pal_data++;
	}

	// load the pnames lump
	uint32_t pnames_lump = Wad_LumpNumFromName(wadfile, "PNAMES");
	uint8_t *pnames_data = (uint8_t*)Wad_ReadLump(wadfile, pnames_lump);

	int num_pnames = *(uint32_t*)pnames_data;
	pnames_data += 4;

	pnames_table = (uint32_t*)malloc(sizeof(uint32_t) * num_pnames);

	for (int i = 0; i < num_pnames; i++, pnames_data += 8) {
		char *name = (char*)pnames_data;
		pnames_table[i] = Wad_LumpNumFromName(wadfile, name);
	}

#if 0
	// find the texture
	uint32_t tex_lump = Wad_LumpNumFromName(wadfile, "TEXTURE1");
	uint8_t *tex_lump_data = (uint8_t*)Wad_ReadLump(wadfile, tex_lump);

	uint8_t *tex_data = tex_lump_data;
	uint32_t num_textures = *(uint32_t*)tex_data;
	uint32_t *offset = (uint32_t*)(tex_data + 4);
	uint32_t *offset_end = offset + num_textures;

	for (int i = 0; i < num_textures; i++, offset++) {
		char *name = (char*)(tex_data + offset[i]);
		if (!strncmp(name, argv[2], 8)) {
			tex_data = tex_data + offset[i];
			break;
		}
	}

	if (offset == offset_end) {
		Error("couldn't find texture '%s'\n", argv[2]);
		return 1;
	}

	uint16_t tex_w = *(uint16_t*)(tex_data + 12);
	uint16_t tex_h = *(uint16_t*)(tex_data + 14);
	uint16_t tex_patch_count = *(uint16_t*)(tex_data + 20);
#endif

#if 1
	// find the texture
	uint32_t tex_lump = Wad_LumpNumFromName(wadfile, "TEXTURE1");
	uint8_t *tex_data = (uint8_t*)Wad_ReadLump(wadfile, tex_lump);
	uint8_t *tex_data_end = tex_data + Wad_LumpSize(wadfile, tex_lump);

	uint32_t num_textures = *(uint32_t*)tex_data;
	tex_data += (4 + sizeof(uint32_t) * num_textures);

	while (tex_data != tex_data_end) {
		char *name = (char*)tex_data;
		if (!strncmp(name, argv[2], 8)) {
			break;
		}

		// jump to next texture
		uint16_t patch_count = *(uint16_t*)(tex_data + 20);
		tex_data += 22 + (patch_count * 10);

		// swap to texture2 lump?
	}

	if (tex_data == tex_data_end) {
		Error("couldn't find texture '%s'\n", argv[2]);
		return 1;
	}
	
	// read the texture data
	tex_w = *(uint16_t*)(tex_data + 12);
	tex_h = *(uint16_t*)(tex_data + 14);
	tex_patch_count = *(uint16_t*)(tex_data + 20);
	tex_data += 22;

	// allocate the texture data
	tex_surface = (uint8_t*)malloc(3 * tex_w * tex_h);
	printf("tex_w=%i, tex_h=%i\n", tex_w, tex_h);

	// read and draw the patches
	for (int i = 0; i < tex_patch_count; i++) {
		int32_t patch_origin_x = *(uint16_t*)(tex_data + 0);
		int32_t patch_origin_y = *(uint16_t*)(tex_data + 2);
		uint32_t patch_num = *(uint16_t*)(tex_data + 4);
		uint32_t patch_step_dir = *(uint16_t*)(tex_data + 6);
		uint32_t patch_colormap = *(uint16_t*)(tex_data + 8);
		tex_data += 10;

		if (i == 0) {
			DrawPatch(patch_origin_x, patch_origin_y, patch_num);
		}
	}

	// write the data out
	FILE *fp_out = fopen("out.rgb", "wb");
	fwrite(tex_surface, sizeof(uint8_t), tex_w * tex_h * 3, fp_out);
	fclose(fp_out);
#endif

#if 0
	// process command line
	for (int i = 2; i < argc; i++) {
		int lumpnum = Wad_LumpNumFromName(wadfile, argv[i]);
		if (lumpnum == -1) {
			Error("couldn't find lump %s\n", argv[i]);
		}

		DumpLump(wadfile, lumpnum);
	}
#endif

	Wad_Close(wadfile);
	
	return 0;
}
