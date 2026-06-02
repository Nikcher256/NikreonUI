# NikreonUI

NikreonUI is a reusable retained UI library for native tools and game HUDs. It
contains backend-neutral widgets in `NikreonUI::Core` and an optional Vulkan
renderer in `NikreonUI::Vulkan`. It deliberately does not contain game-world
sprite, tilemap, particle, or camera rendering.

## Targets

| Target | Contents |
| --- | --- |
| `NikreonUI::Core` | Widgets, layout, input, styles, surfaces, clipping |
| `NikreonUI::Vulkan` | Vulkan quad, SDF shape, and FreeType text renderers |
| `NikreonUI::NikreonUI` | Compatibility alias for `NikreonUI::Vulkan` |

Configure with `-DNIKREON_UI_BUILD_VULKAN=OFF` to use another renderer backend.
Implement `Renderer2D` and `TextRenderer`, then pass them into a `UIFrame`.

## Surface And Frame

`UISurface` gives each UI tree its own local coordinate space. `UIFrame`
carries the surface, resolved style, shape renderer, text renderer, input
context, and synchronized clipping.

```cpp
UIContext input;
UIStyle style;
UISurface hud{{24.0f, 24.0f}, {320.0f, 180.0f}};
UIFrame frame{input, shapes, text, style, hud};

Label title{"hud.title", "Mission"};
title.setBounds({12.0f, 12.0f}, {180.0f, 24.0f});
title.setStyleClass("heading");
title.render(frame);
```

## Widgets

The core target provides:

- `Button`, `Checkbox`, `Slider`, `NumberInput`, and `TextInput`
- `Label`, `Panel`, textured `Image`/`Icon`, `NineSlicePanel`, and `ProgressBar`
- `ScrollContainer` with clipping, child ownership, content offsets, wheel
  scrolling, draggable thumbs, and theme-owned scrollbar styling
- Linear, dock, stack, anchor, padding, gap, and alignment layouts

Composite controls own their editing details. Render `Slider`, `NumberInput`,
and `TextInput` with `render(frame)` to draw shape, value text, selection, and
caret without reaching into nested controls.

```cpp
Slider volume{"hud.volume", 0.8f};
volume.setBounds({12.0f, 52.0f}, {180.0f, 20.0f});
volume.update(input, text, style);
volume.render(frame);
```

## Styling

`UIStyleParser` supports a deliberately small CSS-like format with
`type`, `type.class`, `type#id`, and `type.class#id` selectors. ID rules override
class rules. Shared theme data is application-neutral; editor panel sizing,
viewport colors, and toolbar configuration belong in the editor.

```css
slider.hud {
    fill: 0.12, 0.14, 0.18, 1;
    accent: 0.36, 0.58, 0.82, 1;
}

text.heading {
    color: 0.92, 0.98, 1, 1;
    font-scale: 1.1;
}
```

Text inputs support focus, UTF-8 insertion, caret movement, selection,
clipboard shortcuts, and horizontal overflow scrolling. Stylesheets can set
their text, placeholder, selection, caret, and scrollbar colors.

## Sample And Tests

`NikreonUISample` is a standalone Core-only sample. `NikreonUIWidgetTests`
contains focused widget and scrolling checks.

```sh
cmake -S . -B build -DNIKREON_UI_BUILD_VULKAN=OFF
cmake --build build
ctest --test-dir build
```
