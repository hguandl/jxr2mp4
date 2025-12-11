#include "decode.h"

#include <JXRGlue.h>

long PixelVectorFromFile(const char *file, uint8_t **rgba, int *width,
                         int *height) {
  ERR err = 0;
  uint8_t *buf = NULL;

  PKCodecFactory *pCodecFactory = NULL;
  PKImageDecode *pDecoder = NULL;
  PKFormatConverter *pConverter = NULL;

  Call(PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION));

  Call(pCodecFactory->CreateDecoderFromFile(file, &pDecoder));
  Call(pDecoder->GetSize(pDecoder, width, height));
  pDecoder->WMP.wmiSCP.uAlphaMode = 2;

  Call(pCodecFactory->CreateFormatConverter(&pConverter));
  Call(pConverter->Initialize(pConverter, pDecoder, ".tif",
                              GUID_PKPixelFormat64bppRGBAHalf));

  U32 stride = (*width) * sizeof(uint64_t);
  buf = (uint8_t *)malloc((size_t)stride * (*height));

  PKRect rect = {0, 0, *width, *height};
  Call(pConverter->Copy(pConverter, &rect, (U8 *)buf, stride));

Cleanup:
  if (Failed(err)) {
    free(buf);
  }

  pConverter->Release(&pConverter);
  pDecoder->Release(&pDecoder);
  pCodecFactory->Release(&pCodecFactory);

  *rgba = buf;
  return err;
}

long PixelPlanesFromFile(const char *file, uint8_t **gbrap, int *width,
                         int *height) {
  ERR err = 0;

  uint8_t *rgba = NULL;
  Call(PixelVectorFromFile(file, &rgba, width, height));

  const uint16_t *restrict src = (uint16_t *)rgba;
  const int total = (*width) * (*height);

  uint16_t *restrict dst = (uint16_t *)malloc((total * sizeof(uint64_t)));

  uint16_t *restrict pG = dst;
  uint16_t *restrict pB = pG + total;
  uint16_t *restrict pR = pB + total;
  uint16_t *restrict pA = pR + total;

  for (int i = 0; i < total; i++) {
    pG[i] = src[4 * i + 1];
    pB[i] = src[4 * i + 2];
    pR[i] = src[4 * i + 0];
    pA[i] = src[4 * i + 3];
  }

  *gbrap = (uint8_t *)dst;

Cleanup:
  free(rgba);
  return err;
}
