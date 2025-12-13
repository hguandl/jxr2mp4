#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

long PixelVectorFromFile(const char *file, uint8_t **rgba, int *width,
                         int *height, size_t *cbytes);

long PixelPlanesFromFile(const char *file, uint8_t **gbrap, int *width,
                         int *height, size_t *cbytes);

#ifdef __cplusplus
}
#endif
