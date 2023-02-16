#pragma once

#include <CL/cl.hpp>

#include "image.hpp"
#include "sensor.hpp"

#include <vector>

class Sensor;

class Handler {
    Sensor sensor_;

    Image dark_reference_object_;
    Image dark_reference_white_;
    Image white_reference_;
    cl::Buffer coefficients_buffer_;
    cl::Buffer dark_reference_object_buffer_;
    cl::Buffer dark_reference_white_buffer_;
    cl::Buffer white_reference_buffer_;
    unsigned int exposure_time_object_;
    unsigned int exposure_time_white_reference_;

    // Attributes for OpenCL
    std::vector<cl::Device> devices_;
    cl::Device device_;
    cl::Context context_;
    cl::CommandQueue queue_;
    cl::Program::Sources convert_to_cube_and_reflection_correction_source_;
    cl::Program::Sources spectral_correction_source_;
    cl::Program::Sources offset_correction_source_;
    cl::Program::Sources get_one_band_and_colourmap_source_;
    cl::Program convert_to_cube_and_reflection_correction_program_;
    cl::Program spectral_correction_program_;
    cl::Program offset_correction_program_;
    cl::Program get_one_band_and_colourmap_program_;
    cl::Kernel convert_to_cube_and_reflection_correction_kernel_;
    cl::Kernel spectral_correction_kernel_;
    cl::Kernel offset_correction_kernel_;
    cl::Kernel get_one_band_and_colourmap_kernel_;

    void updateWhiteReferenceBuffer();
    void updateDarkReferenceObjectBuffer();
    void updateDarkReferenceWhiteBuffer();

public:
    // Constructor
    Handler(const Sensor& sensor,
            const Image& dark_reference_object,
            const Image& dark_reference_white,
            const Image& white_reference,
            unsigned int exposure_time_object,
            unsigned int exposure_time_white_reference);

    // Getters
    Sensor getSensor() const { return sensor_; };

    // Setters
    void setWhiteReference(const Image& white_reference);
    void setDarkReferenceObject(const Image& dark_reference_object);
    void setDarkReferenceWhite(const Image& dark_reference_white);

    // Offset correction from raw data
    void offsetOpenCL(uint16_t* input, Image& output);
    void offset(uint16_t* input, Image& output);

    // Converts raw image data to cube image data and performs relfection correction
    void convertToCubeAndReflectionCorrection(Image& image);
    void convertToCubeAndReflectionCorrectionOpenCL(Image& image);

    // Spectral correction - Cube data used!
    void spectralCorrection(Image& output, const Image& input);
    void spectralCorrectionOpenCL(Image& image, unsigned int workgroup_1, unsigned int workgroup_2);

    // Retrieve one band - Cube data used!
    // band_index is a value from <0, numberOfBands - 1>
    void getOneBandAndColourmap(std::vector<uint16_t>& output, const Image& input, unsigned int band_index);
    void getOneBandAndColourmapOpenCL(std::vector<uint16_t>& output, const Image& input, unsigned int band_index);
};
