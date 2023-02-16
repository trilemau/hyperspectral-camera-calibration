#pragma once

#include <string>
#include <vector>

class Sensor;

class Image {
    // Attributes
    unsigned int bpp_;
    unsigned int sensor_width_;
    unsigned int sensor_height_;
    unsigned int offset_x_;
    unsigned int offset_y_;

    unsigned int active_area_width_;
    unsigned int active_area_height_;
    unsigned int pattern_width_;
    unsigned int pattern_height_;
    unsigned int spatial_width_;
    unsigned int spatial_height_;
    unsigned int number_of_bands_;

    std::vector<uint16_t> data_raw_;
    std::vector<uint16_t> data_cube_;

public:
    Image();
    Image(const Sensor& sensor);
    Image(const Sensor& sensor, const std::string& filePath);
    Image(const Sensor& sensor, const std::vector<uint16_t>& data);

    size_t getArrayIndex(size_t x, size_t y) const;

    uint16_t pixel(size_t x, size_t y) const;
    uint16_t& mutablePixel(size_t x, size_t y);

    uint16_t pixel(size_t i) const;
    uint16_t& mutablePixel(size_t i);

    uint16_t pixelCube(size_t i) const;
    uint16_t& mutablePixelCube(size_t i);

    size_t size() const;

    const std::vector<uint16_t>& data() const;
    std::vector<uint16_t>& mutableData();

    const std::vector<uint16_t>& cube() const;
    std::vector<uint16_t>& mutableCube();

    void saveWithoutChecking(const std::string& filename) const;
    void save(const std::string& filename) const;
};
