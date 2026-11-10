/* SPDX-License-Identifier: GPL-3.0-only */

#include <math.h>
#include <pixman.h>
#include <stdbool.h>
#include <stdlib.h>

void display_mode_fill_fit(pixman_image_t *image, uint32_t width, uint32_t height,
		bool fill) {
	uint32_t src_width = pixman_image_get_width(image);
	uint32_t src_height = pixman_image_get_height(image);
	double sx = (double)width / src_width;
	double sy = (double)height / src_height;
	double s = fill ? fmax(sx, sy) : fmin(sx, sy);
	pixman_transform_t t;
	pixman_transform_init_scale(&t, pixman_double_to_fixed(1 / s),
			pixman_double_to_fixed(1 / s));
	pixman_transform_translate(&t, NULL, pixman_double_to_fixed((src_width -
					width / s) / 2), pixman_double_to_fixed((src_height -
						height / s) / 2));
	pixman_image_set_transform(image, &t);
}

void display_mode_stretch(pixman_image_t *image, uint32_t width, uint32_t height) {
	uint32_t src_width = pixman_image_get_width(image);
	uint32_t src_height = pixman_image_get_height(image);
	double sx = (double)width / src_width;
	double sy = (double)height / src_height;
	pixman_transform_t t;
	pixman_transform_init_scale(&t, pixman_double_to_fixed(1 / sx),
			pixman_double_to_fixed(1 / sy));
	pixman_image_set_transform(image, &t);
}

void display_mode_center(pixman_image_t *image, uint32_t width, uint32_t height) {
	uint32_t src_width = pixman_image_get_width(image);
	uint32_t src_height = pixman_image_get_height(image);
	pixman_transform_t t;
	pixman_transform_init_translate(&t, pixman_int_to_fixed((src_width - width)
				/ 2), pixman_int_to_fixed((src_height - height) / 2));
	pixman_image_set_transform(image, &t);
}
