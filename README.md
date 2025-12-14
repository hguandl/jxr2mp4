# JXR2MP4

A command-line tool to transcode JPEG XR images to MP4 videos using FFmpeg.
HDR colors are handled with zscale filter and H.265 encoding.

## Prerequisites

- FFmpeg with zscale and an HEVC encoder (e.g., x265) support

```sh
# On Ubuntu/Debian
$ sudo apt install ffmpeg

# On macOS
$ brew install ffmpeg

# On Windows
> winget install ffmpeg
```

## Usage

```sh
JXR to MP4 transcoder

Usage: jxr2mp4 [OPTIONS] <-i <INPUT_FILE>|-d <INPUT_DIRECTORY>>

Options:
  -c <CONFIG>           Encode configuration file [default: config.toml]
  -o <OUTPUT>           Output MP4 file (with `-i`) or directory (with `-d`)
  -i <INPUT_FILE>       Single input JXR file
  -d <INPUT_DIRECTORY>  Input directory containing JXR files
  -h, --help            Print help (see more with '--help')
  -V, --version         Print version

Examples:
  jxr2mp4 -i input.jxr -o output.mp4
  jxr2mp4 -c config.toml -d input_directory -o output_directory
```

## Configurations

The encoding configuration is specified in a TOML file (default: `config.toml`).

### General Configuration

```toml
# Max retries for failed conversions
retry = 3
# Duration for the output video in seconds
seconds = 5
# Display FFmpeg logging information
verbose = true
```

### Zscale Configuration

Please refer to the [FFmpeg Zscale Filter Documentation](https://ffmpeg.org/ffmpeg-filters.html#zscale) for details.

```toml
# Output video width (0 to keep original)
out_w = 0
# Output video height (0 to keep original)
out_h = 0
# Nominal peak luminance (0 to use default)
npl = 0
```

### H.265 Codec Configuration

Please refer to the [FFmpeg H.265/HEVC Video Encoding Guide](https://trac.ffmpeg.org/wiki/Encode/H.265) for details.

```toml
# Output pixel format
pix_fmt = "yuv420p10le"
# Codec name
codec = "libx265"

# Extra options for the codec
extra_opts = [
  "-x265-params",
  "lossless=1"
]
```

Available extra options may be found by: `ffmpeg -h encoder=<codec>`.

#### Hardware Accelerations

Please refer to the [FFmpeg HWAccelIntro](https://trac.ffmpeg.org/wiki/HWAccelIntro) for details.

- Apple platform example

```toml
# Use VideoToolbox for hardware-accelerated encoding
pix_fmt = "p010le"
codec = "hevc_videotoolbox"
extra_opts = [
  "-q:v",
  "100"
]
```

- NVIDIA platform example

```toml
# Use NVENC for hardware-accelerated encoding
pix_fmt = "p010le"
codec = "hevc_nvenc"
extra_opts = [
  "-preset",
  "lossless"
]
```
