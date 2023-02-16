#pragma once

#include "resources/pugixml-1.10/src/pugixml.hpp"
#include <vector>

// ----- Class forwards -----

class Sensor;
class RawInfo;

// ----- XmlParser -----

class XmlParser {
    static bool ParseSensorInfo(Sensor& sensor, pugi::xml_node node);
    static bool ParseFilterInfo(Sensor& sensor, pugi::xml_node node);
    static bool ParseFilterArea(Sensor& sensor, pugi::xml_node node);
    static bool ParseCorrectionMatrices(Sensor& sensor, pugi::xml_node node);
    static bool ParseVirtualBands(std::vector<float>& coefficients, pugi::xml_node);

public:
    static bool ParseSensorCalibrationFileCamera(Sensor& sensor, const std::string& file);
};
