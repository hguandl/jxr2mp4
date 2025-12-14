use std::io::Write;
use std::ops::Deref;
use std::path::Path;

use anyhow::{Context, Result, bail};
use serde::Deserialize;
use subprocess::{Exec, Redirection};

unsafe extern "C" {
    fn PixelPlanesFromFile(
        file: *const i8,
        gbrap: *mut *mut u8,
        width: *mut i32,
        height: *mut i32,
        cbytes: *mut usize,
    ) -> i64;

    fn libc_free(ptr: *mut u8);
}

#[derive(Debug)]
pub struct PixelPlanes {
    pub width: i32,
    pub height: i32,
    data: *mut u8,
    cbytes: usize,
}

impl Deref for PixelPlanes {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        let len = self.cbytes * (self.width * self.height * 4) as usize;
        unsafe { std::slice::from_raw_parts(self.data, len) }
    }
}

impl Drop for PixelPlanes {
    fn drop(&mut self) {
        unsafe { libc_free(self.data) }
    }
}

impl PixelPlanes {
    pub fn from_file<P: AsRef<Path>>(file: P) -> Result<Self> {
        let str = file.as_ref().as_os_str().as_encoded_bytes();
        let c_file = std::ffi::CString::new(str).unwrap();
        let mut data = std::ptr::null_mut();
        let mut width = 0;
        let mut height = 0;
        let mut cbytes = 0;

        let result = unsafe {
            PixelPlanesFromFile(
                c_file.as_ptr(),
                &mut data,
                &mut width,
                &mut height,
                &mut cbytes,
            )
        };

        if result != 0 {
            bail!("Failed to read pixel planes from file ({result})");
        }

        Ok(PixelPlanes {
            width,
            height,
            cbytes,
            data,
        })
    }
}

#[derive(Debug, Deserialize)]
pub struct EncodeConfig {
    pub retry: usize,
    pub seconds: i32,
    pub verbose: bool,
    pub out_w: i32,
    pub out_h: i32,
    pub npl: i32,
    pub pix_fmt: String,
    pub codec: String,
    pub extra_opts: Vec<String>,
}

impl PixelPlanes {
    pub fn encode<P: AsRef<Path>>(&self, config: &EncodeConfig, file: P) -> Result<()> {
        let pix_fmt = match self.cbytes {
            2 => "gbrapf16le",
            4 => "gbrapf32le",
            _ => bail!("Unsupported cbytes: {}", self.cbytes),
        };

        const FRAMES: i32 = 2;

        let zscale = {
            let mut zscale = "pin=709:p=2020:min=709:m=2020_ncl:tin=linear:t=smpte2084".to_string();
            if config.npl > 0 {
                zscale.push_str(&format!(":npl={}", config.npl));
            }
            if config.out_w > 0 && config.out_h > 0 {
                zscale.push_str(&format!(":w={}:h={}", config.out_w, config.out_h));
            }
            zscale
        };

        let args = [
            "-hide_banner",
            "-f",
            "rawvideo",
            "-pix_fmt",
            pix_fmt,
            "-color_trc",
            "linear",
            "-video_size",
            &format!("{}x{}", self.width, self.height),
            "-i",
            "-",
            "-vframes",
            &FRAMES.to_string(),
            "-r",
            &format!("{}/{}", FRAMES, config.seconds),
            "-vf",
            &format!("zscale={}", zscale),
            "-pix_fmt",
            &config.pix_fmt,
            "-c:v",
            &config.codec,
        ];

        let mut args = Vec::from(args);
        args.extend(config.extra_opts.iter().map(|s| s.as_str()));
        args.extend_from_slice(&["-tag:v", "hvc1", "-f", "mp4", "-brand", "mp42", "-y"]);

        let exec = Exec::cmd("ffmpeg")
            .args(&args)
            .arg(file.as_ref().as_os_str())
            .stdin(Redirection::Pipe);
        let exec = if config.verbose {
            exec
        } else {
            exec.stderr(subprocess::NullFile)
        };
        let mut p = exec.popen().context("Failed to start FFmpeg")?;

        for _ in 0..FRAMES {
            p.stdin.as_ref().unwrap().write_all(self).unwrap();
        }
        drop(p.stdin.take());

        let status = p.wait().context("Failed to wait for FFmpeg to exit")?;
        if !status.success() {
            bail!("FFmpeg exited with status: {status:?}");
        }

        Ok(())
    }
}
