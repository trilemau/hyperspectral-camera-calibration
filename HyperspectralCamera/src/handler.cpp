#include "handler.hpp"

#include "filepaths.hpp"

#include <cmath>
#include <fstream>
#include <iostream>

Handler::Handler(const Sensor& sensor,
    const Image& dark_reference_object,
    const Image& dark_reference_white,
    const Image& white_reference,
    unsigned int exposure_time_object_ns,
    unsigned int exposure_time_white_reference)
    : sensor_(sensor)
    , dark_reference_object_(dark_reference_object)
    , dark_reference_white_(dark_reference_white)
    , white_reference_(white_reference)
    , exposure_time_object_(exposure_time_object_ns)
    , exposure_time_white_reference_(exposure_time_white_reference) {
    std::ifstream convert_to_cube_and_reflection_correction_file(CONVERT_TO_CUBE_AND_REFLECTION_CORRECTION_FILE);
    std::ifstream spectral_correction_file(SPECTRAL_CORRECTION_FILE);
    std::ifstream offset_file(OFFSET_FILE);
    std::ifstream get_one_band_and_colourmap_file(GET_ONE_BAND_AND_COLOURMAP);

    std::string convert_to_cube_and_reflection_correction_source{ std::istreambuf_iterator<char>(convert_to_cube_and_reflection_correction_file), std::istreambuf_iterator<char>() };
    std::string spectral_correction_source{ std::istreambuf_iterator<char>(spectral_correction_file), std::istreambuf_iterator<char>() };
    std::string offset_correction_source{ std::istreambuf_iterator<char>(offset_file), std::istreambuf_iterator<char>() };
    std::string get_one_band_and_colourmap_source{ std::istreambuf_iterator<char>(get_one_band_and_colourmap_file), std::istreambuf_iterator<char>() };

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    auto platform = platforms.front();
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices_);

    if (devices_.size() == 0) {
        throw std::runtime_error("OpenCL 0 devices");
    }

    device_ = devices_.front();
    context_ = cl::Context(device_);

    convert_to_cube_and_reflection_correction_source_ = cl::Program::Sources({ std::make_pair(convert_to_cube_and_reflection_correction_source.c_str(), convert_to_cube_and_reflection_correction_source.size() + 1) });
    spectral_correction_source_ = cl::Program::Sources({ std::make_pair(spectral_correction_source.c_str(), spectral_correction_source.size() + 1) });
    offset_correction_source_ = cl::Program::Sources({ std::make_pair(offset_correction_source.c_str(), offset_correction_source.size() + 1) });
    get_one_band_and_colourmap_source_ = cl::Program::Sources({ std::make_pair(get_one_band_and_colourmap_source.c_str(), get_one_band_and_colourmap_source.size() + 1) });

    convert_to_cube_and_reflection_correction_program_ = cl::Program(context_, convert_to_cube_and_reflection_correction_source_);
    spectral_correction_program_ = cl::Program(context_, spectral_correction_source_);
    offset_correction_program_ = cl::Program(context_, offset_correction_source_);
    get_one_band_and_colourmap_program_ = cl::Program(context_, get_one_band_and_colourmap_source_);

    // Initialize programs
    auto error = convert_to_cube_and_reflection_correction_program_.build(devices_);

    if (error != 0) {
        throw std::runtime_error("OpenCL convert to cube and reflection correction program build error");
    }

    error = spectral_correction_program_.build();

    if (error != 0) {
        throw std::runtime_error("OpenCL spectral correction program build error");
    }

    error = offset_correction_program_.build();

    if (error != 0) {
        throw std::runtime_error("OpenCL offset correction program build error");
    }

    error = get_one_band_and_colourmap_program_.build();

    if (error != 0) {
        throw std::runtime_error("OpenCL get one band and colourmap program build error");
    }

    // Initialize queue
    queue_ = cl::CommandQueue(context_, device_);

    // Initialize kernels
    convert_to_cube_and_reflection_correction_kernel_ = cl::Kernel(convert_to_cube_and_reflection_correction_program_, "ConvertToCubeAndReflectionCorrection", &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL convert to cube and reflection correction kernel error");
    }

    spectral_correction_kernel_ = cl::Kernel(spectral_correction_program_, "SpectralCorrection", &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL spectral correction kernel error");
    }

    offset_correction_kernel_ = cl::Kernel(offset_correction_program_, "OffsetCorrection", &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL offset correction kernel error");
    }

    get_one_band_and_colourmap_kernel_ = cl::Kernel(get_one_band_and_colourmap_program_, "GetOneBandAndColourmap", &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL get one band and colourmap kernel error");
    }

    // Initialize buffers
    coefficients_buffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(float) * sensor_.coefficients().size(), sensor_.mutableCoefficients().data(), &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL coefficient buffer error");
    }

    updateDarkReferenceObjectBuffer();
    updateDarkReferenceWhiteBuffer();
    updateWhiteReferenceBuffer();
}

