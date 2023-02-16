#include "catch.hpp"

#include "filepaths.hpp"
#include "handler.hpp"
#include "utils.hpp"
#include "xmlparser.hpp"

bool checkEqualVectors(const std::vector<uint16_t>& result, const std::vector<uint16_t>& expected, bool must_be_precise = true) {
    // Check size
    if (result.size() != expected.size()) {
        std::cerr << "Size of vectors is not the same.\n";
        return false;
    }

    auto success = true;

    for (size_t i = 0; i < result.size(); i++) {
        if (must_be_precise) {
            if (result[i] != expected[i]) {
                std::cout << result[i] << " != " << expected[i] << "(index = " << i << ")\n";
                success = false;
            }
        } else {
            if (result[i] != expected[i] && result[i] != expected[i] + 1 && result[i] != expected[i] - 1) {
                std::cout << result[i] << " != " << expected[i] << " (index = " << i << ")\n";
                success = false;
            }
        }
    }

    return success;
}

TEST_CASE("Handler + Image tests") {
    Sensor sensor;

    sensor.setSensorWidth(9);
    sensor.setSensorHeight(5);
    sensor.setOffsetX(2);
    sensor.setOffsetY(1);
    sensor.setPatternWidth(2);
    sensor.setPatternHeight(2);
    sensor.setActiveAreaWidth(6);
    sensor.setActiveAreaHeight(4);
    sensor.setSpatialWidth(sensor.activeAreaWidth() / sensor.patternWidth());
    sensor.setSpatialHeight(sensor.activeAreaHeight() / sensor.patternHeight());
    sensor.setNumberOfBands(4);
    sensor.mutableCoefficients() = {
        0.1f, 0.2f, 0.3f, 0.4f,
        0.5f, 0.6f, 0.7f, 0.8f,
        0.9f, 0.10f, 0.11f, 0.12f,
        0.13f, 0.14f, 0.15f, 0.16f
    };

    std::vector<uint16_t> data_no_offset{
        999, 999,  999,  999,  999,  999,  999,  999,  999,
        999, 999,    0,    1,    2,    3,    4,    5,  999,
        999, 999,  341,  342,  343,  344,  345,  346,  999,
        999, 999,  682,  683,  684,  685,  686,  687,  999,
        999, 999, 1018, 1019, 1020, 1021, 1022, 1023,  999
    };

    std::vector<uint16_t> data {
           0,    1,    2,    3,    4,   5,
         341,  342,  343,  344,  345, 346,
         682,  683,  684,  685,  686, 687,
        1018, 1019, 1020, 1021, 1022, 1023
    };

    std::vector<uint16_t> dark_ref_data {
        5, 10, 5, 10, 5, 10,
        5, 10, 5, 10, 5, 10,
        5, 10, 5, 10, 5, 10,
        5, 10, 5, 10, 5, 10
    };

    std::vector<uint16_t> white_ref_data {
        1023, 1023, 1023, 1023, 1023, 1023,
        1022, 1022, 1022, 1022, 1022, 1022,
        1021, 1021, 1021, 1021, 1021, 1021,
        1020, 1020, 1020, 1020, 1020, 1020,
    };

    std::vector<uint16_t> dark_ref_white_data{
        10, 15, 10, 15, 10, 15,
        10, 15, 10, 15, 10, 15,
        10, 15, 10, 15, 10, 15,
        10, 15, 10, 15, 10, 15
    };

    auto exposure_time_white_ref = 1000;
    auto exposure_time_object = 500;

    Image input(sensor, data);
    Image dark_ref_object(sensor, dark_ref_data);
    Image dark_ref_white(sensor, dark_ref_white_data);
    Image white_ref(sensor, white_ref_data);

    Handler handler(sensor, dark_ref_object, dark_ref_white, white_ref, exposure_time_object, exposure_time_white_ref);

    SECTION("Offset correction") {
        Image output(sensor);
        REQUIRE_NOTHROW(handler.offset(data_no_offset.data(), output));

        REQUIRE(checkEqualVectors(output.data(), data));
    }

    SECTION("Offset correction OpenCL") {
        Image output(sensor);
        REQUIRE_NOTHROW(handler.offsetOpenCL(data_no_offset.data(), output));

        REQUIRE(checkEqualVectors(output.data(), data));
    }

    SECTION("Convert to cube + reflection correction") {
        REQUIRE_NOTHROW(handler.convertToCubeAndReflectionCorrection(input));

        std::vector<uint16_t> expected{
            0, 0, 679, 674, 0, 0, 683, 678, 0, 0, 687, 682,
            1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
        };

        REQUIRE(checkEqualVectors(input.cube(), expected, false));
    }

    SECTION("Convert to cube + reflection correction OpenCL") {
        REQUIRE_NOTHROW(handler.convertToCubeAndReflectionCorrectionOpenCL(input));

        std::vector<uint16_t> expected{
            0, 0, 679, 674, 0, 0, 683, 678, 0, 0, 687, 682,
            1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
        };

        REQUIRE(checkEqualVectors(input.cube(), expected, false));
    }

    SECTION("Spectral correction") {
        Image output(sensor);
        REQUIRE_NOTHROW(handler.convertToCubeAndReflectionCorrection(input));
        REQUIRE_NOTHROW(handler.spectralCorrection(output, input));

        std::vector<uint16_t> expected{
            473, 1014, 155, 209, 476, 1020, 156, 210, 478, 1023, 157, 212,
            1023, 1023, 1023, 593, 1023, 1023, 1023, 593, 1023, 1023, 1023, 593
        };

        REQUIRE(checkEqualVectors(output.cube(), expected));
    }

    SECTION("Spectral correction OpenCL") {
        REQUIRE_NOTHROW(handler.convertToCubeAndReflectionCorrectionOpenCL(input));
        REQUIRE_NOTHROW(handler.spectralCorrectionOpenCL(input, sensor.spatialWidth() * sensor.spatialHeight(), sensor.numberOfBands()));

        std::vector<uint16_t> expected{
            473, 1014, 155, 209, 476, 1020, 156, 210, 478, 1023, 157, 212,
            1023, 1023, 1023, 593, 1023, 1023, 1023, 593, 1023, 1023, 1023, 593
        };

        REQUIRE(checkEqualVectors(input.cube(), expected));
    }

    SECTION("GetOneBand and colourmap") {
        REQUIRE_NOTHROW(handler.convertToCubeAndReflectionCorrection(input));

        SECTION("First band") {
            std::vector<uint16_t> result;
            REQUIRE_NOTHROW(handler.getOneBandAndColourmap(result, input, 0));
            
            std::vector<uint16_t> expected{
                257, 0, 1028, 257, 0, 1028, 257, 0, 1028,
                62965, 65534, 41890, 62965, 65534, 41890, 62965, 65534, 41890,
            };

            REQUIRE(checkEqualVectors(result, expected));
        }

        SECTION("Last band") {
            std::vector<uint16_t> result;
            REQUIRE_NOTHROW(handler.getOneBandAndColourmap(result, input, 3));
            std::vector<uint16_t> expected{
                58331, 26839, 9984, 58549, 27211, 9661, 58767, 27584, 9337,
                62965, 65534, 41890, 62965, 65534, 41890, 62965, 65534, 41890
            };

            REQUIRE(checkEqualVectors(result, expected));
        }
    }

    SECTION("GetOneBand and colourmap OpenCL") {
        REQUIRE_NOTHROW(handler.convertToCubeAndReflectionCorrectionOpenCL(input));

        SECTION("First band") {
            std::vector<uint16_t> result;
            REQUIRE_NOTHROW(handler.getOneBandAndColourmapOpenCL(result, input, 0));

            std::vector<uint16_t> expected{
                257, 0, 1028, 257, 0, 1028, 257, 0, 1028,
                62965, 65534, 41890, 62965, 65534, 41890, 62965, 65534, 41890,
            };

            REQUIRE(checkEqualVectors(result, expected));
        }

        SECTION("Last band") {
            std::vector<uint16_t> result;
            REQUIRE_NOTHROW(handler.getOneBandAndColourmapOpenCL(result, input, 3));
            std::vector<uint16_t> expected{
                58331, 26839, 9984, 58549, 27211, 9661, 58767, 27584, 9337,
                62965, 65534, 41890, 62965, 65534, 41890, 62965, 65534, 41890
            };

            REQUIRE(checkEqualVectors(result, expected));
        }
    }
}

