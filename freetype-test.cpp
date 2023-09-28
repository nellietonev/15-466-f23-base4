#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>

#include <iostream>

//This file exists to check that programs that use freetype / harfbuzz link properly in this base code.
//You probably shouldn't be looking here to learn to use either library.

// Code highly based on https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c

#define WIDTH 640
#define HEIGHT 480

/* origin is the upper left corner */
unsigned char image[HEIGHT * WIDTH * 4];

void draw_bitmap(FT_Bitmap *bitmap, FT_Int x, FT_Int y) {
	FT_Int i, j, p, q;
	FT_Int x_max = x + bitmap->width;
	FT_Int y_max = y + bitmap->rows;

	/* for simplicity, we assume that `bitmap->pixel_mode' */
	/* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

	for (i = x, p = 0; i < x_max; i++, p++) {
		for (j = y, q = 0; j < y_max; j++, q++) {
			if (i < 0 || j < 0 ||
				i >= WIDTH || j >= HEIGHT)
				continue;

			image[j][i] |= bitmap->buffer[q * bitmap->width + p];
		}
	}
}

int main(int argc, char **argv) {
	FT_Library library; /* handle to library */
	FT_Face face; /* handle to face object */

	FT_GlyphSlot slot;
	FT_Matrix matrix; /* transformation matrix */
	FT_Vector pen; /* untransformed origin */

	std::string text_str = "Hello, World!";
	char* text = const_cast<char *>(text_str.c_str());
	size_t num_chars = strlen(text);

	double angle =  (25.0/360) * float(M_PI) * 2; /* use 25 degrees */
	size_t target_height = 0;


	if (FT_Init_FreeType(&library) != 0) {
		std::cerr << "Failed to initialize FreeType" << std::endl;
		return 1;
	}

	if (FT_New_Face(library, "Knewave-Regular.ttf", 0, &face) != 0) {
		std::cerr << "Failed to load face" << std::endl;
		return 1;
	}

	if (FT_Set_Char_Size(face, 0, 50 * 64, 100, 0) != 0) {
		std::cerr << "Failed to set char size" << std::endl;
		return 1;
	}

	/* cmap selection omitted;                                        */
	/* for simplicity we assume that the font contains a Unicode cmap */

	/* Create hb-ft font. */
	hb_font_t *hb_font;
	hb_font = hb_ft_font_create (face, NULL);

	/* Create hb-buffer and populate. */
	hb_buffer_t *hb_buffer;
	hb_buffer = hb_buffer_create ();
	hb_buffer_add_utf8 (hb_buffer, text, -1, 0, -1);
	hb_buffer_guess_segment_properties (hb_buffer);

	/* Shape it! */
	hb_shape (hb_font, hb_buffer, NULL, 0);

	/* Get glyph information and positions out of the buffer. */
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

	slot = face->glyph;

	/* set up matrix */
	matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
	matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
	matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
	matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

	/* the pen position in 26.6 cartesian space coordinates; */
	/* start at (300, 200) relative to the upper left corner  */
	pen.x = 1 * 64;
	pen.y = (target_height) * 64;

	for (size_t n = 0; n < num_chars; n++) {
		face->glyph->glyph_index = info[n].codepoint;
		face->glyph->bitmap_left = pos[n].x_offset;
		face->glyph->bitmap_top = pos[n].y_offset;
		face->glyph->advance.x = pos[n].x_advance;
		face->glyph->advance.y = pos[n].y_advance;

		/* set transformation */
		FT_Set_Transform(face, &matrix, &pen);

		/* load glyph image into the slot (erase previous one) */
		if (FT_Load_Char(face, text[n], FT_LOAD_RENDER) != 0) {
			std::cerr << "Failed to load glyph image for index " << std::to_string(n) << std::endl;
			continue; /* ignore errors */
		}

		/* now, draw to our target surface (convert position) */
		draw_bitmap(&slot->bitmap, slot->bitmap_left, target_height - slot->bitmap_top);

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	std::cout << "It worked?" << std::endl;

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	return 0;
}