void Handler::updateWhiteReferenceBuffer() {
    cl_int error;

    white_reference_buffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(uint16_t) * white_reference_.size(), white_reference_.mutableData().data(), &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL white reference buffer error");
    }
}

void Handler::updateDarkReferenceObjectBuffer() {
    cl_int error;

    dark_reference_object_buffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(uint16_t) * dark_reference_object_.size(), dark_reference_object_.mutableData().data(), &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL dark reference object buffer error");
    }
}

void Handler::updateDarkReferenceWhiteBuffer() {
    cl_int error;

    dark_reference_white_buffer_ = cl::Buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(uint16_t) * dark_reference_white_.size(), dark_reference_white_.mutableData().data(), &error);

    if (error != 0) {
        throw std::runtime_error("OpenCL dark reference white buffer error");
    }
}

void Handler::setWhiteReference(const Image& white_reference) {
    white_reference_ = white_reference;
    updateWhiteReferenceBuffer();
}

void Handler::setDarkReferenceObject(const Image& dark_reference_object) {
    dark_reference_object_ = dark_reference_object;
    updateDarkReferenceObjectBuffer();
}

void Handler::setDarkReferenceWhite(const Image& dark_reference_white) {
    dark_reference_white_ = dark_reference_white;
    updateDarkReferenceWhiteBuffer();
}

void Handler::offsetOpenCL(uint16_t* input, Image& output) {
    // Resize output
    output.mutableData().resize(static_cast<uint64_t>(sensor_.activeAreaWidth()) * sensor_.activeAreaHeight());

    cl_int error;

    cl::Buffer input_buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(uint16_t) * sensor_.sensorWidth() * sensor_.sensorHeight(), input, &error);
    cl::Buffer output_buffer(context_, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_READ_ONLY, sizeof(uint16_t) * output.size(), output.mutableData().data(), &error);

    error = offset_correction_kernel_.setArg(0, input_buffer);
    error = offset_correction_kernel_.setArg(1, output_buffer);
    error = offset_correction_kernel_.setArg(2, sensor_.offsetX());
    error = offset_correction_kernel_.setArg(3, sensor_.offsetY());
    error = offset_correction_kernel_.setArg(4, sensor_.activeAreaWidth());
    error = offset_correction_kernel_.setArg(5, sensor_.activeAreaHeight());

    error = queue_.enqueueNDRangeKernel(offset_correction_kernel_, cl::NullRange, cl::NDRange(sensor_.sensorWidth(), sensor_.sensorHeight()));
    queue_.enqueueMapBuffer(output_buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(uint16_t) * output.size(), {}, {}, &error);
}

void Handler::offset(uint16_t* input, Image& output) {
    // Resize output
    output.mutableData().resize(static_cast<uint64_t>(sensor_.activeAreaWidth()) * sensor_.activeAreaHeight());

    auto end_x = sensor_.offsetX() + sensor_.activeAreaWidth();
    auto end_y = sensor_.offsetY() + sensor_.activeAreaHeight();

    for (size_t y = 0; y < sensor_.sensorHeight(); y++) {
        for (size_t x = 0; x < sensor_.sensorWidth(); x++) {
            if (x >= sensor_.offsetX() && x < end_x && y >= sensor_.offsetY() && y < end_y) {
                auto i = (x - sensor_.offsetX()) + sensor_.activeAreaWidth() * (y - sensor_.offsetY());
                output.mutablePixel(i) = input[x + sensor_.sensorWidth() * y];
            }
        }
    }
}

