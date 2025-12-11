#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

long PixelVectorFromFile(const char *file, uint8_t **rgba, int *width,
                         int *height);

long PixelPlanesFromFile(const char *file, uint8_t **gbrap, int *width,
                         int *height);

#ifdef __cplusplus
}
#endif
