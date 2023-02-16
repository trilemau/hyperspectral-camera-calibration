#ifdef __APPLE__
#elif _WIN64
    #define OFFSET_FILE R"(../OpenCLKernels/offsetcorrection.cl)"
    #define CONVERT_TO_CUBE_AND_REFLECTION_CORRECTION_FILE R"(../OpenCLKernels/converttocube_reflectioncorrection.cl)"
    #define SPECTRAL_CORRECTION_FILE R"(../OpenCLKernels/spectralcorrection.cl)"
    #define GET_ONE_BAND_AND_COLOURMAP R"(../OpenCLKernels/getoneband_colourmap.cl)"
    #define CALIBRATION_FILE R"(../CalibrationFile/CMV2K-SSM5x5-600_1000-5.6.16.11.xml)" // Calibration file is not included in the project
    #define WHITE_REFERENCE_FILE R"(resources/white_reference.raw)"
    #define DARK_REFERENCE_FILE R"(resources/dark_reference.raw)"
    #define DARK_REFERENCE_WHITE_FILE R"(resources/dark_reference_white.raw)"
    #define SNAPSHOT_FOLDER R"(snapshots/)"
#endif