void Handler::convertToCubeAndReflectionCorrection(Image& image) {
    // Resize not needed, it is done in Image constructor

    for (size_t y = 0; y < sensor_.spatialHeight(); y++) {
        for (size_t x = 0; x < sensor_.spatialWidth(); x++) {
            auto cube_width = sensor_.spatialWidth() * sensor_.numberOfBands();
            auto pixel_start = x * sensor_.numberOfBands() + y * cube_width;

            for (size_t band_y = 0; band_y < sensor_.patternHeight(); band_y++) {
                for (size_t band_x = 0; band_x < sensor_.patternWidth(); band_x++) {
                    size_t i = image.getArrayIndex(x * sensor_.patternWidth() + band_x, y * sensor_.patternHeight() + band_y);

                    int object = image.pixel(i) - dark_reference_object_.pixel(i);

                    if (object < 0) {
                        object = 0;
                    }

                    const int white = white_reference_.pixel(i) - dark_reference_white_.pixel(i);
                    const float object_time_ratio = static_cast<float>(object) / white;
                    const float time_ratio = static_cast<float>(exposure_time_white_reference_) / exposure_time_object_;
                    const float v = object_time_ratio * time_ratio;

                    float result = v * PIXEL_MAX;

                    if (result > PIXEL_MAX) {
                        result = PIXEL_MAX;
                    }

                    const auto result_uint16t = static_cast<uint16_t>(result);
                    image.mutablePixelCube(pixel_start) = result_uint16t;

                    pixel_start++;
                }
            }
        }
    }
}

void Handler::convertToCubeAndReflectionCorrectionOpenCL(Image& image) {
    cl_int error;

    cl::Buffer cube_buffer(context_, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_READ_ONLY, sizeof(uint16_t) * image.size(), image.mutableCube().data(), &error);
    cl::Buffer input_buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(uint16_t) * image.size(), image.mutableData().data(), &error);

    error = convert_to_cube_and_reflection_correction_kernel_.setArg(0, input_buffer);
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(1, cube_buffer);
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(2, sensor_.activeAreaWidth());
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(3, sensor_.patternWidth());
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(4, sensor_.patternHeight());
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(5, dark_reference_object_buffer_);
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(6, dark_reference_white_buffer_);
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(7, white_reference_buffer_);
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(8, exposure_time_white_reference_);
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(9, exposure_time_object_);
    error = convert_to_cube_and_reflection_correction_kernel_.setArg(10, PIXEL_MAX);

    error = queue_.enqueueNDRangeKernel(convert_to_cube_and_reflection_correction_kernel_, cl::NullRange, cl::NDRange(sensor_.spatialWidth(), sensor_.spatialHeight()));
    queue_.enqueueMapBuffer(cube_buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(uint16_t) * image.size(), {}, {}, &error);
}

void Handler::spectralCorrection(Image& output, const Image& input) {
    // check images are same size
    if (input.size() != output.size()) {
        throw std::runtime_error("Handler::spectralCorrection images are not the same size");
    }

    for (size_t pixel_start = 0; pixel_start < input.size(); pixel_start += sensor_.numberOfBands()) {
        for (size_t band = 0; band < sensor_.numberOfBands(); band++) {
            auto output_index = pixel_start + band;
            float result = 0;

            for (size_t i = 0; i < sensor_.numberOfBands(); i++) {
                auto coefficient_index = sensor_.numberOfBands() * band + i;
                auto input_index = pixel_start + i;
                result += sensor_.coefficients()[coefficient_index] * input.pixelCube(input_index);
            }

            if (result > PIXEL_MAX) {
                output.mutablePixelCube(output_index) = PIXEL_MAX;
            } 
            else {
                output.mutablePixelCube(output_index) = static_cast<uint16_t>(result);
            }
        }
    }
}

