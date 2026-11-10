/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef WAYWALLPAPER_H_
#define WAYWALLPAPER_H_

#include <pixman.h>
#include <stdint.h>
#include <stdio.h>

/* display-mode.c */
void display_mode_fill_fit(pixman_image_t *image, uint32_t width, uint32_t
		height, bool fill);
void display_mode_stretch(pixman_image_t *image, uint32_t width, uint32_t
		height);
void display_mode_center(pixman_image_t *image, uint32_t width, uint32_t
		height);

/* jpeg.c */
pixman_image_t *read_jpeg(FILE *fp);

/* png.c */
pixman_image_t *read_png(FILE *fp);

/* shm.c */
int allocate_shm_file(size_t size);

/* util.c */
char *xstrdup(const char *s);

/* webp.c */
pixman_image_t *read_webp(FILE *fp);

#endif /* WAYWALLPAPER_H_ */
