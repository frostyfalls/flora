/* SPDX-License-Identifier: GPL-3.0-only */

#include <pixman.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

pixman_image_t *read_png(FILE *fp) {
	fseek(fp, 0, SEEK_SET);

	unsigned char header[8];
	if (fread(header, 1, sizeof(*header), fp) != sizeof(*header)) {
		return NULL;
	}
	if (!png_check_sig(header, sizeof(*header))) {
		return NULL;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
			NULL, NULL);
	if (png_ptr == NULL) {
		return NULL;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return NULL;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return NULL;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sizeof(*header));

	png_read_info(png_ptr, info_ptr);
	uint32_t width, height, color_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, NULL, &color_type, NULL,
			NULL, NULL);

	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY
			|| color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	}
	png_set_bgr(png_ptr);

	uint32_t passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	uint32_t stride = width * 4;

	pixman_image_t *image = pixman_image_create_bits(PIXMAN_x8r8g8b8, width,
			height, NULL, stride);
	if (image == NULL) {
		return NULL;
	}

	uint32_t *data = pixman_image_get_data(image);
	for (uint32_t pass = 0; pass < passes; ++pass) {
		for (uint32_t y = 0; y < height; ++y) {
			png_read_row(png_ptr, (png_bytep)data + y * stride, NULL);
		}
	}
	png_read_end(png_ptr, info_ptr);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	return image;
}
