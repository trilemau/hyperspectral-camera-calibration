#include "sensor.hpp"

Sensor::Sensor()
    : layout_type_(LayoutType::NO_LAYOUT_TYPE)
    , bpp_(0)
    , sensor_width_(0)
    , sensor_height_(0)
    , offset_x_(0)
    , offset_y_(0)
    , active_area_width_(0)
    , active_area_height_(0)
    , pattern_width_(0)
    , pattern_height_(0)
    , spatial_width_(0)
    , spatial_height_(0)
    , number_of_bands_(0) {}