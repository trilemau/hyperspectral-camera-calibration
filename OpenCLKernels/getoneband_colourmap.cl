#define PIXEL_MAX 1023
#define SHORT_MAX 65535

kernel void GetOneBandAndColourmap(
    global const unsigned short* input,
    global unsigned short* output,
    unsigned int band_index,
    unsigned int number_of_bands)
{
    float part = 1.0 / 7;

    float color1_r = (float)1 / 255 * SHORT_MAX;
    float color1_g = (float)0 / 255 * SHORT_MAX;
    float color1_b = (float)4 / 255 * SHORT_MAX;

    float color2_r = (float)48 / 255 * SHORT_MAX;
    float color2_g = (float)7 / 255 * SHORT_MAX;
    float color2_b = (float)84 / 255 * SHORT_MAX;

    float color3_r = (float)105 / 255 * SHORT_MAX;
    float color3_g = (float)15 / 255 * SHORT_MAX;
    float color3_b = (float)111 / 255 * SHORT_MAX;

    float color4_r = (float)158 / 255 * SHORT_MAX;
    float color4_g = (float)40 / 255 * SHORT_MAX;
    float color4_b = (float)100 / 255 * SHORT_MAX;

    float color5_r = (float)208 / 255 * SHORT_MAX;
    float color5_g = (float)72 / 255 * SHORT_MAX;
    float color5_b = (float)67 / 255 * SHORT_MAX;

    float color6_r = (float)239 / 255 * SHORT_MAX;
    float color6_g = (float)125 / 255 * SHORT_MAX;
    float color6_b = (float)21 / 255 * SHORT_MAX;

    float color7_r = (float)242 / 255 * SHORT_MAX;
    float color7_g = (float)194 / 255 * SHORT_MAX;
    float color7_b = (float)35 / 255 * SHORT_MAX;

    float color8_r = (float)245 / 255 * SHORT_MAX;
    float color8_g = (float)255 / 255 * SHORT_MAX;
    float color8_b = (float)163 / 255 * SHORT_MAX;

    unsigned int COLOURS_PER_PIXEL = 3;

    unsigned int spatial_x = get_global_id(0);
    unsigned int spatial_y = get_global_id(1);
    unsigned int spatial_width = get_global_size(0);

    unsigned int input_index = spatial_x * number_of_bands + spatial_y * spatial_width * number_of_bands + band_index;
    unsigned int output_index = spatial_x * COLOURS_PER_PIXEL + spatial_y * spatial_width * COLOURS_PER_PIXEL;

    float ratio = (float) input[input_index] / PIXEL_MAX;

    if (ratio < 1 * part) {
        float color_ratio = (ratio - 0 * part) / part;
        output[output_index + 0] = color1_r + color_ratio * (color2_r - color1_r);
        output[output_index + 1] = color1_g + color_ratio * (color2_g - color1_g);
        output[output_index + 2] = color1_b + color_ratio * (color2_b - color1_b);
    }
    else if (ratio < 2 * part) {
        float color_ratio = (ratio - 1 * part) / part;
        output[output_index + 0] = color2_r + color_ratio * (color3_r - color2_r);
        output[output_index + 1] = color2_g + color_ratio * (color3_g - color2_g);
        output[output_index + 2] = color2_b + color_ratio * (color3_b - color2_b);
    }
    else if (ratio < 3 * part) {
        float color_ratio = (ratio - 2 * part) / part;
        output[output_index + 0] = color3_r + color_ratio * (color4_r - color3_r);
        output[output_index + 1] = color3_g + color_ratio * (color4_g - color3_g);
        output[output_index + 2] = color3_b + color_ratio * (color4_b - color3_b);
    }
    else if (ratio < 4 * part) {
        float color_ratio = (ratio - 3 * part) / part;
        output[output_index + 0] = color4_r + color_ratio * (color5_r - color4_r);
        output[output_index + 1] = color4_g + color_ratio * (color5_g - color4_g);
        output[output_index + 2] = color4_b + color_ratio * (color5_b - color4_b);
    }
    else if (ratio < 5 * part) {
        float color_ratio = (ratio - 4 * part) / part;
        output[output_index + 0] = color5_r + color_ratio * (color6_r - color5_r);
        output[output_index + 1] = color5_g + color_ratio * (color6_g - color5_g);
        output[output_index + 2] = color5_b + color_ratio * (color6_b - color5_b);
    }
    else if (ratio < 6 * part) {
        float color_ratio = (ratio - 5 * part) / part;
        output[output_index + 0] = color6_r + color_ratio * (color7_r - color6_r);
        output[output_index + 1] = color6_g + color_ratio * (color7_g - color6_g);
        output[output_index + 2] = color6_b + color_ratio * (color7_b - color6_b);
    }
    else {
        float color_ratio = (ratio - 6 * part) / part;
        output[output_index + 0] = color7_r + color_ratio * (color8_r - color7_r);
        output[output_index + 1] = color7_g + color_ratio * (color8_g - color7_g);
        output[output_index + 2] = color7_b + color_ratio * (color8_b - color7_b);
    }
}