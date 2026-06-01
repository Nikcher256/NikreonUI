# NikreonUI

Reusable native GPU UI and text rendering library extracted from Nikreon Engine.

Current features:

- Retained buttons, checkboxes, and sliders
- Row, column, dock, stack, padding, gap, alignment, and anchor layouts
- CSS-like style parsing with widget and text classes
- CSS-like `type`, `type.class`, `type#id`, and `type.class#id` selectors
- Batched Vulkan quads and SDF rounded rectangles
- FreeType font atlas generation and batched Vulkan glyph rendering

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
