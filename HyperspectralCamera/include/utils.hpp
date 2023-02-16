#pragma once

#include <iostream>
#include <vector>

// ----- Class forwards -----

enum class LayoutType;


// ----- Utils -----

class Utils {
public:
    static LayoutType getLayoutType(const std::string& layout_type_text);
    static bool getBool(const std::string& bool_text);
    static bool isCharValid(char c);
    static bool parseFloatArray(std::vector<float>& output, const std::string& array_text);
    static bool doesFileExist(const std::string& filepath);
    static std::string getTimeStamp();
};
