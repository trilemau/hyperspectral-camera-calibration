#define PIXEL_MAX 1023

kernel void ConvertToCubeAndReflectionCorrection(
    constant const unsigned short* input,
    global unsigned short* cube,
    unsigned int width,
    unsigned int pixel_width,
    unsigned int pixel_height,
    constant const unsigned short* dark_ref_object,
    constant const unsigned short* dark_ref_white,
    constant const unsigned short* white_ref,
    unsigned int exposure_time_white_ref,
    unsigned int exposure_time_object)
{
    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);

    unsigned int spatial_width = get_global_size(0);
    unsigned int number_of_bands = pixel_width * pixel_height;

    unsigned int cube_width = spatial_width * number_of_bands;
    size_t pixel_start = x * number_of_bands + y * cube_width;

    for (size_t band_y = 0; band_y < pixel_height; band_y++) {
        for (size_t band_x = 0; band_x < pixel_width; band_x++) {
            size_t i = (x * pixel_width + band_x) + (width * (y * pixel_height + band_y));

            int object = input[i] - dark_ref_object[i];

            if (object < 0) {
                object = 0;
            }

            const int white = white_ref[i] - dark_ref_white[i];
            const float object_time_ratio = (float) object / white;
            const float time_ratio = (float) exposure_time_white_ref / exposure_time_object;
            const float v = object_time_ratio * time_ratio;

            float result = PIXEL_MAX * v;

            if (result > PIXEL_MAX) {
                result = PIXEL_MAX;
            }

            const unsigned short result_short = (unsigned short) result;

            cube[pixel_start] = result_short;

            pixel_start++;
        }
    }
}