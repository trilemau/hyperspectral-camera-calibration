#include <GLFW/glfw3.h>
#include "xiApi.h"

#include "filepaths.hpp"
#include "image.hpp"
#include "handler.hpp"
#include "sensor.hpp"
#include "utils.hpp"
#include "xmlparser.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#define KEY_PRESS_MASK 0x8000

#define EXPOSURE_TIME 12500
#define EXPOSURE_TIME_WHITE_REFERENCE 12500
#define SPECTRAL_WORKGROUP1 108
#define SPECTRAL_WORKGROUP2 5

// Number for iterations used for benchmark
// Set to 0 for no benchmark
#define COMPARISON_ITERATIONS 0

void printInfo(int exposure_time, int band_index, double get_image_time, double offset_correction_time, double converttocube_reflection_correction_time, double spectral_correction_time, double getoneband_colourmap_time, double render_time) {
    auto total_time = get_image_time + offset_correction_time + converttocube_reflection_correction_time + spectral_correction_time + getoneband_colourmap_time + render_time;
    
    std::cout << "----- INFO -----\n";
    std::cout << "Exposure time: " << exposure_time / 1000000 << "s (" << 1000000.0 / exposure_time << "fps)\n";
    std::cout << "Band number: " << band_index + 1 << "\n";
    std::cout << "Get image time: " << get_image_time << "s (" << 1.0 / get_image_time << "fps)\n";
    std::cout << "Offset correction: " << offset_correction_time << "s (" << 1.0 / offset_correction_time << "fps)\n";
    std::cout << "Convert to cube and reflection correction: " << converttocube_reflection_correction_time << "s (" << 1.0 / converttocube_reflection_correction_time << "fps)\n";
    std::cout << "Spectral correction: " << spectral_correction_time << "s (" << 1.0 / spectral_correction_time << "fps)\n";
    std::cout << "GetOneBand and colourmap: " << getoneband_colourmap_time << "s (" << 1.0 / getoneband_colourmap_time << "fps)\n";
    std::cout << "Render time: " << render_time << "s (" << 1.0 / render_time << "fps)\n";
    std::cout << "Total time: " << total_time << "s (" << 1.0 / total_time << "fps)\n";
    std::cout << '\n';
}

bool areYouSure() {
    std::cout << "This will replace existing file, are you sure? Type y / n:\n";

    std::string input;

    while (1) {
        std::cin >> input;

        if (input == "y") {
            return true;
        }
        else if (input == "n") {
            return false;
        }
        else {
            std::cout << "Invalid input! Type y / n:\n";
        }
    }

    return true;
}

void takeWhiteReference(Handler& handler, const Image& image) {
    if (!areYouSure()) {
        return;
    }
    
    image.saveWithoutChecking(WHITE_REFERENCE_FILE);
    handler.setWhiteReference(image);
    std::cout << "New white reference set.\n";
}

void takeDarkReference(Handler& handler, const Image& image) {
    if (!areYouSure()) {
        return;
    }
    
    image.saveWithoutChecking(DARK_REFERENCE_FILE);
    handler.setDarkReferenceObject(image);
    std::cout << "New dark reference for object set.\n";
}

void takeDarkReferenceWhite(Handler& handler, const Image& image) {
    if (!areYouSure()) {
        return;
    }
    
    image.saveWithoutChecking(DARK_REFERENCE_WHITE_FILE);
    handler.setDarkReferenceWhite(image);
    std::cout << "New dark reference for white reference set.\n";
}

void takeImage(const Image& image) {
    image.save(SNAPSHOT_FOLDER + Utils::getTimeStamp() + ".hdr");
}

void incrementBand(Sensor& sensor, int& band_index) {
    band_index = (band_index + 1) % sensor.numberOfBands();
    std::cout << "Set band index to = " << band_index + 1 << '\n';
}

void decrementBand(Sensor& sensor, int& band_index) {
    if (band_index == 0) {
        band_index = sensor.numberOfBands() - 1;
    }
    else {
        band_index = (band_index - 1) % sensor.numberOfBands();
    }

    std::cout << "Set band index to = " << band_index + 1 << '\n';
}

