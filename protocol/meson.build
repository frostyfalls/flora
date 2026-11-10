# SPDX-License-Identifier: GPL-3.0-only

wayland_protocols = dependency('wayland-protocols').get_variable('pkgdatadir')
wayland_scanner = find_program(dependency('wayland-scanner').get_variable('wayland_scanner'))

proto_headers = []
proto_sources = []

foreach proto : [
  'wlr-layer-shell-unstable-v1.xml',
  wayland_protocols / 'stable/xdg-shell/xdg-shell.xml',
]
  proto_headers += custom_target(
    proto.underscorify() + '-client-header',
    output: '@BASENAME@.h',
    input: proto,
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
  proto_sources += custom_target(
    proto.underscorify() + '-private-code',
    output: '@BASENAME@.c',
    input: proto,
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )
endforeach
