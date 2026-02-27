"""
BlockBlaster launcher icon generator.
Produces mipmap-* PNG icons for Android at all required densities.

Layout: 5×5 grid on a dark navy rounded-square background.
Each cell is a coloured block with a highlight (top-left) and shadow (bottom).
Pieces are arranged to suggest a BlockBlast mid-game board:

  .  .  C  C  C      ← cyan  I-piece (horizontal)
  R  .  .  C  .      ← red   L-piece (vertical, 4 tall)
  R  O  O  O  .      ← orange T-piece
  R  .  .  G  G      ← green 2×2 square
  R  P  P  G  G      ← purple I-piece (2 wide, bottom row)
"""

import os
from PIL import Image, ImageDraw

# ---------------------------------------------------------------------------
# Colour palette  (R, G, B)
# ---------------------------------------------------------------------------
BG     = (20, 24, 45)       # dark navy
RED    = (239, 68,  68)
CYAN   = (34, 211, 238)
ORANGE = (251, 146, 60)
GREEN  = (74, 222, 128)
PURPLE = (167, 139, 250)

COLOR_MAP = {'R': RED, 'C': CYAN, 'O': ORANGE, 'G': GREEN, 'P': PURPLE}

# 5×5 grid; None = empty cell
GRID = [
    [None,  None,  'C',   'C',   'C'  ],
    ['R',   None,  None,  'C',   None ],
    ['R',   'O',   'O',   'O',   None ],
    ['R',   None,  None,  'G',   'G'  ],
    ['R',   'P',   'P',   'G',   'G'  ],
]

# ---------------------------------------------------------------------------

def _clamp(v):
    return max(0, min(255, int(v)))

def _lighter(color, factor=1.55):
    return tuple(_clamp(c * factor) for c in color)

def _darker(color, factor=0.45):
    return tuple(_clamp(c * factor) for c in color)


def make_icon(size: int) -> Image.Image:
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Rounded-square background
    bg_radius = size // 5
    draw.rounded_rectangle(
        [0, 0, size - 1, size - 1],
        radius=bg_radius,
        fill=(*BG, 255),
    )

    # Grid geometry
    n = 5
    pad = max(4, int(size * 0.10))
    cell_w = (size - 2 * pad) / n
    gap    = max(1.0, cell_w * 0.07)
    br     = max(1.5, cell_w * 0.14)   # block corner radius

    for row_i, row in enumerate(GRID):
        for col_i, key in enumerate(row):
            if key is None:
                continue

            color = COLOR_MAP[key]

            # Block bounding box (inside the grid cell)
            x0 = pad + col_i * cell_w + gap
            y0 = pad + row_i * cell_w + gap
            x1 = pad + col_i * cell_w + cell_w - gap
            y1 = pad + row_i * cell_w + cell_w - gap

            # --- main face ---
            draw.rounded_rectangle([x0, y0, x1, y1], radius=br, fill=color)

            # --- top highlight strip ---
            h_strip = max(1.5, (y1 - y0) * 0.20)
            draw.rounded_rectangle(
                [x0, y0, x1, y0 + h_strip],
                radius=br,
                fill=_lighter(color),
            )

            # --- bottom shadow strip ---
            inner_x0 = x0 + br
            inner_x1 = x1 - br
            if inner_x1 > inner_x0:
                draw.rectangle(
                    [inner_x0, y1 - h_strip, inner_x1, y1],
                    fill=_darker(color),
                )

    return img


def make_round_icon(size: int) -> Image.Image:
    """Same design but composited onto a circular mask for ic_launcher_round."""
    square = make_icon(size)

    # Create a circle mask
    mask = Image.new('L', (size, size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.ellipse([0, 0, size - 1, size - 1], fill=255)

    result = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    result.paste(square, mask=mask)
    return result


# ---------------------------------------------------------------------------
# Density → pixel size mapping
# ---------------------------------------------------------------------------
DENSITIES = {
    'mipmap-mdpi':     48,
    'mipmap-hdpi':     72,
    'mipmap-xhdpi':    96,
    'mipmap-xxhdpi':  144,
    'mipmap-xxxhdpi': 192,
}

if __name__ == '__main__':
    base = os.path.join(os.path.dirname(__file__), 'android', 'res')
    for folder, sz in DENSITIES.items():
        out_dir = os.path.join(base, folder)
        os.makedirs(out_dir, exist_ok=True)
        make_icon(sz).save(os.path.join(out_dir, 'ic_launcher.png'))
        make_round_icon(sz).save(os.path.join(out_dir, 'ic_launcher_round.png'))
        print(f'  {folder:22s}  {sz}×{sz}px')
    print('Done.')
