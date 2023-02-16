#pragma once

#include "common.hpp"

#include <vector>

class Sensor {
    // Attributes
    LayoutType layout_type_;

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
    std::vector<float> coefficients_;

public:
    Sensor();

    // Getters
    LayoutType layoutType() const { return layout_type_; }
    unsigned int bpp() const { return bpp_; }
    unsigned int sensorWidth() const { return sensor_width_; }
    unsigned int sensorHeight() const { return sensor_height_; }
    unsigned int offsetX() const { return offset_x_; }
    unsigned int offsetY() const { return offset_y_; }
    unsigned int activeAreaWidth() const { return active_area_width_; }
    unsigned int activeAreaHeight() const { return active_area_height_; }
    unsigned int patternWidth() const { return pattern_width_; }
    unsigned int patternHeight() const { return pattern_height_; }
    unsigned int spatialWidth() const { return spatial_width_; }
    unsigned int spatialHeight() const { return spatial_height_; }
    unsigned int numberOfBands() const { return number_of_bands_; }
    const std::vector<float>& coefficients() const { return coefficients_; }

    // Setters
    void setLayoutType(LayoutType layout_type) { layout_type_ = layout_type; }
    void setBpp(unsigned int bpp) { bpp_ = bpp; }
    void setSensorWidth(unsigned int sensor_width) { sensor_width_ = sensor_width; }
    void setSensorHeight(unsigned int sensor_height) { sensor_height_ = sensor_height; }
    void setOffsetX(unsigned int offset_x) { offset_x_ = offset_x; }
    void setOffsetY(unsigned int offset_y) { offset_y_ = offset_y; }
    void setActiveAreaWidth(unsigned int width) { active_area_width_ = width; }
    void setActiveAreaHeight(unsigned int height) { active_area_height_ = height; }
    void setPatternWidth(unsigned int pattern_width) { pattern_width_ = pattern_width; }
    void setPatternHeight(unsigned int pattern_height) { pattern_height_ = pattern_height; }
    void setSpatialWidth(unsigned int spatial_width) { spatial_width_ = spatial_width; }
    void setSpatialHeight(unsigned int spatial_height) { spatial_height_ = spatial_height; }
    void setNumberOfBands(unsigned int number_of_bands) { number_of_bands_ = number_of_bands; }
    std::vector<float>& mutableCoefficients() { return coefficients_; }
};