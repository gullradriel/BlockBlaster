/* Copyright (C) 2026 Nilorea Studio

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <https://www.gnu.org/licenses/>. */

/**
 * \file blockblaster_shapes.h
 * \brief Static table of all block shapes available in the game.
 *
 * Shapes are ordered from easiest (fewest filled cells) to hardest (most
 * filled cells).  This ordering is relied upon by the difficulty-weighting
 * system in bag_refill(): the first shape in the array has the lowest
 * difficulty index (d = 0) and the last has the highest (d = 1).
 *
 * \author Castagnier Mickael aka Gull Ra Driel
 * \version 1.0
 * \date 21/02/2026
 */

#ifndef __BLOCKBLASTER_SHAPES__
#define __BLOCKBLASTER_SHAPES__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Read-only table of all block shapes offered to the player.
 *
 * Each entry is a Shape descriptor (w, h, cells[][], name).  The array is
 * declared static const so that every translation unit that includes this
 * header shares the same table without multiple-definition errors (the
 * compiler elides unused copies).
 *
 * Order matters: shapes must be sorted from fewest to most occupied cells so
 * that weighted_shape_index() can compute a normalised difficulty value d in
 * [0, 1] by position alone.
 */
static const Shape SHAPES[] = {

    /* [X]  single cell dot */
    {1, 1, {{true}}, "1"},
    /* [X]  single cell dot (duplicate for statistical balance) */
    {1, 1, {{true}}, "1"},
    /* [X]  single cell dot (duplicate for statistical balance) */
    {1, 1, {{true}}, "1"},
    /* [X]  single cell dot (duplicate for statistical balance) */
    {1, 1, {{true}}, "1"},

    /* [X][X]  2x1 horizontal bar */
    {2, 1, {{true, true}}, "I2"},
    /* [X][X]  2x1 horizontal bar (duplicate for statistical balance) */
    {2, 1, {{true, true}}, "I2"},
    /* [X][X]  2x1 horizontal bar (duplicate for statistical balance) */
    {2, 1, {{true, true}}, "I2"},
    /* [X][X]  2x1 horizontal bar (duplicate for statistical balance) */
    {2, 1, {{true, true}}, "I2"},

    /* [X]
       [X]
       1x2 vertical bar */
    {1, 2, {{true}, {true}}, "V2"},
    /* [X]
       [X]
       1x2 vertical bar (duplicate for statistical balance) */
    {1, 2, {{true}, {true}}, "V2"},
    /* [X]
       [X]
       1x2 vertical bar (duplicate for statistical balance) */
    {1, 2, {{true}, {true}}, "V2"},
    /* [X]
       [X]
       1x2 vertical bar (duplicate for statistical balance) */
    {1, 2, {{true}, {true}}, "V2"},

    /* [X][X][X]  3x1 horizontal bar */
    {3, 1, {{true, true, true}}, "I3"},
    /* [X][X][X]  3x1 horizontal bar (duplicate for statistical balance) */
    {3, 1, {{true, true, true}}, "I3"},
    /* [X][X][X]  3x1 horizontal bar (duplicate for statistical balance) */
    {3, 1, {{true, true, true}}, "I3"},
    /* [X][X][X]  3x1 horizontal bar (duplicate for statistical balance) */
    {3, 1, {{true, true, true}}, "I3"},

    /* [X]
       [X]
       [X]
       1x3 vertical bar */
    {1, 3, {{true}, {true}, {true}}, "V3"},
    /* [X]
       [X]
       [X]
       1x3 vertical bar (duplicate for statistical balance) */
    {1, 3, {{true}, {true}, {true}}, "V3"},
    /* [X]
       [X]
       [X]
       1x3 vertical bar (duplicate for statistical balance) */
    {1, 3, {{true}, {true}, {true}}, "V3"},
    /* [X]
       [X]
       [X]
       1x3 vertical bar (duplicate for statistical balance) */
    {1, 3, {{true}, {true}, {true}}, "V3"},

    /* [X][ ]
       [ ][X]
       2x2 diagonal (backslash) */
    {2, 2, {{true, false}, {false, true}}, "D\\2"},
    /* [X][ ]
       [ ][X]
       2x2 diagonal (backslash) (duplicate for statistical balance) */
    {2, 2, {{true, false}, {false, true}}, "D\\2"},

    /* [ ][X]
       [X][ ]
       2x2 diagonal (forward slash) */
    {2, 2, {{false, true}, {true, false}}, "D/2"},
    /* [ ][X]
       [X][ ]
       2x2 diagonal (forward slash) (duplicate for statistical balance) */
    {2, 2, {{false, true}, {true, false}}, "D/2"},

    /* [X][ ]
       [X][X]
       2x2 L-shape (top-right cell missing) */
    {2, 2, {{true, false}, {true, true}}, "L2"},

    /* [ ][X]
       [X][X]
       2x2 J-shape (top-left cell missing) */
    {2, 2, {{false, true}, {true, true}}, "J2"},

    /* [X][X]
       [X][X]
       2x2 full block */
    {2, 2, {{true, true}, {true, true}}, "O2"},

    /* [X][ ][ ]
       [X][X][X]
       3x2 L-shape (left column + full bottom row) */
    {3, 2, {{true, false, false}, {true, true, true}}, "L3a"},

    /* [X][X]
       [X][ ]
       [X][ ]
       2x3 L-shape (full top row + left column extending down) */
    {2, 3, {{true, true}, {true, false}, {true, false}}, "L3b"},

    /* [ ][ ][X]
       [X][X][X]
       3x2 J-shape (right column + full bottom row) */
    {3, 2, {{false, false, true}, {true, true, true}}, "J3a"},

    /* [X][X]
       [ ][X]
       [ ][X]
       2x3 J-shape (full top row + right column extending down) */
    {2, 3, {{true, true}, {false, true}, {false, true}}, "J3b"},

    /* [X][X][X]
       [ ][X][ ]
       3x2 T-shape (full top row + centre cell below) */
    {3, 2, {{true, true, true}, {false, true, false}}, "T"},

    /* [ ][X][ ]
       [X][X][X]
       3x2 T-shape flipped (centre cell on top + full bottom row) */
    {3, 2, {{false, true, false}, {true, true, true}}, "T_flip"},

    /* [ ][X]
       [X][X]
       [ ][X]
       2x3 T-shape rotated left (right column + centre cell to the left) */
    {2, 3, {{false, true}, {true, true}, {false, true}}, "T_left"},

    /* [X][ ]
       [X][X]
       [X][ ]
       2x3 T-shape rotated right (left column + centre cell to the right) */
    {2, 3, {{true, false}, {true, true}, {true, false}}, "T_right"},

    /* [X][X][ ]
       [ ][X][X]
       3x2 S-shape (horizontal) */
    {3, 2, {{true, true, false}, {false, true, true}}, "S"},

    /* [ ][X]
       [X][X]
       [X][ ]
       2x3 S-shape (vertical) */
    {2, 3, {{false, true}, {true, true}, {true, false}}, "SV"},

    /* [ ][X][X]
       [X][X][ ]
       3x2 Z-shape (horizontal mirror of S) */
    {3, 2, {{false, true, true}, {true, true, false}}, "Z"},

    /* [X][ ]
       [X][X]
       [ ][X]
       2x3 Z-shape (vertical mirror of SV) */
    {2, 3, {{true, false}, {true, true}, {false, true}}, "ZV"},

    /* [X][X][X]
       [X][ ][ ]
       3x2 C-shape open on the right */
    {3, 2, {{true, true, true}, {true, false, false}}, "C3a"},

    /* [X][X][X]
       [ ][ ][X]
       3x2 C-shape open on the left */
    {3, 2, {{true, true, true}, {false, false, true}}, "C3c"},

    /* [X][X][X][X]  4x1 horizontal bar */
    {4, 1, {{true, true, true, true}}, "I4"},

    /* [X]
       [X]
       [X]
       [X]
       1x4 vertical bar */
    {1, 4, {{true}, {true}, {true}, {true}}, "V4"},

    /* [X][ ][ ]
       [X][ ][ ]
       [X][X][X]
       3x3 L-shape (left column + bottom row) */
    {3,
     3,
     {{true, false, false}, {true, false, false}, {true, true, true}},
     "L4"},

    /* [ ][ ][X]
       [ ][ ][X]
       [X][X][X]
       3x3 J-shape (right column + bottom row, mirror of L4) */
    {3,
     3,
     {{false, false, true}, {false, false, true}, {true, true, true}},
     "J4"},

    /* [X][X][X]
       [ ][X][ ]
       [ ][X][ ]
       3x3 T-shape (full top row + centre column extending down) */
    {3,
     3,
     {{true, true, true}, {false, true, false}, {false, true, false}},
     "T4"},

    /* [ ][X][ ]
       [ ][X][ ]
       [X][X][X]
       3x3 T-shape reversed (full bottom row + centre column extending up) */
    {3,
     3,
     {{false, true, false}, {false, true, false}, {true, true, true}},
     "T4R"},

    /* [X][ ][X]
       [X][X][X]
       3x2 U-shape open on the bottom */
    {3, 2, {{true, false, true}, {true, true, true}}, "U3x2"},

    /* [X][X][X]
       [X][ ][X]
       3x2 U-shape open on the top */
    {3, 2, {{true, true, true}, {true, false, true}}, "U3x2_flip"},

    /* [X][X]
       [X][ ]
       [X][X]
       2x3 U-shape open on the right */
    {2, 3, {{true, true}, {true, false}, {true, true}}, "U2x3_right"},

    /* [X][X]
       [ ][X]
       [X][X]
       2x3 U-shape open on the left */
    {2, 3, {{true, true}, {false, true}, {true, true}}, "U2x3_left"},

    /* [X][X][X]
       [X][X][X]
       3x2 filled rectangle */
    {3, 2, {{true, true, true}, {true, true, true}}, "R3x2"},

    /* [X][X]
       [X][X]
       [X][X]
       2x3 filled rectangle */
    {2, 3, {{true, true}, {true, true}, {true, true}}, "R2x3"},

    /* [ ][X][ ]
       [X][X][X]
       [ ][X][ ]
       3x3 plus sign (centre row + centre column) */
    {3,
     3,
     {{false, true, false}, {true, true, true}, {false, true, false}},
     "Plus"},

    /* [X][X][X]
       [X][X][X]
       [X][X][X]
       3x3 full block (hardest fixed shape) */
    {3, 3, {{true, true, true}, {true, true, true}, {true, true, true}}, "O3"},

    /* ------------------------------------------------------------------ */
    /* Diagonal shapes                                                     */
    /* ------------------------------------------------------------------ */

    /* [X][ ][ ]
       [ ][X][ ]
       [ ][ ][X]
       3x3 diagonal (backslash) */
    {3,
     3,
     {{true, false, false}, {false, true, false}, {false, false, true}},
     "D\\3"},

    /* [ ][ ][X]
       [ ][X][ ]
       [X][ ][ ]
       3x3 diagonal (forward slash) */
    {3,
     3,
     {{false, false, true}, {false, true, false}, {true, false, false}},
     "D/3"},

    /* [X][X][X][X][X]  5x1 horizontal bar */
    {5, 1, {{true, true, true, true, true}}, "I5"},

    /* [X]
       [X]
       [X]
       [X]
       [X]
       1x5 vertical bar */
    {1, 5, {{true}, {true}, {true}, {true}, {true}}, "V5"},

    /* [X][ ][ ][ ]
       [ ][X][ ][ ]
       [ ][ ][X][ ]
       [ ][ ][ ][X]
       4x4 diagonal (backslash) */
    {4,
     4,
     {{true, false, false, false},
      {false, true, false, false},
      {false, false, true, false},
      {false, false, false, true}},
     "D\\4"},

    /* [ ][ ][ ][X]
       [ ][ ][X][ ]
       [ ][X][ ][ ]
       [X][ ][ ][ ]
       4x4 diagonal (forward slash) */
    {4,
     4,
     {{false, false, false, true},
      {false, false, true, false},
      {false, true, false, false},
      {true, false, false, false}},
     "D/4"},

};

/**
 * \brief Number of entries in the SHAPES table.
 *
 * Computed at compile time from the array size so that adding or removing
 * shapes automatically updates this count without manual maintenance.
 */
static const int SHAPES_COUNT = (int) (sizeof(SHAPES) / sizeof(SHAPES[0]));

#ifdef __cplusplus
}
#endif

#endif /* __BLOCKBLASTER_SHAPES__ */
