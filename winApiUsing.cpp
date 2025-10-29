#include "winApiUsing.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

struct Vector3 {
    float x, y, z;
};

Vector3 getScreenData() {
    DEVMODE devmode{};
    devmode.dmSize = sizeof(devmode);

    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode)) {
        return { (float)devmode.dmPelsWidth, (float)devmode.dmPelsHeight, (float)devmode.dmDisplayFrequency };
    }
    return { 0.0f, 0.0f, 0.0f };
}