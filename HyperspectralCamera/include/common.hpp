#pragma once

// ----- ----- ----- ----- ---
// ------ Macro values --- ---
// ----- ----- ----- ----- ---

#define SHORT_MAX 65535
#define PIXEL_MAX 1023
#define COLOURS_PER_PIXEL 3
#define BYTES_PER_PIXEL 2

#define PIXEL_BYTE_SIZE sizeof(uint16_t)
#define PIXEL_BIT_SIZE sizeof(uint16_t) * CHAR_BIT

// ----- ----- ----- ----- ---
// ----- ---- Enums ------ ---
// ----- ----- ----- ----- ---

// ----- ArrayParseState-----

enum class ArrayParseState {
    WAITING_FOR_COMMA,
    WAITING_FOR_SPACE
};


// ----- LayoutType -----

enum class LayoutType {
    NO_LAYOUT_TYPE = 0,
    MOSAIC = 1,
    TILED = 2,
    WEDGE = 3
};