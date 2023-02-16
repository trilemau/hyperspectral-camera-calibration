#include "image.hpp"
#include "sensor.hpp"
#include "utils.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

Image::Image()
    : bpp_(0)
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

Image::Image(const Sensor& sensor)
    : bpp_(sensor.bpp())
    , sensor_width_(sensor.sensorWidth())
    , sensor_height_(sensor.sensorHeight())
    , offset_x_(sensor.offsetX())
    , offset_y_(sensor.offsetY())
    , active_area_width_(sensor.activeAreaWidth())
    , active_area_height_(sensor.activeAreaHeight())
    , pattern_width_(sensor.patternWidth())
    , pattern_height_(sensor.patternHeight())
    , spatial_width_(active_area_width_ / pattern_width_)
    , spatial_height_(active_area_height_ / pattern_height_)
    , number_of_bands_(pattern_width_ * pattern_height_)
    , data_raw_(static_cast<uint64_t>(active_area_width_) * active_area_height_)
    , data_cube_(data_raw_.size()) {
    // Check active area is divisible by pattern width / height
    if ((active_area_width_ % pattern_width_) != 0 || (active_area_height_ % pattern_height_) != 0) {
        throw std::runtime_error("Image invalid active area / pattern");
    }
}

Image::Image(const Sensor& sensor, const std::string& filePath)
    : Image(sensor) {
    // Open image file
    std::ifstream file(filePath, std::ios::binary);
    
    // Leave image empty if failed to open file
    if (file.fail()) {
        std::cerr << "Failed to open file \"" << filePath << "\".\n";
        return;
    }

    // Get size of file
    file.seekg(0, std::ios::end);
    auto actual_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Get expected size of file
    auto pixels = active_area_width_ * active_area_height_;
    auto expected_size = pixels * BYTES_PER_PIXEL;

    // Check if file is correct size
    if (actual_size != expected_size) {
        throw std::runtime_error("Image expected size " + std::to_string(expected_size) + " but was " + std::to_string(actual_size));
    }

    // Retrieve data from file
    file.read(reinterpret_cast<char*>(data_raw_.data()), actual_size);

    // Resize cube data for later
    data_cube_.resize(pixels);
}

Image::Image(const Sensor& sensor, const std::vector<uint16_t>& data)
    : Image(sensor) {
    auto expected_size = data_raw_.size();

    // Check if file is correct size
    if (data.size() != expected_size) {
        throw std::runtime_error("Image expected size " + std::to_string(expected_size) + " but was " + std::to_string(data.size()));
    }

    data_raw_ = data;

    // Resize cube data for later
    data_cube_.resize(data.size());
}

uint16_t Image::pixel(size_t x, size_t y) const {
    const auto i = getArrayIndex(x, y);

    if (i >= size()) {
        throw std::out_of_range("Image::pixel(" + std::to_string(x) + ", " + std::to_string(y) + ") out of range " + std::to_string(active_area_width_) + "x" + std::to_string(active_area_height_));
    }

    return data_raw_[i];
}

uint16_t& Image::mutablePixel(size_t x, size_t y) {
    const auto i = getArrayIndex(x, y);

    if (i >= size()) {
        throw std::out_of_range("Image::mutablePixel(" + std::to_string(x) + ", " + std::to_string(y) + ") out of range " + std::to_string(active_area_width_) + "x" + std::to_string(active_area_height_));
    }

    return data_raw_[i];
}

uint16_t Image::pixel(size_t i) const {
    if (i >= size()) {
        throw std::out_of_range("Image::pixel(" + std::to_string(i) + ") out of range " + std::to_string(active_area_width_) + "x" + std::to_string(active_area_height_));
    }
    
    return data_raw_[i];
}

uint16_t& Image::mutablePixel(size_t i) {
    if (i >= size()) {
        throw std::out_of_range("Image::mutablePixel(" + std::to_string(i) + ") out of range " + std::to_string(active_area_width_) + "x" + std::to_string(active_area_height_));
    }

    return data_raw_[i];
}

uint16_t Image::pixelCube(size_t i) const {
    if (i >= size()) {
        throw std::out_of_range("Image::pixelCube(" + std::to_string(i) + ") out of range " + std::to_string(active_area_width_) + "x" + std::to_string(active_area_height_));
    }

    return data_cube_[i];
}

uint16_t& Image::mutablePixelCube(size_t i) {
    if (i >= size()) {
        throw std::out_of_range("Image::mutablePixelCube(" + std::to_string(i) + ") out of range " + std::to_string(active_area_width_) + "x" + std::to_string(active_area_height_));
    }

    return data_cube_[i];
}

size_t Image::size() const {
    return data_raw_.size();
}

size_t Image::getArrayIndex(size_t x, size_t y) const {
    return x + (active_area_width_ * y);
}

const std::vector<uint16_t>& Image::data() const {
    return data_raw_;
}

std::vector<uint16_t>& Image::mutableData() {
    return data_raw_;
}

const std::vector<uint16_t>& Image::cube() const {
    return data_cube_;
}

std::vector<uint16_t>& Image::mutableCube() {
    return data_cube_;
}

void Image::saveWithoutChecking(const std::string& filename) const {
    std::cout << "Saving image \"" << filename << "\"...\n";

    std::ofstream out(filename, std::ios::out | std::ios::binary);
    out.write(reinterpret_cast<const char*>(data_raw_.data()), data_raw_.size() * 2);

    std::cout << "Saving successful.\n";
}

void Image::save(const std::string& filename) const {
    // Check if file already exists
    if (Utils::doesFileExist(filename)) {
        std::cout << "File already exists!\n";
        return;
    }

    saveWithoutChecking(filename);
}