TEST_CASE("Utils") {
    SECTION("ParseFloatArray") {
        auto array_text = "-0.016232591, 0.062453916, 6.7129e-005, -3.5533e-005, -0.000382194";
        std::vector<float> result;
        REQUIRE(Utils::parseFloatArray(result, array_text));

        REQUIRE(result.size() == 5);
        CHECK(result[0] == std::stof("-0.016232591"));
        CHECK(result[1] == std::stof("0.062453916"));
        CHECK(result[2] == std::stof("6.7129e-005"));
        CHECK(result[3] == std::stof("-3.5533e-005"));
        CHECK(result[4] == std::stof("-0.000382194"));
    }
}

TEST_CASE("XmlParser") {
    Sensor sensor;

    REQUIRE(XmlParser::ParseSensorCalibrationFileCamera(sensor, CALIBRATION_FILE));

    CHECK(sensor.layoutType() == LayoutType::MOSAIC);
    CHECK(sensor.bpp() == 10);
    CHECK(sensor.sensorWidth() == 2048);
    CHECK(sensor.sensorHeight() == 1088);
    CHECK(sensor.offsetX() == 0);
    CHECK(sensor.offsetY() == 3);
    CHECK(sensor.activeAreaWidth() == 2045);
    CHECK(sensor.activeAreaHeight() == 1080);
    CHECK(sensor.patternWidth() == 5);
    CHECK(sensor.patternHeight() == 5);
    CHECK(sensor.spatialWidth() == 409);
    CHECK(sensor.spatialHeight() == 216);
    CHECK(sensor.numberOfBands() == 25);
    
    const auto& coefficients = sensor.coefficients();
    REQUIRE(coefficients.size() == 625);
    CHECK(coefficients[0] == -0.0854386f);
    CHECK(coefficients[1] == -0.0225897f);
    CHECK(coefficients[2] == -0.00577639f);
    CHECK(coefficients[3] == -0.128251f);
    CHECK(coefficients[4] == -0.933499f);
    CHECK(coefficients[5] == -0.718447f);
    CHECK(coefficients[6] == -0.0804905f);
    CHECK(coefficients[7] == 0.00325381f);
    CHECK(coefficients[8] == -0.805999f);
    CHECK(coefficients[9] == 5.32515f);
    CHECK(coefficients[10] == -0.157854f);
    CHECK(coefficients[11] == -0.0495194f);
    CHECK(coefficients[12] == -0.0555022f);
    CHECK(coefficients[13] == -0.169307f);
    CHECK(coefficients[14] == -0.490573f);
    CHECK(coefficients[15] == -0.18886f);
    CHECK(coefficients[16] == 0.153923f);
    CHECK(coefficients[17] == 0.0115126f);
    CHECK(coefficients[18] == -0.111428f);
    CHECK(coefficients[19] == 0.109541f);
    CHECK(coefficients[20] == 0.00240896f);
    CHECK(coefficients[21] == -0.0748289f);
    CHECK(coefficients[22] == -0.0245523f);
    CHECK(coefficients[23] == -0.0170335f);
    CHECK(coefficients[24] == -0.440674f);
}
