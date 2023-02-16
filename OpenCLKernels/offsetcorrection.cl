kernel void OffsetCorrection(
    global const unsigned short* input,
    global unsigned short* output,
    int offset_x,
    int offset_y,
    int active_area_width,
    int active_area_height)
{
    int sensor_width = get_global_size(0);
    int x = get_global_id(0);
    int y = get_global_id(1);

    int end_x = offset_x + active_area_width;
    int end_y = offset_y + active_area_height;

    if (x >= offset_x && x < end_x && y >= offset_y && y < end_y) {
        int i = (x - offset_x) + active_area_width * (y - offset_y);
        output[i] = input[x + sensor_width * y];
    }
}