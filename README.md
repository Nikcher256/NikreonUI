# NikreonUI

Reusable native GPU UI and text rendering library extracted from Nikreon Engine.

Current features:

- Retained buttons, checkboxes, sliders, scrub-style number inputs, and text inputs
- Nested clip rectangles and wheel-driven scroll containers with draggable thumbs
- Row, column, dock, stack, padding, gap, alignment, and anchor layouts
- CSS-like style parsing with widget and text classes
- CSS-like `type`, `type.class`, `type#id`, and `type.class#id` selectors
- Batched Vulkan quads and SDF rounded rectangles
- FreeType growing font atlas generation and batched UTF-8 Vulkan glyph rendering

The library intentionally does not own an application window, swapchain, or editor panels. Consumers provide input snapshots and Vulkan frame resources.

ID selectors override reusable class selectors. For example:

```css
button.toolbar {
    border-radius: 10;
}

button.toolbar#toolbar.stop {
    selected-fill: 0.64, 0.28, 0.32, 1.0;
}
```

## Stylesheet Reference

Stylesheets use a deliberately small CSS-like syntax. Each declaration ends
with `;`. Colors are normalized RGBA values, vectors are comma-separated
numbers, and scalar sizes are pixel values.

Supported selector forms:

| Form | Example |
| --- | --- |
| Global UI settings | `ui` |
| Shared box style | `toolbar`, `panel`, `field` |
| Widget or text type | `button`, `checkbox`, `slider`, `number-input`, `text-input`, `text` |
| Type with reusable class | `button.toolbar`, `text.muted` |
| Type with unique ID | `button#toolbar.stop` |
| Type with class and ID | `button.toolbar#toolbar.stop` |

ID styles override class styles. A `type.class#id` rule starts with the
resolved class style and then applies its ID-specific declarations.

### Value Formats

| Format | Example |
| --- | --- |
| Scalar number | `border-width: 1;` |
| Two-component vector | `padding: 8, 5;` |
| RGBA color | `fill: 0.13, 0.145, 0.18, 1.0;` |
| Name | `font: default;` |

### `ui`

| Property | Value |
| --- | --- |
| `spacing` | Scalar gap size |
| `toolbar-height-min` | Scalar minimum toolbar height |
| `toolbar-height-max` | Scalar maximum toolbar height |

### `toolbar`, `panel`, `field`

| Property | Value |
| --- | --- |
| `fill` | RGBA color |
| `border-color` | RGBA color |
| `border-width` | Scalar width |
| `border-radius` | Scalar radius |
| `padding` | Horizontal, vertical vector |

### `button`

Buttons support all shared box properties plus:

| Property | Value |
| --- | --- |
| `hover-fill` | RGBA color |
| `pressed-fill` | RGBA color |
| `selected-fill` | RGBA color |
| `selected-border-color` | RGBA color |
| `icon-color` | RGBA color |
| `selected-icon-color` | RGBA color |
| `accent` | RGBA color |

### `checkbox`

Checkboxes support all shared box properties plus:

| Property | Value |
| --- | --- |
| `hover-fill` | RGBA color |
| `accent` | RGBA checked-mark color |

### `slider`

Sliders support all shared box properties plus:

| Property | Value |
| --- | --- |
| `hover-fill` | RGBA color |
| `accent` | RGBA filled-track color |
| `knob-color` | RGBA knob color |

### `number-input`

Number inputs support horizontal mouse-drag scrubbing and all shared box
properties plus:

| Property | Value |
| --- | --- |
| `hover-fill` | RGBA color |
| `accent` | RGBA left-edge accent color |

### `text-input`

Text inputs support focus, UTF-8 insertion, caret movement on UTF-8 boundaries,
shift-selection, delete, backspace, home, end, enter-to-commit, and
escape-to-blur. They support all shared box properties plus:

| Property | Value |
| --- | --- |
| `hover-fill` | RGBA color |
| `focused-fill` | RGBA color |
| `focused-border-color` | RGBA color |

### `text`

| Property | Value |
| --- | --- |
| `color` | RGBA color |
| `font` | Loaded font name |
| `font-scale` | Scalar multiplier |
| `opacity` | Scalar alpha multiplier |
| `offset` | X, Y vector |
| `text-align` | `left`, `center`, or `right` |
| `vertical-align` | `top`, `center`, or `bottom` |

Text alignment is resolved inside the consumer-provided text rectangle.
`offset` is applied after alignment.

`TextRenderer::drawText` and `measureText` also accept `TextLayout` options for
line spacing, maximum width, and basic wrapping.
