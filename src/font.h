#ifndef FONT_H
#define FONT_H

#include <stdbool.h>
#include <stddef.h>

#include <uthash.h>
#include <stb_truetype.h>

struct chardata_t
{
    int id;
    UT_hash_handle hh;

    stbtt_packedchar pchar;
};

// uthash functions
void _addCharData(struct chardata_t** cmap, int codepoint, stbtt_packedchar pchar);
void _deleteCharData(struct chardata_t** cmap, int codepoint);
struct chardata_t* _getCharData(struct chardata_t** cmap, int codepoint);

struct font_t
{
    struct chardata_t* cmap;

    unsigned int texture;
    int textureWidth;
    int textureHeight;

    float pixelHeight;

    uint8_t* bitmap;
};

void font_init(struct font_t* f);
void font_terminate(struct font_t* f);

struct font_t* font_create();
void font_destroy(struct font_t* f);

// Export internal bitmap for a font. This should not be used if this font is
// already a bitmap font.
void exportBitmap(const char* imgOutFilename, struct font_t* font);
void exportFontData(const char* binOutFilename, struct font_t* font);

// Throwaway bitmap font data. If you loaded a TTF, we keep the generated bitmap
// just in case we want to export it, but it isn't necessary to keep around.
void flushFontBitmap(struct font_t* font);

// Loads a TTF file on the heap into *ttfData. Remember to free it!
void _loadTTF_file(const char* filename, uint8_t** ttfData);

// Load a bitmap into a font's texture handle.
void _bindFontTexture(struct font_t* font, uint8_t* bitmap);

struct font_t* loadTTF(struct font_t* font, const char* filename, float pixelHeight);
struct font_t* loadBitmapFontFiles(struct font_t* font, const char* imgFile, const char* binFile);
struct font_t* loadBitmapFontData(struct font_t* font,
                                  const uint8_t imgData[], size_t imgDataSize,
                                  const uint8_t binData[], size_t binDataSize);

void getPackedQuad(struct font_t* font, int codepoint,
                   float* xpos, float* ypos,
                   int align_to_integer,
                   stbtt_aligned_quad* q);

void drawString(struct font_t* font, float x, float y, const char* string);

float getStringWidth(struct font_t* font, const char* string);
float getMonospaceWidth(struct font_t* font);

#endif /* FONT_H */
