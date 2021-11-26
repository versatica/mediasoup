use std::env;
use std::process::Command;

fn main() {
    if std::env::var("DOCS_RS").is_ok() {
        // Skip everything when building docs on docs.rs
        return;
    }

    let build_type = if cfg!(debug_assertions) {
        "Debug"
    } else {
        "Release"
    };

    let out_dir = env::var("OUT_DIR").unwrap();

    // Add C++ std lib
    #[cfg(target_os = "linux")]
    {
        let path = Command::new(env::var("CXX").unwrap_or_else(|_| "c++".to_string()))
            .arg("--print-file-name=libstdc++.a")
            .output()
            .expect("Failed to start")
            .stdout;
        println!(
            "cargo:rustc-link-search=native={}",
            String::from_utf8_lossy(&path)
                .trim()
                .strip_suffix("libstdc++.a")
                .expect("Failed to strip suffix"),
        );
        println!("cargo:rustc-link-lib=static=stdc++");
    }
    #[cfg(any(
        target_os = "freebsd",
        target_os = "dragonfly",
        target_os = "openbsd",
        target_os = "netbsd"
    ))]
    {
        let path = Command::new(env::var("CXX").unwrap_or_else(|_| "c++".to_string()))
            .arg("--print-file-name=libc++.a")
            .output()
            .expect("Failed to start")
            .stdout;
        println!(
            "cargo:rustc-link-search=native={}",
            String::from_utf8_lossy(&path)
                .trim()
                .strip_suffix("libc++.a")
                .expect("Failed to strip suffix"),
        );
        println!("cargo:rustc-link-lib=static=c++");
    }
    #[cfg(target_os = "macos")]
    {
        let path = Command::new("xcrun")
            .args(&["--show-sdk-path"])
            .output()
            .expect("Failed to start")
            .stdout;

        let libpath = format!(
            "{}/usr/lib",
            String::from_utf8(path)
                .expect("Failed to decode path")
                .trim()
        );
        println!("cargo:rustc-link-search={}", libpath);
        println!("cargo:rustc-link-lib=dylib=c++");
        println!("cargo:rustc-link-lib=dylib=c++abi");
    }
    #[cfg(target_os = "windows")]
    {
        panic!("Building on Windows is not currently supported");
        // TODO: Didn't bother, feel free to PR
    }

    // The build here is a bit awkward since we can't just specify custom target directory as
    // openssl will fail to build with `make[1]: /bin/sh: Argument list too long` due to large
    // number of files. So instead we build in place, copy files to out directory and then clean
    // after ourselves
    {
        // Build
        if !Command::new("make")
            .arg("libmediasoup-worker")
            .env("MEDIASOUP_OUT_DIR", &out_dir)
            .env("MEDIASOUP_BUILDTYPE", &build_type)
            .spawn()
            .expect("Failed to start")
            .wait()
            .expect("Wasn't running")
            .success()
        {
            panic!("Failed to build libmediasoup-worker")
        }

        if env::var("KEEP_BUILD_ARTIFACTS") != Ok("1".to_string()) {
            // Clean
            if !Command::new("make")
                .arg("clean-all")
                .spawn()
                .expect("Failed to start")
                .wait()
                .expect("Wasn't running")
                .success()
            {
                panic!("Failed to clean libmediasoup-worker")
            }
        }
    }

    println!("cargo:rustc-link-lib=static=mediasoup-worker");
    println!("cargo:rustc-link-search=native={}/{}", out_dir, build_type);
}
