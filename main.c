#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define POPEN _popen
#define PO_MODE_WB "wb"
#define PCLOSE _pclose
#else
#include <signal.h>
#define POPEN popen
#define PO_MODE_WB "w"
#define PCLOSE pclose
#endif

#include "decode.h"

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input.jxr> <output.mp4>\n", argv[0]);
    return 1;
  }

  const char *input_file = argv[1];
  const char *output_file = argv[2];

  uint8_t *gbrap = NULL;
  int width, height;

  long ret = PixelPlanesFromFile(input_file, &gbrap, &width, &height);
  if (ret < 0) {
    fprintf(stderr, "Failed to read JXR file: %ld", ret);
    return 1;
  }

  const int frames = 2;
  const int seconds = 5;

  const size_t cmdlen = strlen(output_file) + 512L;
  char *cmd = (char *)malloc(cmdlen);
  snprintf(cmd, cmdlen,
           "ffmpeg -hide_banner"
           " -f rawvideo"
           " -pix_fmt gbrapf16le -color_trc linear"
           " -video_size %dx%d"
           " -i -"
           " -vframes %d -r %d/%d"
           " -vf zscale=%s"
           " -pix_fmt yuv444p10le"
           " -c:v libx265"
           " -x265-params lossless=1"
           " -tag:v hvc1"
           " -f mp4 -brand mp42"
           " -y %s",
           width, height, frames, frames, seconds,
           "pin=709:p=2020:min=709:m=2020_ncl:tin=linear:t=smpte2084:npl=203",
           output_file);

#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  FILE *pipe_fp = POPEN(cmd, PO_MODE_WB);
  free(cmd);

  if (!pipe_fp) {
    perror("popen failed");
    free(gbrap);
    return 1;
  }

  size_t total = (size_t)width * (size_t)height;

  for (int i = 0; i < frames; ++i) {
    size_t written = fwrite(gbrap, sizeof(uint64_t), total, pipe_fp);

    if (written < total) {
      fprintf(stderr, "Error writing frame %d. FFmpeg might have closed.\n", i);
      break;
    }
  }

  free(gbrap);

  return PCLOSE(pipe_fp);
}
