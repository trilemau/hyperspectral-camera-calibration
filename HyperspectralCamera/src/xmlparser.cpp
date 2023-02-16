#include "xmlparser.hpp"

#include <iostream>
#include <sstream>

#include "sensor.hpp"
#include "utils.hpp"

#define CORRECTION_MATRIX_NAME "hsi_675-975"

// ----- XmlParser -----

bool XmlParser::ParseSensorInfo(Sensor& sensor, pugi::xml_node node) {
    auto width = node.child("width");
    auto height = node.child("height");
    auto bpp = node.child("input_bpp");

    // check attributes are valid
    if (!width || !height || !bpp) {
        std::cerr << "XmlParser::ParseSensorInfo attributes not valid.\n";
        return false;
    }

    sensor.setSensorWidth(std::stoi(width.child_value()));
    sensor.setSensorHeight(std::stoi(height.child_value()));
    sensor.setBpp(std::stoi(bpp.child_value()));

    return true;
}

bool XmlParser::ParseFilterInfo(Sensor& sensor, pugi::xml_node node) {
    auto layout_type = node.attribute("layout");
    auto pattern_width = node.child("pattern_width");
    auto pattern_height = node.child("pattern_height");

    // check simple attributes are valid
    if (!layout_type || !pattern_width || !pattern_height) {
        std::cerr << "XmlParser::ParseFilterInfo simple attributes not valid.\n";
        return false;
    }

    sensor.setLayoutType(Utils::getLayoutType(layout_type.as_string()));
    sensor.setPatternWidth(std::stoi(pattern_width.child_value()));
    sensor.setPatternHeight(std::stoi(pattern_height.child_value()));

    // object attributes
    auto filter_area = node.child("filter_area");
    auto virtual_bands = node.child("spectral_correction_info").child("virtual_bands");

    // check object attributes are valid
    if (!filter_area) {
        std::cerr << "XmlParser::ParseFilterInfo filter area not valid.\n";
        return false;
    }

    auto result = ParseFilterArea(sensor, filter_area);

    // Check pattern width / height is not 0 for division
    if (sensor.patternWidth() == 0 || sensor.patternHeight() == 0) {
        std::cerr << "XmlParser::ParseFilterInfo pattern height or width is equal to 0.\n";
        return false;
    }

    sensor.setSpatialWidth(sensor.activeAreaWidth() / sensor.patternWidth());
    sensor.setSpatialHeight(sensor.activeAreaHeight() / sensor.patternHeight());
    sensor.setNumberOfBands(sensor.patternWidth() * sensor.patternHeight());

    return result;
}

bool XmlParser::ParseFilterArea(Sensor& sensor, pugi::xml_node node) {
    auto offset_x = node.child("offset_x");
    auto offset_y = node.child("offset_y");
    auto width = node.child("width");
    auto height = node.child("height");

    // check attributes are valid
    if (!offset_x || !offset_y || !width || !height) {
        std::cerr << "XmlParser::ParseFilterArea attributes not valid.\n";
        return false;
    }

    sensor.setOffsetX(std::stoi(offset_x.child_value()));
    sensor.setOffsetY(std::stoi(offset_y.child_value()));
    sensor.setActiveAreaWidth(std::stoi(width.child_value()));
    sensor.setActiveAreaHeight(std::stoi(height.child_value()));

    return true;
}

bool XmlParser::ParseVirtualBands(std::vector<float>& coefficients, pugi::xml_node node) {
    for (auto virtual_band : node.children("virtual_band")) {

        if (!virtual_band) {
            std::cerr << "XmlParser::ParseVirtualBands band not valid.\n";
            return false;
        }

        auto data = virtual_band.child("coefficients");

        if (!data) {
            std::cerr << "XmlParser::ParseVirtualBands coefficients not valid.\n";
            return false;
        }

        auto result = Utils::parseFloatArray(coefficients, data.child_value());

        // Check if virtual band parse was successful
        if (!result) {
            std::cerr << "XmlParser::ParseVirtualBands parsing virtual bands not successful.\n";
            return false;
        }
    }

    return true;
}

bool XmlParser::ParseCorrectionMatrices(Sensor& sensor, pugi::xml_node node) {
    for (auto correction_matrix : node.children("correction_matrix")) {
        if (!correction_matrix) {
            std::cerr << "XmlParser::ParseCorrectionMatrices correction_matrix not valid.\n";
            return false;
        }

        auto matrix_name_node = correction_matrix.child("name");

        // Check matrix_name is valid
        if (!matrix_name_node) {
            std::cerr << "XmlParser::ParseCorrectionMatrices matrix_name not found.\n";
            return false;
        }

        std::string matrix_name = matrix_name_node.child_value();

        // check if correct matrix name
        if (matrix_name != CORRECTION_MATRIX_NAME) {
            continue;
        }

        auto virtual_bands = correction_matrix.child("virtual_bands");

        if (!virtual_bands) {
            std::cerr << "XmlParser::ParseCorrectionMatrices virtual_bands not valid.\n";
            return false;
        }

        auto result = ParseVirtualBands(sensor.mutableCoefficients(), virtual_bands);

        if (!result) {
            std::cerr << "XmlParser::ParseCorrectionMatrices virtual bands parsing failed.\n";
            return false;
        }

        break;
    }

    // Check if coefficients are empty
    return sensor.coefficients().size() != 0;
}

bool XmlParser::ParseSensorCalibrationFileCamera(Sensor& sensor, const std::string& file) {
    pugi::xml_document document;
    auto parse_result = document.load_file(file.c_str());

    // Check if loading was successful
    if (!parse_result) {
        std::cerr << file;
        return false;
    }

    auto root = document.child("sensor_calibration");

    // Check root is valid
    if (!root) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera root (sensor_calibration) not found.\n";
        return false;
    }

    auto sensor_info = root.child("sensor_info");

    // Check sensor_info is valid
    if (!sensor_info) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera sensor_info not found.\n";
        return false;
    }

    auto result = ParseSensorInfo(sensor, sensor_info);

    auto filter_info = root.child("filter_info");

    // Check filter_info is valid
    if (!filter_info) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera filter_info not found.\n";
        return false;
    }

    auto filter_zone = filter_info.child("filter_zones").child("filter_zone");

    // Check filter_zone is valid
    if (!filter_zone) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera filter_zone not found.\n";
        return false;
    }

    result &= ParseFilterInfo(sensor, filter_zone);

    auto system_info = root.child("system_info");

    // Check system_info is valid
    if (!system_info) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera system_info not found.\n";
        return false;
    }

    auto spectral_correction_info = system_info.child("spectral_correction_info");

    // Check spectral_correction_info is valid
    if (!spectral_correction_info) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera spectral_correction_info not found.\n";
        return false;
    }

    auto correction_matrices = spectral_correction_info.child("correction_matrices");

    // Check correction_matrices is valid
    if (!correction_matrices) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera correction_matrices not found.\n";
        return false;
    }

    result &= ParseCorrectionMatrices(sensor, correction_matrices);

    // Check parsing was successful
    if (!result) {
        std::cerr << "XmlParser::ParseSensorCalibrationFileCamera parsing not successful.\n";
        return false;
    }

    return true;
}