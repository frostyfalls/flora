# SPDX-License-Identifier: GPL-3.0-only

project(
  'flora',
  'c',
  version: '0.1.0',
  default_options: ['c_std=c99', 'warning_level=2'],
)

add_project_arguments(
  '-D_POSIX_C_SOURCE=200809L',
  '-Wno-unused-parameter',
  language: 'c',
)

wayland_client = dependency('wayland-client')
pixman = dependency('pixman-1')
libpng = dependency('libpng', required: get_option('png'))
libjpeg = dependency('libjpeg', required: get_option('jpeg'))
libwebp = dependency('libwebp', required: get_option('webp'))

format_sources = []
format_dependencies = []

if libpng.found()
  format_sources += 'png.c'
  format_dependencies += libpng
  add_project_arguments('-DFLORA_HAVE_PNG', language: 'c')
endif
if libjpeg.found()
  format_sources += 'jpeg.c'
  format_dependencies += libjpeg
  add_project_arguments('-DFLORA_HAVE_JPEG', language: 'c')
endif
if libwebp.found()
  format_sources += 'webp.c'
  format_dependencies += libwebp
  add_project_arguments('-DFLORA_HAVE_WEBP', language: 'c')
endif

subdir('protocol')

executable(
  'flora',
  ['main.c', 'display-mode.c', 'shm.c', 'util.c']
  + proto_headers + proto_sources + format_sources,
  dependencies: [wayland_client, pixman] + format_dependencies,
  include_directories: 'include',
)

summary(
  {
    'PNG support': libpng.found(),
    'JPEG support': libjpeg.found(),
    'WEBP support': libwebp.found(),
  },
  bool_yn: true,
)
