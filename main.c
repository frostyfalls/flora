/* SPDX-License-Identifier: GPL-3.0-only */

#include <ctype.h>
#include <errno.h>
#include <pixman.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wlr-layer-shell-unstable-v1.h>

#include "waywallpaper.h"

enum waywallpaper_display_mode {
	MODE_INVALID = 0,
	MODE_FILL,
	MODE_FIT,
	MODE_STRETCH,
	MODE_CENTER,
	MODE_TILE,
};

struct waywallpaper_state {
	struct wl_display *display;
	struct wl_event_loop *event_loop;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct zwlr_layer_shell_v1 *layer_shell;
	bool quit;
	struct wl_list outputs; /* struct waywallpaper_output::link */
	uint32_t color;
	const char *image_path;
	FILE *image_file;
	enum waywallpaper_display_mode display_mode;
	bool pixel_perfect;
};

struct waywallpaper_output {
	struct waywallpaper_state *state;
	struct wl_output *wl_output;
	uint32_t wl_name;
	char *name, *description;
	uint32_t width, height;
	struct wl_surface *surface;
	struct zwlr_layer_surface_v1 *layer_surface;
	struct wl_list link;
};

static void noop() {
}

static pixman_image_t *read_image(FILE *fp) {
	pixman_image_t *image;
#ifdef WAYWALLPAPER_HAVE_PNG
	image = read_png(fp);
	if (image != NULL) {
		return image;
	}
#endif /* WAYWALLPAPER_HAVE_PNG */
#ifdef WAYWALLPAPER_HAVE_JPEG
	image = read_jpeg(fp);
	if (image != NULL) {
		return image;
	}
#endif /* WAYWALLPAPER_HAVE_JPEG */
#ifdef WAYWALLPAPER_HAVE_WEBP
	image = read_webp(fp);
	if (image != NULL) {
		return image;
	}
#endif /* WAYWALLPAPER_HAVE_WEBP */
	return NULL;
}

static void unmap_pixman_image(pixman_image_t *image, void *data) {
	uint32_t height = pixman_image_get_height(image);
	uint32_t stride = pixman_image_get_stride(image);
	munmap(pixman_image_get_data(image), height * stride);
}

static void wl_buffer_handle_release(void *data, struct wl_buffer *wl_buffer) {
	wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener buffer_listener = {
	.release = wl_buffer_handle_release,
};

static pixman_image_t *create_surface_image(struct wl_shm *shm, struct
		wl_surface *surface, uint32_t width, uint32_t height) {
	uint32_t stride = width * 4;
	uint32_t size = height * stride;

	int fd = allocate_shm_file(size);
	if (fd == -1) {
		return NULL;
	}
	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
			0);
	if (data == MAP_FAILED) {
		close(fd);
		return NULL;
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width,
			height, stride, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);

	wl_buffer_add_listener(buffer, &buffer_listener, NULL);
	wl_surface_attach(surface, buffer, 0, 0);

	pixman_image_t *image = pixman_image_create_bits_no_clear(PIXMAN_x8r8g8b8,
			width, height, data, stride);
	if (image == NULL) {
		return NULL;
	}
	pixman_image_set_destroy_function(image, unmap_pixman_image, NULL);

	return image;
}

static void render_surface(struct waywallpaper_output *output) {
	struct waywallpaper_state *state = output->state;
	uint16_t red = ((state->color >> 16) & 0xff) * 256u;
	uint16_t green = ((state->color >> 8) & 0xff) * 256u;
	uint16_t blue = (state->color & 0xff) * 256u;
	pixman_image_t *image = create_surface_image(state->shm, output->surface,
			output->width, output->height);
	pixman_image_fill_rectangles(PIXMAN_OP_SRC, image, &(pixman_color_t){red,
			green, blue, 0}, 1, &(pixman_rectangle16_t){0, 0, output->width,
			output->height});
	pixman_image_t *wallpaper;
	if (state->image_file) {
		wallpaper = read_image(state->image_file);
	}
	if (wallpaper != NULL) {
		if (state->display_mode == MODE_FILL) {
			display_mode_fill_fit(wallpaper, output->width, output->height,
					true);
		} else if (state->display_mode == MODE_FIT) {
			display_mode_fill_fit(wallpaper, output->width, output->height,
					false);
		} else if (state->display_mode == MODE_STRETCH) {
			display_mode_stretch(wallpaper, output->width, output->height);
		} else if (state->display_mode == MODE_CENTER) {
			display_mode_center(wallpaper, output->width, output->height);
		} else if (state->display_mode == MODE_TILE) {
			pixman_image_set_repeat(image, PIXMAN_REPEAT_NORMAL);
		}
		if (!state->pixel_perfect) {
			pixman_image_set_filter(wallpaper, PIXMAN_FILTER_BEST, NULL, 0);
		}
		pixman_image_composite32(PIXMAN_OP_OVER, wallpaper, NULL, image, 0,
				0, 0, 0, 0, 0, output->width, output->height);
		pixman_image_unref(wallpaper);
	}
	wl_surface_commit(output->surface);
	pixman_image_unref(image);
}

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1
		*layer_surface, uint32_t serial, uint32_t width, uint32_t height) {
	struct waywallpaper_output *output = data;
	if (output->width == width && output->height == height) {
		return;
	}
	output->width = width;
	output->height = height;
	zwlr_layer_surface_v1_ack_configure(output->layer_surface, serial);
	render_surface(output);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = noop,
};