int main() {
    Sensor sensor;
    
    if (!XmlParser::ParseSensorCalibrationFileCamera(sensor, CALIBRATION_FILE)) {
        std::cerr << "CALIBRATION FILE PARSE FAILED";
        return EXIT_FAILURE;
    }

    // Check mosaic layout type
    if (sensor.layoutType() != LayoutType::MOSAIC) {
        std::cerr << "Layout type must be MOSAIC";
        return EXIT_FAILURE;
    }

    // Check bits per pixel is 10
    if (sensor.bpp() != 10) {
        std::cerr << "Bits per pixel is not 10 (actual value = " << sensor.bpp() << ")";
        return EXIT_FAILURE;
    }

    auto program_return = EXIT_SUCCESS;
    auto inAcquisition = false;

    auto multiply = 2;
    auto screen_width = sensor.spatialWidth();
    auto screen_height = sensor.spatialHeight();
    auto band_index = 9;

    double get_image_time = 0;
    double offset_correction_time = 0;
    double converttocube_reflection_correction_time = 0;
    double spectral_correction_time = 0;
    double getoneband_colourmap_time = 0;
    double render_time = 0;

    auto buffer_size = 0;

    Image output(sensor);

    Image dark_ref(sensor);
    Image dark_ref_white(sensor);
    Image white_ref(sensor);

    if (Utils::doesFileExist(DARK_REFERENCE_FILE)) {
        dark_ref = Image(sensor, DARK_REFERENCE_FILE);
    }
    else {
        std::cerr << "Failed to load dark reference object.\n";
    }

    if (Utils::doesFileExist(DARK_REFERENCE_WHITE_FILE)) {
        dark_ref_white = Image(sensor, DARK_REFERENCE_WHITE_FILE);
    }
    else {
        std::cerr << "Failed to load dark reference white.\n";
    }

    if (Utils::doesFileExist(WHITE_REFERENCE_FILE)) {
        white_ref = Image(sensor, WHITE_REFERENCE_FILE);
    }
    else {
        std::cerr << "Failed to load white reference.\n";
    }

    Handler handler(sensor, dark_ref, dark_ref_white, white_ref, EXPOSURE_TIME, EXPOSURE_TIME_WHITE_REFERENCE);

    // GLFW
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return EXIT_FAILURE;

    // Get monitor content scaling
    auto monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    float scale_x, scale_y;
    glfwGetMonitorContentScale(monitor, &scale_x, &scale_y);

    // Disable window resizing
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(screen_width * multiply, screen_height * multiply, "HyperspectralCalibration", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // Set starting position and zooming
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelZoom(multiply * scale_x, -multiply * scale_y);
    glRasterPos2i(-1, 1);

    // Setup XIMEA Camera
    HANDLE handle = NULL;
    XI_RETURN status = XI_OK;

    // Retrieving a handle to the camera device 
    status = xiOpenDevice(0, &handle);

    if (status != XI_OK) {
        std::cerr << "Error after xiOpenDevice\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Setting Exposure Time parameter in microseconds
    status = xiSetParamInt(handle, XI_PRM_EXPOSURE, EXPOSURE_TIME);

    if (status != XI_OK) {
        std::cerr << "Error after xiSetParam exposure time\n";
        program_return = EXIT_FAILURE;
        goto finish;
    }

    // Setting buffer policy to safe / unsafe
    status = xiSetParamInt(handle, XI_PRM_BUFFER_POLICY, XI_BP_UNSAFE);

    if (status != XI_OK) {
        std::cerr << "Error after xiSetParam buffer policy\n";
        program_return = EXIT_FAILURE;
        goto finish;
    }

    // Setting image data format to raw 16 bit
    status = xiSetParamInt(handle, XI_PRM_IMAGE_DATA_FORMAT, XI_RAW16);

    if (status != XI_OK) {
        std::cerr << "Error after xiSetParam format 16bit raw\n";
        program_return = EXIT_FAILURE;
        goto finish;
    }

    // Get XIMEA Image
    XI_IMG ximea_image;
    memset(&ximea_image, 0, sizeof(ximea_image));
    ximea_image.size = sizeof(XI_IMG);

    // Get the buffer size in bytes
    status = xiGetParamInt(handle, XI_PRM_IMAGE_PAYLOAD_SIZE, &buffer_size);

    // Check buffer_size corresponds to calibration file
    if (buffer_size != sensor.sensorWidth() * sensor.sensorHeight() * PIXEL_BYTE_SIZE) {
        std::cerr << "Buffer_size does not correspond to calibration file\n";
        std::cerr << "Buffer_size = " << buffer_size << "; calibration_file = " << sensor.sensorWidth() * sensor.sensorHeight() * PIXEL_BYTE_SIZE << "\n";
        program_return = EXIT_FAILURE;
        goto finish;
    }

    // Start acquisition
    status = xiStartAcquisition(handle);

    if (status != XI_OK) {
        std::cerr << "Error after xiStartAcquisition\n";
        program_return = EXIT_FAILURE;
        goto finish;
    }

    inAcquisition = true;

    try
    {
        if (COMPARISON_ITERATIONS > 0) {
            std::cout << "Comparison started with repeat time: " << COMPARISON_ITERATIONS << "\n";

            Image image(handler.getSensor());
            Image image_spectral(handler.getSensor());
            Image image_opencl(handler.getSensor());

            std::vector<uint16_t> output;
            std::vector<uint16_t> output_opencl;

            std::chrono::duration<double> get_image_time = std::chrono::duration<double>::zero();
            std::chrono::duration<double> render_time = std::chrono::duration<double>::zero();

            std::chrono::duration<double> offset_time = std::chrono::duration<double>::zero();
            std::chrono::duration<double> cube_reflection_time = std::chrono::duration<double>::zero();
            std::chrono::duration<double> spectral_time = std::chrono::duration<double>::zero();
            std::chrono::duration<double> getoneband_colourmap_time = std::chrono::duration<double>::zero();

            std::chrono::duration<double> offset_opencl_time = std::chrono::duration<double>::zero();
            std::chrono::duration<double> cube_reflection_opencl_time = std::chrono::duration<double>::zero();
            std::chrono::duration<double> spectral_opencl_time = std::chrono::duration<double>::zero();
            std::chrono::duration<double> getoneband_colourmap_opencl_time = std::chrono::duration<double>::zero();

            for (size_t i = 0; i < COMPARISON_ITERATIONS; i++) {
                std::cout << "Iteration " << i + 1 << " of " << COMPARISON_ITERATIONS << "\n";

                // Get image
                auto get_image_start = std::chrono::system_clock::now();
                status = xiGetImage(handle, 1000, &ximea_image);

                if (status != XI_OK) {
                    std::cerr << "Error after xiGetImage\n";
                    program_return = EXIT_FAILURE;
                    goto finish;
                }

                auto raw_data = reinterpret_cast<uint16_t*>(ximea_image.bp);

                // C++
                auto offset_start = std::chrono::system_clock::now();
                handler.offset(raw_data, image);
                auto cube_and_reflection_start = std::chrono::system_clock::now();
                handler.convertToCubeAndReflectionCorrection(image);
                auto spectral_start = std::chrono::system_clock::now();
                handler.spectralCorrection(image_spectral, image);
                auto getoneband_colourmap_start = std::chrono::system_clock::now();
                handler.getOneBandAndColourmap(output, image, 1);
                auto end = std::chrono::system_clock::now();

                // OpenCL
                auto offset_opencl_start = std::chrono::system_clock::now();
                handler.offsetOpenCL(raw_data, image_opencl);
                auto cube_and_reflection_opencl_start = std::chrono::system_clock::now();
                handler.convertToCubeAndReflectionCorrectionOpenCL(image);
                auto spectral_opencl_start = std::chrono::system_clock::now();
                handler.spectralCorrectionOpenCL(image, SPECTRAL_WORKGROUP1, SPECTRAL_WORKGROUP2);
                auto getoneband_colourmap_opencl_start = std::chrono::system_clock::now();
                handler.getOneBandAndColourmapOpenCL(output_opencl, image_opencl, 1);
                auto opencl_end = std::chrono::system_clock::now();

                // Render
                auto render_start = std::chrono::system_clock::now();
                glClear(GL_COLOR_BUFFER_BIT);
                glDrawPixels(screen_width, screen_height, GL_RGB, GL_UNSIGNED_SHORT, output_opencl.data());
                glfwSwapBuffers(window);
                glfwPollEvents();
                auto render_end = std::chrono::system_clock::now();

                // Get image
                get_image_time += offset_start - get_image_start;

                // Render
                render_time += render_end - render_start;

                // Sum C++
                offset_time += cube_and_reflection_start - offset_start;
                cube_reflection_time += spectral_start - cube_and_reflection_start;
                spectral_time += getoneband_colourmap_start - spectral_start;
                getoneband_colourmap_time += end - getoneband_colourmap_start;

                // Sum OpenCL
                offset_opencl_time += cube_and_reflection_opencl_start - offset_opencl_start;
                cube_reflection_opencl_time += spectral_opencl_start - cube_and_reflection_opencl_start;
                spectral_opencl_time += getoneband_colourmap_opencl_start - spectral_opencl_start;
                getoneband_colourmap_opencl_time += opencl_end - getoneband_colourmap_opencl_start;
            }

            std::cout << "\n";

            std::chrono::duration<double> best_offset_time = offset_time;
            std::chrono::duration<double> best_cube_reflection_time = cube_reflection_opencl_time;
            std::chrono::duration<double> best_spectral_time = spectral_opencl_time;
            std::chrono::duration<double> best_getoneband_colourmap_time = getoneband_colourmap_time;

            auto sum_time = get_image_time + offset_time + cube_reflection_time + spectral_time + getoneband_colourmap_time + render_time;
            auto sum_opencl_time = get_image_time + offset_opencl_time + cube_reflection_opencl_time + spectral_opencl_time + getoneband_colourmap_opencl_time + render_time;
            auto best_time = get_image_time + best_offset_time + best_cube_reflection_time + best_spectral_time + best_getoneband_colourmap_time + render_time;

            std::cout << "----- RESULTS (C++ / OpenCL) -----\n";
            std::cout << "Number of repeats: " << COMPARISON_ITERATIONS << "\n";
            std::cout << "Total time: " << sum_time.count() << "s --- " << sum_opencl_time.count() << "s\n";

            std::cout << "\n----- AVERAGE GET IMAGE TIME -----\n";
            std::cout << "Get image time = " << get_image_time.count() / COMPARISON_ITERATIONS << "s --- " << 1.0f / (get_image_time.count() / COMPARISON_ITERATIONS) << "fps\n";

            std::cout << "\n----- AVERAGE RENDER TIME -----\n";
            std::cout << "Render time = " << render_time.count() / COMPARISON_ITERATIONS << "s --- " << 1.0f / (render_time.count() / COMPARISON_ITERATIONS) << "fps\n";

            std::cout << "\n----- AVERAGE TIME PER FRAME (C++ / OpenCL) -----\n";
            std::cout << "Offset correction = " << offset_time.count() / COMPARISON_ITERATIONS << "s --- " << offset_opencl_time.count() / COMPARISON_ITERATIONS << "s\n";
            std::cout << "Convert to cube + reflection correction = " << cube_reflection_time.count() / COMPARISON_ITERATIONS << "s --- " << cube_reflection_opencl_time.count() / COMPARISON_ITERATIONS << "s\n";
            std::cout << "Spectral correction = " << spectral_time.count() / COMPARISON_ITERATIONS << "s --- " << spectral_opencl_time.count() / COMPARISON_ITERATIONS << "s\n";
            std::cout << "GetOneBand + colourmap = " << getoneband_colourmap_time.count() / COMPARISON_ITERATIONS << "s --- " << getoneband_colourmap_opencl_time.count() / COMPARISON_ITERATIONS << "s\n";
            std::cout << "Sum " << sum_time.count() / COMPARISON_ITERATIONS << "s --- " << sum_opencl_time.count() / COMPARISON_ITERATIONS << "s\n";

            std::cout << "\n----- AVERAGE FPS (C++ / OpenCL) -----\n";
            std::cout << "Offset correction = " << 1.0f / (offset_time.count() / COMPARISON_ITERATIONS) << "fps --- " << 1.0f / (offset_opencl_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "Convert to cube + reflection correction = " << 1.0f / (cube_reflection_time.count() / COMPARISON_ITERATIONS) << "fps --- " << 1.0f / (cube_reflection_opencl_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "Spectral correction = " << 1.0f / (spectral_time.count() / COMPARISON_ITERATIONS) << "fps --- " << 1.0f / (spectral_opencl_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "GetOneBand + colourmap = " << 1.0f / (getoneband_colourmap_time.count() / COMPARISON_ITERATIONS) << "fps --- " << 1.0f / (getoneband_colourmap_opencl_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "Sum = " << 1.0f / (sum_time.count() / COMPARISON_ITERATIONS) << "fps --- " << 1.0f / (sum_opencl_time.count() / COMPARISON_ITERATIONS) << "fps\n";

            std::cout << "\n----- Performance gain (OpenCL compared to C++) -----\n";
            std::cout << std::round((sum_time.count() / COMPARISON_ITERATIONS) / (sum_opencl_time.count() / COMPARISON_ITERATIONS) * 100) << "%\n";

            std::cout << "\n----- AVERAGE BEST TIME AND FPS -----\n";
            std::cout << "Offset correction = " << best_offset_time.count() / COMPARISON_ITERATIONS << "s --- " << 1.0f / (best_offset_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "Convert to cube + reflection correction = " << best_cube_reflection_time.count() / COMPARISON_ITERATIONS << "fps --- " << 1.0f / (best_cube_reflection_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "Spectral correction = " << best_spectral_time.count() / COMPARISON_ITERATIONS << "s --- " << 1.0f / (best_spectral_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "GetOneBand + colourmap = " << best_getoneband_colourmap_time.count() / COMPARISON_ITERATIONS << "fps --- " << 1.0f / (best_getoneband_colourmap_time.count() / COMPARISON_ITERATIONS) << "fps\n";
            std::cout << "Sum = " << best_time.count() / COMPARISON_ITERATIONS << "s --- " << 1.0f / (best_time.count() / COMPARISON_ITERATIONS) << "fps\n";

            std::cout << "\nComparison done.\n";
        }

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window)) {
            auto start = std::chrono::system_clock::now();
            // Getting image from camera
            status = xiGetImage(handle, 1000, &ximea_image);

            if (status != XI_OK) {
                std::cerr << "Error after xiGetImage\n";
                program_return = EXIT_FAILURE;
                goto finish;
            }

            auto raw_data = reinterpret_cast<uint16_t*>(ximea_image.bp);
            Image image(sensor);
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> time = end - start;
            get_image_time = time.count();

            start = std::chrono::system_clock::now();
            handler.offset(raw_data, image);
            end = std::chrono::system_clock::now();
            time = end - start;
            offset_correction_time = time.count();

            if (GetKeyState('W') & KEY_PRESS_MASK) {
                takeWhiteReference(handler, image);
            }

            if (GetKeyState('D') & KEY_PRESS_MASK) {
                takeDarkReference(handler, image);
            }

            if (GetKeyState('A') & KEY_PRESS_MASK) {
                takeDarkReferenceWhite(handler, image);
            }

            if (GetKeyState('I') & KEY_PRESS_MASK) {
                printInfo(EXPOSURE_TIME, band_index, get_image_time, offset_correction_time, converttocube_reflection_correction_time, spectral_correction_time, getoneband_colourmap_time, render_time);
            }

            if (GetKeyState(VK_SPACE) & KEY_PRESS_MASK) {
                takeImage(image);
            }

            if (GetKeyState(VK_RIGHT) & KEY_PRESS_MASK) {
                incrementBand(sensor, band_index);
            }

            if (GetKeyState(VK_LEFT) & KEY_PRESS_MASK) {
                decrementBand(sensor, band_index);
            }

            if (GetKeyState(VK_ESCAPE) & KEY_PRESS_MASK) {
                std::cout << "Exiting program.\n";
                break;
            }

            start = std::chrono::system_clock::now();
            handler.convertToCubeAndReflectionCorrectionOpenCL(image);
            end = std::chrono::system_clock::now();
            time = end - start;
            converttocube_reflection_correction_time = time.count();

            start = std::chrono::system_clock::now();
            handler.spectralCorrectionOpenCL(image, SPECTRAL_WORKGROUP1, SPECTRAL_WORKGROUP2);
            end = std::chrono::system_clock::now();
            time = end - start;
            spectral_correction_time = time.count();

            std::vector<uint16_t> pixels;
            start = std::chrono::system_clock::now();
            handler.getOneBandAndColourmap(pixels, image, band_index);
            end = std::chrono::system_clock::now();
            time = end - start;
            getoneband_colourmap_time = time.count();

            start = std::chrono::system_clock::now();
            /* Render here */
            glClear(GL_COLOR_BUFFER_BIT);

            glDrawPixels(screen_width, screen_height, GL_RGB, GL_UNSIGNED_SHORT, pixels.data());

            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
            end = std::chrono::system_clock::now();
            time = end - start;
            render_time = time.count();
        }
    }
    catch (const std::exception& e)
    {
        program_return = EXIT_FAILURE;
        std::cerr << "Exception thrown:\n";
        std::cerr << e.what();
        goto finish;
    }

finish:
    if (inAcquisition) {
        status = xiStopAcquisition(handle);

        if (status != XI_OK) {
            std::cerr << "Error after xiStopAcquisition\n";
            program_return = EXIT_FAILURE;
        }
    }

    if (handle) {
        xiCloseDevice(handle);
    }

    glfwTerminate();

    return program_return;
}