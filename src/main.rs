mod image;

use std::path::{Path, PathBuf};

use anyhow::{Context, Result};
use clap::{Args, Parser};

use image::{EncodeConfig, PixelPlanes};

const CLI_EXAMPLES: &str = r#"Examples:
  jxr2mp4 -i input.jxr -o output.mp4
  jxr2mp4 -c config.toml -d input_directory -o output_directory
"#;

/// JXR to MP4 transcoder
///
/// Convert HDR colors from linear to PQ.
#[derive(Debug, Parser)]
#[command(version, after_help = CLI_EXAMPLES)]
struct Cli {
    /// Encode configuration file
    #[arg(short, default_value = "config.toml")]
    config: PathBuf,

    /// Output MP4 file (with `-i`) or directory (with `-d`)
    #[arg(short)]
    output: Option<PathBuf>,

    #[clap(flatten)]
    input: CliInput,
}

#[derive(Debug, Args)]
#[group(required = true, multiple = false)]
struct CliInput {
    /// Single input JXR file
    #[arg(short = 'i')]
    input_file: Option<PathBuf>,

    /// Input directory containing JXR files
    #[arg(short = 'd')]
    input_directory: Option<PathBuf>,
}

fn main() -> Result<()> {
    let args = Cli::parse();

    let config = std::fs::read_to_string(&args.config)?;
    let config = toml::from_str::<EncodeConfig>(&config)?;

    match (args.input.input_file, args.input.input_directory) {
        (Some(file), None) => {
            let output = args.output.unwrap_or_else(|| file.with_extension("mp4"));
            transcode(&config, &file, &output);
        }
        (None, Some(directory)) => {
            let output_dir = args.output.as_deref().unwrap_or(directory.as_path());
            for entry in std::fs::read_dir(&directory)? {
                let entry = entry?;
                let path = entry.path();
                if path.extension().and_then(|s| s.to_str()) != Some("jxr") {
                    continue;
                }
                let stem = match path.file_stem() {
                    Some(stem) => stem,
                    None => continue,
                };
                println!("----------------------------------------");
                let output = output_dir.join(stem).with_added_extension("mp4");
                transcode(&config, &path, &output);
            }
        }
        _ => unreachable!("Either input_file or input_directory must be provided"),
    }

    Ok(())
}

fn transcode<P: AsRef<Path>>(config: &EncodeConfig, input: P, output: P) {
    println!("Transcoding {:?}", input.as_ref());

    let mut attempts = 0;
    let mut image = Option::None;
    let mut result = anyhow::Ok(());

    let mut process = || {
        let image = match &image {
            Some(planes) => planes,
            None => {
                let p = PixelPlanes::from_file(&input).context("Failed to decode pixel planes")?;
                image = Some(p);
                image.as_ref().unwrap()
            }
        };
        image
            .encode(config, &output)
            .context("Failed to encode pixel planes")?;
        anyhow::Ok(())
    };

    while attempts < config.retry {
        if attempts > 0 {
            println!("Retrying... ({}/{})", attempts + 1, config.retry);
        }
        result = process();
        if result.is_ok() {
            break;
        }
        attempts += 1;
    }

    match result {
        Ok(_) => println!("Successfully transcoded to {:?}", output.as_ref()),
        Err(e) => println!("Failed to transcode: {:?}", e),
    }
}