void Handler::spectralCorrectionOpenCL(Image& image, unsigned int workgroup_1, unsigned int workgroup_2) {
    cl_int error;

    cl::Buffer output_buffer(context_, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_READ_ONLY, sizeof(uint16_t) * image.size(), image.mutableCube().data(), &error);
    cl::Buffer input_buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(uint16_t) * image.size(), image.mutableCube().data(), &error);

    error = spectral_correction_kernel_.setArg(0, output_buffer);
    error = spectral_correction_kernel_.setArg(1, input_buffer);
    error = spectral_correction_kernel_.setArg(2, coefficients_buffer_);
    error = spectral_correction_kernel_.setArg(3, sensor_.activeAreaWidth());
    error = spectral_correction_kernel_.setArg(4, sensor_.numberOfBands());

    error = queue_.enqueueNDRangeKernel(spectral_correction_kernel_, cl::NullRange, cl::NDRange(static_cast<uint64_t>(sensor_.spatialWidth()) * sensor_.spatialHeight(), sensor_.numberOfBands()), cl::NDRange(workgroup_1, workgroup_2));
    queue_.enqueueMapBuffer(output_buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(uint16_t) * image.size(), {}, {}, &error);
}

void Handler::getOneBandAndColourmap(std::vector<uint16_t>& output, const Image& input, unsigned int band_index) {
    if (band_index >= sensor_.numberOfBands()) {
        throw std::runtime_error("getOneBandAndColourmap band_index outside of number of bands range.");
    }

    // Resize output to fit RGB
    output.resize(static_cast<uint64_t>(sensor_.spatialWidth()) * sensor_.spatialHeight() * COLOURS_PER_PIXEL);

    auto colour1_r = (float)1 / 255 * SHORT_MAX;
    auto colour1_g = (float)0 / 255 * SHORT_MAX;
    auto colour1_b = (float)4 / 255 * SHORT_MAX;

    auto colour2_r = (float)48 / 255 * SHORT_MAX;
    auto colour2_g = (float)7 / 255 * SHORT_MAX;
    auto colour2_b = (float)84 / 255 * SHORT_MAX;

    auto colour3_r = (float)105 / 255 * SHORT_MAX;
    auto colour3_g = (float)15 / 255 * SHORT_MAX;
    auto colour3_b = (float)111 / 255 * SHORT_MAX;

    auto colour4_r = (float)158 / 255 * SHORT_MAX;
    auto colour4_g = (float)40 / 255 * SHORT_MAX;
    auto colour4_b = (float)100 / 255 * SHORT_MAX;

    auto colour5_r = (float)208 / 255 * SHORT_MAX;
    auto colour5_g = (float)72 / 255 * SHORT_MAX;
    auto colour5_b = (float)67 / 255 * SHORT_MAX;

    auto colour6_r = (float)239 / 255 * SHORT_MAX;
    auto colour6_g = (float)125 / 255 * SHORT_MAX;
    auto colour6_b = (float)21 / 255 * SHORT_MAX;

    auto colour7_r = (float)242 / 255 * SHORT_MAX;
    auto colour7_g = (float)194 / 255 * SHORT_MAX;
    auto colour7_b = (float)35 / 255 * SHORT_MAX;

    auto colour8_r = (float)245 / 255 * SHORT_MAX;
    auto colour8_g = (float)255 / 255 * SHORT_MAX;
    auto colour8_b = (float)163 / 255 * SHORT_MAX;

    auto divide = 7;
    float part = 1.0f / divide;

    for (size_t spatial_y = 0; spatial_y < sensor_.spatialHeight(); spatial_y++) {
        for (size_t spatial_x = 0; spatial_x < sensor_.spatialWidth(); spatial_x++) {
            auto input_index = spatial_x * sensor_.numberOfBands() + spatial_y * sensor_.spatialWidth() * sensor_.numberOfBands() + band_index;
            auto output_index = spatial_x * COLOURS_PER_PIXEL + spatial_y * sensor_.spatialWidth() * COLOURS_PER_PIXEL;

            auto ratio = (float) input.pixelCube(input_index) / PIXEL_MAX;

            if (ratio < 1 * part) {
                float colour_ratio = (ratio - 0 * part) / part;
                output[output_index + 0] = static_cast<uint16_t>(colour1_r + colour_ratio * (colour2_r - colour1_r));
                output[output_index + 1] = static_cast<uint16_t>(colour1_g + colour_ratio * (colour2_g - colour1_g));
                output[output_index + 2] = static_cast<uint16_t>(colour1_b + colour_ratio * (colour2_b - colour1_b));
            }
            else if (ratio < 2 * part) {
                float colour_ratio = (ratio - 1 * part) / part;
                output[output_index + 0] = static_cast<uint16_t>(colour2_r + colour_ratio * (colour3_r - colour2_r));
                output[output_index + 1] = static_cast<uint16_t>(colour2_g + colour_ratio * (colour3_g - colour2_g));
                output[output_index + 2] = static_cast<uint16_t>(colour2_b + colour_ratio * (colour3_b - colour2_b));
            }
            else if (ratio < 3 * part) {
                float colour_ratio = (ratio - 2 * part) / part;
                output[output_index + 0] = static_cast<uint16_t>(colour3_r + colour_ratio * (colour4_r - colour3_r));
                output[output_index + 1] = static_cast<uint16_t>(colour3_g + colour_ratio * (colour4_g - colour3_g));
                output[output_index + 2] = static_cast<uint16_t>(colour3_b + colour_ratio * (colour4_b - colour3_b));
            }
            else if (ratio < 4 * part) {
                float colour_ratio = (ratio - 3 * part) / part;
                output[output_index + 0] = static_cast<uint16_t>(colour4_r + colour_ratio * (colour5_r - colour4_r));
                output[output_index + 1] = static_cast<uint16_t>(colour4_g + colour_ratio * (colour5_g - colour4_g));
                output[output_index + 2] = static_cast<uint16_t>(colour4_b + colour_ratio * (colour5_b - colour4_b));
            }
            else if (ratio < 5 * part) {
                float colour_ratio = (ratio - 4 * part) / part;
                output[output_index + 0] = static_cast<uint16_t>(colour5_r + colour_ratio * (colour6_r - colour5_r));
                output[output_index + 1] = static_cast<uint16_t>(colour5_g + colour_ratio * (colour6_g - colour5_g));
                output[output_index + 2] = static_cast<uint16_t>(colour5_b + colour_ratio * (colour6_b - colour5_b));
            }
            else if (ratio < 6 * part) {
                float colour_ratio = (ratio - 5 * part) / part;
                output[output_index + 0] = static_cast<uint16_t>(colour6_r + colour_ratio * (colour7_r - colour6_r));
                output[output_index + 1] = static_cast<uint16_t>(colour6_g + colour_ratio * (colour7_g - colour6_g));
                output[output_index + 2] = static_cast<uint16_t>(colour6_b + colour_ratio * (colour7_b - colour6_b));
            }
            else {
                float colour_ratio = (ratio - 6 * part) / part;
                output[output_index + 0] = static_cast<uint16_t>(colour7_r + colour_ratio * (colour8_r - colour7_r));
                output[output_index + 1] = static_cast<uint16_t>(colour7_g + colour_ratio * (colour8_g - colour7_g));
                output[output_index + 2] = static_cast<uint16_t>(colour7_b + colour_ratio * (colour8_b - colour7_b));
            }
        }
    }
}

