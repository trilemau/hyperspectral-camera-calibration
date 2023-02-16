#include "utils.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "common.hpp"

// ----- Utils -----

LayoutType Utils::getLayoutType(const std::string& layout_type_text) {
    if (layout_type_text == "MOSAIC")
        return LayoutType::MOSAIC;

    if (layout_type_text == "TILED")
        return LayoutType::TILED;

    if (layout_type_text == "WEDGE")
        return LayoutType::WEDGE;

    return LayoutType::NO_LAYOUT_TYPE;
}

bool Utils::getBool(const std::string& bool_text) {
    if (bool_text == "true")
        return true;

    if (bool_text == "false")
        return false;

    throw std::runtime_error("GetBool(" + bool_text + ") not valid bool text");
}

bool Utils::isCharValid(char c) {
    return c == ' ' || c == ',' || c == '.' || c == '-' || c == 'e' || isdigit(c);
}

bool Utils::parseFloatArray(std::vector<float>& output, const std::string& array_text) {
    auto parsing_state = ArrayParseState::WAITING_FOR_COMMA;
    std::string float_text;

    for (char c : array_text) {
        // check if char is valid
        if (!isCharValid(c)) {
            std::cerr << "ParseFloatArray invalid char \'" << c << "\' in \"" << array_text << "\"";
            return false;
        }

        if (parsing_state == ArrayParseState::WAITING_FOR_COMMA && c == ',') {
            parsing_state = ArrayParseState::WAITING_FOR_SPACE;
        }
        else if (parsing_state == ArrayParseState::WAITING_FOR_SPACE && c == ' ') {
            output.emplace_back(std::stof(float_text));
            float_text.clear();

            parsing_state = ArrayParseState::WAITING_FOR_COMMA;
        }
        else {
            float_text += c;
        }
    }

    // check if parsing ended in invalid state
    if (parsing_state == ArrayParseState::WAITING_FOR_SPACE) {
        std::cerr << "ParseFloatArray ended in invalid state \"" << array_text << "\"";
        return false;
    }

    if (!float_text.empty())
        output.emplace_back(std::stof(float_text));

    return true;
}

bool Utils::doesFileExist(const std::string& filepath) {
    std::ifstream in(filepath);

    if (in.fail()) {
        return false;
    }

    return true;
}

std::string Utils::getTimeStamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%F_%H-%M-%S");

    return ss.str();
}
