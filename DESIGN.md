# ESP32 MiaOS Pixel Design System

## 1. Atmosphere & Identity

Compact retro utility UI for a 320x240 ESP32 framebuffer. The signature is sharp, tonal color bands over a black canvas: dense launcher rows, simple status text, direct controls, no ornamental motion, and no softened web-style surfaces.

## 2. Color

The native display uses indexed palette entries from `lavaDisplayInit()` in `src/lava_native_display.cpp`; do not alter these RGB values when fixing layout.

| Role | Token | Index | RGB | Usage |
|---|---:|---:|---|---|
| Background | `LAVA_BLACK` | 0 | `0, 0, 0` | Screen background, unselected rows |
| Text/primary | `LAVA_WHITE` | 1 | `255, 255, 255` | Primary text on black |
| Selection | `LAVA_BLUE` | 2 | `0, 96, 255` | Selected tabs and rows |
| Success/tag | `LAVA_GREEN` | 3 | `0, 220, 80` | SD-ready state, tags, progress |
| Error | `LAVA_RED` | 4 | `255, 48, 48` | Error and unavailable states |
| Header/accent | `LAVA_YELLOW` | 5 | `255, 220, 0` | Top bars, active labels, cursor |
| Info | `LAVA_CYAN` | 6 | `0, 220, 220` | Loading art and highlighted help text |
| Muted text/frame | `LAVA_GRAY` | 7 | `120, 120, 120` | Footer text, modal outer frame |
| Modal surface | `LAVA_DARK_BLUE` | 8 | `24, 24, 56` | Boot-loader help panel |
| Dialog fill | `LAVA_LIGHT_GRAY` | 15 | `210, 210, 210` | Dialog interiors |

## 3. Typography

Text is bitmap-rendered by `lava_text`; `lavaFontHeight()` is the active em-box height and the only vertical metric containers should use. The selectable faces are designed around compact 7-16 px em boxes, with the current `DejaVu 15` font data reporting 17 px; alignment code must use the live function result rather than clamp or infer from names.

| Face | Active em-box source | Height |
|---|---|---:|
| `5x7 ASCII` | `Small5x7` fixed path | 7 |
| `Basic 8` | `Basic8` fixed path | 8 |
| `Basic 12` | `Basic12` fixed path | 12 |
| `Basic 16` | `Basic16` fixed path | 16 |
| `DejaVu 12` | `fontDejaVu12.height` | 15 |
| `DejaVu 15` | `fontDejaVu15.height` | 17 |
| `Vera Bold 11` | `fontVeraBold11.height` | 13 |
| `Vera Bold 14` | `fontVeraBold14.height` | 16 |
| `Droid GBK 12` | `fontDroidGbk12.height` | 16 |

Glyph `yOffset` is internal to the em-box. It positions bitmap rows inside glyph rendering and must not be used to align text within launcher containers.

## 4. Spacing & Layout

The native screen is `LAVA_SCREEN_W=320` by `LAVA_SCREEN_H=240`. Coordinates are integer pixels; rectangles are sharp and unrounded.

Existing launcher bands include 20 px top headers and tabs, 18 px dialog headers, 16 px dialog menu rows, and dynamic app rows where `appRowHeight = max(16, lavaFontHeight() + 2)`. Common insets are 6, 8, 12, 18, 24, 28, and 40 px as already encoded in `drawLauncher()`.

For text inside any horizontal color band, vertically align the text em-box with:

```cpp
textY = bandY + max<int16_t>(0, (bandHeight - lavaFontHeight()) / 2);
```

The integer odd-pixel remainder stays below the em-box. Do not compensate with glyph `yOffset`; the container aligns the em-box, and glyphs align themselves inside it.

## 5. Components

### Launcher Header
- Full-width 20 px yellow band at `y=0`, black title text, optional right-side clock.

### Tabs
- Horizontal 20 px bands at `y=28`; selected tab is blue/yellow, unselected tab is black/gray, width is text width plus 12 px.

### App Rows
- Rows start at `y=58`, width 308 px from `x=6`, height `appRowHeight`; selected row is blue with black text and yellow cursor, unselected rows are black with white or green text.

### Dialogs
- Rectangular gray outer frame, light-gray or dark-blue body, 18-20 px title band, no rounded corners or shadows.

## 6. Motion & Interaction

The launcher has no animation system. Interaction is immediate redraw on button input, tab changes, selection movement, modal open/close, and periodic clock refresh. Preserve the static, deterministic framebuffer behavior.

## 7. Depth & Surface

Depth strategy is tonal-shift only: black base, colored bands, gray/light-gray dialog layers, and dark-blue help panels. Do not add shadows, gradients, rounded cards, antialiasing assumptions, CSS conventions, or new visual effects.
