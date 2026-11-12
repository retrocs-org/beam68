#ifndef BEAM68_APPEARANCE_H
#define BEAM68_APPEARANCE_H
#include "beam.h"

/* Intuition DrawInfo pen indices, as exposed in the classic NDK.
 * Keep these portable so pen/geometry policy can be tested on a host. */
#define BEAM_PEN_TEXT 2
#define BEAM_PEN_SHADOW 4
#define BEAM_PEN_FILL 5
#define BEAM_PEN_FILLTEXT 6
#define BEAM_PEN_BACKGROUND 7
#define BEAM_PEN_HIGHLIGHTTEXT 8

#define BEAM_PREVIEW_BOX 52
#define BEAM_PREVIEW_MAX_IMAGE 48
#define BEAM_PREVIEW_MIN_WIDTH 340
#define BEAM_PREVIEW_MIN_HEIGHT 58

struct BeamPalette {
    unsigned short text;
    unsigned short shadow;
    unsigned short fill;
    unsigned short fill_text;
    unsigned short background;
    unsigned short highlight;
};

#define BEAM_THEME_ROLE_TEXT 0
#define BEAM_THEME_ROLE_SHADOW 1
#define BEAM_THEME_ROLE_FILL 2
#define BEAM_THEME_ROLE_FILL_TEXT 3
#define BEAM_THEME_ROLE_BACKGROUND 4
#define BEAM_THEME_ROLE_COUNT 5

const char *beam_theme_name(int theme);
const char *beam_theme_description(int theme);
int beam_theme_from_name(const char *name);
int beam_theme_color(int theme, int role, unsigned long *red,
                     unsigned long *green, unsigned long *blue);
int beam_theme_color_custom(int theme, int role,
                            const unsigned long *custom_colors,
                            unsigned long *red, unsigned long *green,
                            unsigned long *blue);
int beam_theme_parse_colors(const char *text, unsigned long *colors);
void beam_theme_format_colors(const unsigned long *colors, char *out,
                              unsigned int capacity);

/* DrawInfo pens may be noncontiguous. Never assume Workbench pen 0/1. */
void beam_palette_init(struct BeamPalette *out,
                       const unsigned short *pens, int count);

/* Available height excludes title, input and status regions.
 * Reserve 52 pixels to the right ONLY if an icon preview fits. */
int beam_preview_reserved(int enabled, int inner_width, int available_height);

/* Returns false for oversized icons: do not clip onto text/gadgets. */
int beam_preview_fits(int width, int height);

#endif
