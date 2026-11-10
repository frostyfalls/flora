# flora

A small wallpaper for Wayland compositors supporting wlr-layer-shell. It
supports solid colors in addition to PNG, JPEG, and WEBP images.

## Dependencies

* wayland
* pixman
* libpng (optional, for PNG support)
* libjpeg-turbo (optional, for JPEG support)
* libwebp (optional, for WEBP support)

If no image formats are supported, only solid colors are available. When using
an image wallpaper, a solid color can still be set to fill the empty space in
the case of fit and center display modes.

## Building

Using a [Meson](https://mesonbuild.com/)-compatible build system, set up the
build directory and configure the list of supported formats.

```
meson setup [-Dpng=disabled] [-Djpeg=disabled] [-Dwebp=disabled] build
ninja -C build
```

## License

flora is licensed under the GPLv3. See COPYING for more information.