static void output_handle_done(void *data, struct wl_output *wl_output) {
	struct waywallpaper_output *output = data;
	struct waywallpaper_state *state = output->state;

	if (output->surface == NULL) {
		output->surface = wl_compositor_create_surface(state->compositor);

		struct wl_region *input_region = wl_compositor_create_region(state->compositor);
		wl_surface_set_input_region(output->surface, input_region);
		wl_region_destroy(input_region);

		output->layer_surface =
			zwlr_layer_shell_v1_get_layer_surface(state->layer_shell,
					output->surface, output->wl_output,
					ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "background");
		zwlr_layer_surface_v1_set_size(output->layer_surface, 0, 0);
		zwlr_layer_surface_v1_set_anchor(output->layer_surface,
				ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
				ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
				ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
				ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
		zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface, -1);
		zwlr_layer_surface_v1_add_listener(output->layer_surface,
				&layer_surface_listener, output);
		wl_surface_commit(output->surface);
	}
}

static void output_handle_name(void *data, struct wl_output *wl_output, const
		char *name) {
	struct waywallpaper_output *output = data;
	free(output->name);
	output->name = xstrdup(name);
}

static void output_handle_description(void *data, struct wl_output *wl_output,
		const char *description) {
	struct waywallpaper_output *output = data;
	free(output->description);
	output->name = xstrdup(description);
}

static const struct wl_output_listener output_listener = {
	.geometry = noop,
	.mode = noop,
	.done = output_handle_done,
	.scale = noop,
	.name = output_handle_name,
	.description = output_handle_description,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct waywallpaper_state *state = data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		state->compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 2);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		struct waywallpaper_output *output = calloc(1, sizeof(*output));
		output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 4);
		output->wl_name = name;
		output->state = state;
		wl_output_add_listener(output->wl_output, &output_listener, output);
		wl_list_insert(&state->outputs, &output->link);
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name) {
	struct waywallpaper_state *state = data;
	struct waywallpaper_output *output, *tmp;
	wl_list_for_each_safe(output, tmp, &state->outputs, link) {
		if (output->wl_name == name) {
			wl_list_remove(&output->link);
			wl_surface_destroy(output->surface);
			wl_output_release(output->wl_output);
			free(output->name);
			free(output->description);
			free(output);
		}
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

static int handle_signal(int sig, void *data) {
	struct waywallpaper_state *state = data;
	state->quit = true;
	return 0;
}

static bool parse_color(const char *color, uint32_t *result) {
	size_t len = strlen(color);
	if (len != 6) {
		return false;
	}
	for (size_t i = 0; i < len; ++i) {
		if (!isxdigit(color[i])) {
			return false;
		}
	}
	*result = strtol(color, NULL, 16);
	return true;
}

int main(int argc, char **argv) {
	struct waywallpaper_state state = {0};

	int opt;
	while ((opt = getopt(argc, argv, ":c:i:m:p")) != -1) {
		switch (opt) {
		case 'c':
			if (!parse_color(optarg, &state.color)) {
				fprintf(stderr, "Invalid color: %s\n", optarg);
				return 1;
			}
			break;
		case 'i':
			state.image_path = optarg;
			state.image_file = fopen(state.image_path, "rb");
			if (state.image_file == NULL) {
				fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
				return 1;
			}
			break;
		case 'm':
			if (strcmp(optarg, "fill") == 0) {
				state.display_mode = MODE_FILL;
			} else if (strcmp(optarg, "fit") == 0) {
				state.display_mode = MODE_FIT;
			} else if (strcmp(optarg, "stretch") == 0) {
				state.display_mode = MODE_STRETCH;
			} else if (strcmp(optarg, "center") == 0) {
				state.display_mode = MODE_CENTER;
			} else if (strcmp(optarg, "tile") == 0) {
				state.display_mode = MODE_TILE;
			} else {
				fprintf(stderr, "Invalid mode: %s\n", optarg);
				return 1;
			}
			break;
		case 'p':
			state.pixel_perfect = true;
			break;
		default:
			fprintf(stderr, "Usage: %s [-p] [-c COLOR] [-i IMAGE] [-m MODE]\n", argv[0]);
			return 1;
		}
	}
	if (state.image_file != NULL && state.display_mode == MODE_INVALID) {
		fprintf(stderr, "Must provide a display mode when setting an image\n");
		return 1;
	}

	wl_list_init(&state.outputs);

	state.display = wl_display_connect(NULL);
	if (state.display == NULL) {
		fprintf(stderr, "Failed to connect to Wayland display\n");
		return 1;
	}

	state.registry = wl_display_get_registry(state.display);
	wl_registry_add_listener(state.registry, &registry_listener, &state);
	wl_display_roundtrip(state.display);

	if (state.layer_shell == NULL) {
		fprintf(stderr, "Compositor doesn't support wlr-layer-shell\n");
		return 1;
	}

	/* TODO(frosty): Handle SIGINT and SIGQUIT to clean up resources */
	while (!state.quit && wl_display_dispatch(state.display) != -1) {
	}

	return 0;
}