void Handler::getOneBandAndColourmapOpenCL(std::vector<uint16_t>& output, const Image& input, unsigned int band_index) {
    if (band_index >= sensor_.numberOfBands()) {
        throw std::runtime_error("getOneBandAndColourmapOpenCL band_index outside of number of bands range.");
    }

    // Resize output to fit RGB
    output.resize(static_cast<uint64_t>(sensor_.spatialWidth()) * sensor_.spatialHeight() * COLOURS_PER_PIXEL);

    cl_int error;

    cl::Buffer input_buffer(context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, sizeof(uint16_t) * input.size(), (void*) input.cube().data(), &error);
    cl::Buffer output_buffer(context_, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_READ_ONLY, sizeof(uint16_t) * output.size(), output.data(), &error);

    error = get_one_band_and_colourmap_kernel_.setArg(0, input_buffer);
    error = get_one_band_and_colourmap_kernel_.setArg(1, output_buffer);
    error = get_one_band_and_colourmap_kernel_.setArg(2, band_index);
    error = get_one_band_and_colourmap_kernel_.setArg(3, sensor_.numberOfBands());

    error = queue_.enqueueNDRangeKernel(get_one_band_and_colourmap_kernel_, cl::NullRange, cl::NDRange(sensor_.spatialWidth(), sensor_.spatialHeight()), cl::NDRange(sensor_.spatialWidth(), 2));
    error = queue_.enqueueReadBuffer(output_buffer, CL_TRUE, 0, sizeof(uint16_t) * output.size(), output.data());
}