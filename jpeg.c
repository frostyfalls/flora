/* SPDX-License-Identifier: GPL-3.0-only */

#include <stdbool.h>
#include <pixman.h>
#include <setjmp.h>
#include <stdio.h>
#include <jpeglib.h>

struct j_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

void j_error_exit(j_common_ptr cinfo) {
	struct j_error_mgr *err = (struct j_error_mgr *)cinfo->err;
	longjmp(err->setjmp_buffer, 1);
}

pixman_image_t *read_jpeg(FILE *fp) {
	fseek(fp, 0, SEEK_SET);

	struct jpeg_decompress_struct cinfo;
	struct j_error_mgr err;

	cinfo.err = jpeg_std_error(&err.pub);
	err.pub.error_exit = j_error_exit;
	if (setjmp(err.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, true);
	cinfo.out_color_space = JCS_EXT_BGRX;
	jpeg_start_decompress(&cinfo);

	if (cinfo.output_components != 4) {
		jpeg_destroy_decompress(&cinfo);
		return NULL;
	}

	uint32_t stride = cinfo.output_width * cinfo.output_components;

	pixman_image_t *image = pixman_image_create_bits(PIXMAN_x8r8g8b8,
			cinfo.output_width, cinfo.output_height, NULL, stride);
	if (image == NULL) {
		jpeg_destroy_decompress(&cinfo);
		return NULL;
	}

	uint8_t *data = (uint8_t *)pixman_image_get_data(image);
	while (cinfo.output_scanline < cinfo.output_height) {
		uint8_t *row = data + (size_t)cinfo.output_scanline * stride;
		jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&row, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return image;
}
