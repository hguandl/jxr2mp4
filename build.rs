fn main() {
    println!("cargo:rerun-if-changed=src/decode.c");

    let proj_dir = std::env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR not set");

    let sub_dirs = [
        "jxrlib/image/decode",
        "jxrlib/image/encode",
        "jxrlib/image/sys",
        "jxrlib/jxrgluelib",
    ];

    let mut files = Vec::new();

    for sub_dir in sub_dirs {
        let dir_path = std::path::Path::new(&proj_dir).join(sub_dir);
        for entry in std::fs::read_dir(dir_path).expect("Failed to read directory") {
            let entry = entry.expect("Failed to get directory entry");
            let path = entry.path();
            if let Some(ext) = path.extension()
                && ext == "c"
            {
                files.push(path);
            }
        }
    }

    let mut build = cc::Build::new();
    build
        .file("src/decode.c")
        .define("DISABLE_PERF_MEASUREMENT", None)
        .flag("-w")
        .include("jxrlib/common/include")
        .include("jxrlib/image/sys")
        .include("jxrlib/jxrgluelib")
        .files(&files);

    if cfg!(target_endian = "big") {
        build.define("_BIG__ENDIAN_", None);
    }

    if !cfg!(target_env = "msvc") {
        build.define("__ANSI__", None);
        build.flag("-Wno-error=implicit-function-declaration");
        println!("cargo:rustc-link-lib=m");
    }

    build.compile("decode");
}
