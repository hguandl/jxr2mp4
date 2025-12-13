#include "decode.h"

#include <JXRGlue.h>

long PixelVectorFromFile(const char *file, uint8_t **rgba, int *width,
                         int *height, size_t *cbytes) {
  ERR err = 0;
  uint8_t *buf = NULL;

  PKCodecFactory *pCodecFactory = NULL;
  PKImageDecode *pDecoder = NULL;

  Call(PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION));

  Call(pCodecFactory->CreateDecoderFromFile(file, &pDecoder));
  Call(pDecoder->GetSize(pDecoder, width, height));
  pDecoder->WMP.wmiSCP.uAlphaMode = 2;

  PKPixelFormatGUID format;
  Call(pDecoder->GetPixelFormat(pDecoder, &format));

  if (IsEqualGUID(&format, &GUID_PKPixelFormat64bppRGBAHalf)) {
    *cbytes = sizeof(uint16_t);
  } else if (IsEqualGUID(&format, &GUID_PKPixelFormat128bppRGBAFloat)) {
    *cbytes = sizeof(uint32_t);
  } else {
    fprintf(stderr,
            "Unsupported Pixel format GUID"
            " {%08x-%04x-%04x-%02x%02x%2x%2x%2x%2x%2x%2x}\n",
            format.Data1, format.Data2, format.Data3, format.Data4[0],
            format.Data4[1], format.Data4[2], format.Data4[3], format.Data4[4],
            format.Data4[5], format.Data4[6], format.Data4[7]);
    err = -1;
    goto Cleanup;
  }

  U32 stride = (*width) * (U32)(*cbytes) * 4;
  buf = (uint8_t *)malloc((size_t)stride * (*height));

  PKRect rect = {0, 0, *width, *height};
  Call(pDecoder->Copy(pDecoder, &rect, (U8 *)buf, stride));

Cleanup:
  if (Failed(err)) {
    free(buf);
    buf = NULL;
  }

  pDecoder->Release(&pDecoder);
  pCodecFactory->Release(&pCodecFactory);

  *rgba = buf;
  return err;
}

#define PACKED_TO_PLANAR_LOOP(T, src, dst)                                     \
  const T *restrict __src_t = (const T *)src;                                  \
                                                                               \
  T *restrict __pG = (T *)dst;                                                 \
  T *restrict __pB = __pG + total;                                             \
  T *restrict __pR = __pB + total;                                             \
  T *restrict __pA = __pR + total;                                             \
                                                                               \
  for (int i = 0; i < total; i++) {                                            \
    __pG[i] = __src_t[4 * i + 1];                                              \
    __pB[i] = __src_t[4 * i + 2];                                              \
    __pR[i] = __src_t[4 * i + 0];                                              \
    __pA[i] = __src_t[4 * i + 3];                                              \
  }

long PixelPlanesFromFile(const char *file, uint8_t **gbrap, int *width,
                         int *height, size_t *cbytes) {
  ERR err = 0;
  uint8_t *dst = NULL;
  uint8_t *rgba = NULL;

  Call(PixelVectorFromFile(file, &rgba, width, height, cbytes));

  const int total = (*width) * (*height);
  dst = (uint8_t *)malloc((total * (*cbytes) * 4));

  if (*cbytes == sizeof(uint16_t)) {
    PACKED_TO_PLANAR_LOOP(uint16_t, rgba, dst)
  } else if (*cbytes == sizeof(uint32_t)) {
    PACKED_TO_PLANAR_LOOP(uint32_t, rgba, dst)
  } else {
    err = -1;
    goto Cleanup;
  }

Cleanup:
  if (Failed(err)) {
    free(dst);
    dst = NULL;
  }

  free(rgba);

  *gbrap = (uint8_t *)dst;
  return err;
}
