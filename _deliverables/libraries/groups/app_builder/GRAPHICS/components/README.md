# GRAPHICS components

## Reusable color picker

`graphics::components::ColorPicker` is available through `components.hpp`.
It draws directly into a `graphics::Canvas` and does not depend on an OS
dialog or window toolkit.

```cpp
graphics::components::ColorPicker picker;
picker.set_color({255, 128, 0, 255});
picker.draw(canvas, 20, 20, 300, 200);
if (picker.pointer(pointer_x, pointer_y, 20, 20, 300, 200)) {
    const graphics::Color selected = picker.color();
    // Apply selected.r/g/b/a to the caller's draft state.
}
```

Use the same rectangle for drawing and pointer handling. Forward pointer
presses and moves while dragging; `pointer` reports whether a saturation/value,
hue, or opacity control consumed the position. The caller owns pointer capture,
release, cancellation and committing edits.

The picker displays a saturation/value square, hue strip, alpha slider,
checkerboard transparency and color swatches. Alpha is 0-255. RGB-only
`ColorPickerModel` construction remains supported, defaulting alpha to 255.
Rectangles smaller than 220 x 180 are inactive and are not drawn.

Build `components_tests` and run `GraphicsComponentsTests` with CTest to
exercise color conversion, interaction geometry, alpha and rendering.
