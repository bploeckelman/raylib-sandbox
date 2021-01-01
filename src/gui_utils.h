#ifndef RAYLIB_SANDBOX_GUI_UTILS_H
#define RAYLIB_SANDBOX_GUI_UTILS_H

typedef struct ValueSlider ValueSlider;

struct ValueSlider {
    Rectangle bounds;
    const char *textLeft;
    const char *textRight;
    float value;
    float minValue;
    float maxValue;
};

#endif //RAYLIB_SANDBOX_GUI_UTILS_H
