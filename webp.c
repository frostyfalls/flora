/* SPDX-License-Identifier: GPL-3.0-only */

#include <pixman.h>
#include <stdio.h>
#include <sys/mman.h>
#include <webp/decode.h>

pixman_image_t *read_webp(FILE *fp) {
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	uint8_t *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileno(fp), 0);

	int32_t width, height;
	if (!WebPGetInfo(data, size, &width, &height)) {
		munmap(data, size);
		return NULL;
	}

	uint32_t stride = width * 4;
	pixman_image_t *image = pixman_image_create_bits(PIXMAN_x8r8g8b8,
			width, height, NULL, stride);
	if (image == NULL) {
		munmap(data, size);
		return NULL;
	}

	if (!WebPDecodeBGRAInto(data, size,
				(uint8_t *)pixman_image_get_data(image), height * stride,
				stride)) {
		pixman_image_unref(image);
		munmap(data, size);
		return NULL;
	}

	munmap(data, size);

	return image;
}
