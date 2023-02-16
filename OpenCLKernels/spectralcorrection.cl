#define PIXEL_MAX 1023

kernel void SpectralCorrection(
    global unsigned short* output,
    constant const unsigned short* input,
    constant const float* coefficients,
    unsigned int width,
    unsigned int number_of_bands)
{
    unsigned int spatial_pixel = get_global_id(0);
    unsigned int band = get_global_id(1);
    unsigned int pixel_start = spatial_pixel * number_of_bands;

    unsigned int output_index = pixel_start + band;
    
    float result = 0;

    for (size_t i = 0; i < number_of_bands; i++) {
        size_t coefficient_index = number_of_bands * band + i;
        size_t input_index = pixel_start + i;
        result += coefficients[coefficient_index] * input[input_index];
    }

    if (result > PIXEL_MAX) {
        output[output_index] = PIXEL_MAX;
    }
    else {
        output[output_index] = (unsigned short) result;
    }
}