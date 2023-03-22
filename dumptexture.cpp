#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "doomlib.h"

// lump memory stack
#define MEM_STACK_SIZE (32 * 1024 * 1024)
static uint8_t mem_stack[MEM_STACK_SIZE];
static uint32_t mem_sp;

static wadfile_t *wadfile;

static uint8_t pal[256][3];
static uint32_t *pnames_table;

static uint16_t tex_w;
static uint16_t tex_h;
static uint16_t tex_patch_count;
static uint8_t *tex_surface;

static uint8_t *tex_lump_data[2];
static uint32_t tex_lump_size[2];

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

uint8_t *Mem_Alloc(uint32_t num_bytes)
{
	if (mem_sp + num_bytes > MEM_STACK_SIZE) {
		Error("out of buffer memory");
		return NULL;
	}

	uint8_t *mem = mem_stack + mem_sp;
	mem_sp += num_bytes;
	return mem;
}

uint8_t *ReadLump(uint32_t lumpnum)
{
	int size = Wad_LumpSize(wadfile, lumpnum);
	if(!size || size == -1) {
		Error("invalid lump size\n");
	}

	uint8_t *buffer = Mem_Alloc(size);
	Wad_ReadLumpIntoBuffer(wadfile, lumpnum, buffer);

	return buffer;
}

// FIXME: pass in patch data?
static void DrawPatch(uint8_t *surface, int32_t patch_origin_x, int32_t patch_origin_y, uint8_t *patch_data)
{
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

			for (int i = 0; i < length; i++) {
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
				uint8_t *pixel = surface + (pixel_y * tex_w * 3) + (pixel_x * 3);
				*pixel++ = pal[color][0];
				*pixel++ = pal[color][1];
				*pixel++ = pal[color][2];
			}

			y_data += 4 + length;
		}
	}
}

static void DrawTexture(uint8_t *tex_data)
{
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

		if (1) {
			uint8_t *patch_data = (uint8_t*)Wad_ReadLump(wadfile, pnames_table[patch_num]);
			printf("draw patch x=%i y=%i", patch_origin_x, patch_origin_y);
			DrawPatch(tex_surface, patch_origin_x, patch_origin_y, patch_data);
			Wad_FreeLump(patch_data);
		}
	}
}



static void LoadPalette()
{
	// load the palette data
	uint32_t pal_lumpnum = Wad_LumpNumFromName(wadfile, "PLAYPAL");
	uint8_t *pal_data = (uint8_t*)Wad_ReadLump(wadfile, pal_lumpnum);
	for (int i = 0; i < 256; i++) {
		pal[i][0] = *pal_data++;
		pal[i][1] = *pal_data++;
		pal[i][2] = *pal_data++;
	}
}

// FIXME: pnames lump is only needed temporarily
static void LoadPatchNames()
{
	// load the pnames lump
	uint32_t pnames_lumpnum = Wad_LumpNumFromName(wadfile, "PNAMES");
	uint8_t *pnames_data = ReadLump(pnames_lumpnum);

	int num_pnames = *(uint32_t*)pnames_data;
	pnames_data += 4;

	pnames_table = (uint32_t*)Mem_Alloc(sizeof(uint32_t) * num_pnames);

	for (int i = 0; i < num_pnames; i++, pnames_data += 8) {
		char *name = (char*)pnames_data;
		pnames_table[i] = Wad_LumpNumFromName(wadfile, name);
	}
}

static void LoadTextureLumps()
{
	// load the texture1 lump
	uint32_t lumpnum = Wad_LumpNumFromName(wadfile, "TEXTURE1");
	tex_lump_size[0] = Wad_LumpSize(wadfile, lumpnum);
	tex_lump_data[0] = Mem_Alloc(tex_lump_size[0]);
	Wad_ReadLumpIntoBuffer(wadfile, lumpnum, tex_lump_data[0]);

	// load the texture2 lump
	lumpnum = Wad_LumpNumFromName(wadfile, "TEXTURE2");
	if (lumpnum != -1) {
		tex_lump_size[1] = Wad_LumpSize(wadfile, lumpnum);
		tex_lump_data[1] = Mem_Alloc(tex_lump_size[1]);
		Wad_ReadLumpIntoBuffer(wadfile, lumpnum, tex_lump_data[1]);
	}
}

static uint8_t *FirstTexture()
{
	uint8_t *tex_data = tex_lump_data[0];
	uint32_t num_textures = *(uint32_t*)tex_data;
	tex_data += (4 + sizeof(uint32_t) * num_textures);
	
	return tex_data;
}

static uint8_t *NextTexture(uint8_t *tex_data)
{
	// jump to next texture
	uint16_t patch_count = *(uint16_t*)(tex_data + 20);
	tex_data += 22 + (patch_count * 10);

	if (tex_data == (tex_lump_data[0] + tex_lump_size[0])) {
		if (!tex_lump_data[1]) {
			return NULL;
		}
		
		tex_data = tex_lump_data[1];
		uint32_t num_textures = *(uint32_t*)tex_data;
		tex_data += (4 + sizeof(uint32_t) * num_textures);

		return tex_data;
	}
	else if (tex_data == (tex_lump_data[1] + tex_lump_size[1])) {
		return NULL;
	}

	return tex_data;
}

static void Command_ListTextures()
{
	int i = 0;
	uint8_t *tex_data = FirstTexture();
	while (tex_data) {
		printf("%-8i %-8s\n", i, tex_data);

		tex_data = NextTexture(tex_data);
		i++;
	}

}

static void Command_TextureInfo()
{}

int main(int argc, const char * argv[])
{
	//if(argc < 3) {
	//	printf("dumpwad <wadfile> <texture>\n");
	//	exit(0);
	//}

	// open the wad file
	wadfile = Wad_Open(argv[1]);
	if (!wadfile) {
		Error("failed to open wad file \'%s\'\n", argv[1]);
		return 1;
	}

	LoadPalette();

	LoadPatchNames();

	LoadTextureLumps();

	if (!strcmp(argv[2], "-list")) {
		Command_ListTextures();
		return 0;
	}

	// load texture1, texture2 data lumps?

	// ptr = FindTexture(name);


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
	
	DrawTexture(tex_data);

#if 0	
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
			uint8_t *patch_data = (uint8_t*)Wad_ReadLump(wadfile, pnames_table[patch_num]);
			DrawPatch(tex_surface, patch_origin_x, patch_origin_y, patch_data);
			Wad_FreeLump(patch_data);
		}
	}
#endif

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
