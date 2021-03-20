use std::env;
use std::process::Command;

fn main() {
    let current_dir = std::env::current_dir()
        .unwrap()
        .into_os_string()
        .into_string()
        .unwrap();
    let out_dir = env::var("OUT_DIR").unwrap();

    // The build here is a bit awkward since we can't just specify custom target directory as
    // openssl will fail to build with `make[1]: /bin/sh: Argument list too long` due to large
    // number of files. So instead we build in place, copy files to out directory and then clean
    // after ourselves
    {
        // Build
        if !Command::new("make")
            .arg("libmediasoup-worker")
            .spawn()
            .expect("Failed to start")
            .wait()
            .expect("Wasn't running")
            .success()
        {
            panic!("Failed to build libmediasoup-worker")
        }

        for file in &[
            "libnetstring.a",
            "libuv.a",
            "libopenssl.a",
            "libsrtp.a",
            "libusrsctp.a",
            "libwebrtc.a",
            "libmediasoup-worker.a",
            "libabseil.a",
            #[cfg(windows)]
            "libgetopt.a",
        ] {
            std::fs::copy(
                format!("{}/out/Release/{}", current_dir, file),
                format!("{}/{}", out_dir, file),
            )
            .expect(&format!(
                "Failed to copy static library from {}/out/Release/{} to {}/{}",
                current_dir, file, out_dir, file
            ));
        }

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
    // Add C++ std lib
    #[cfg(target_os = "linux")]
    {
        println!("cargo:rustc-link-lib=static=stdc++");

        let output = Command::new(env::var("c++").unwrap_or("c++".to_string()))
            .arg("-print-search-dirs")
            .output()
            .expect("Failed to start");
        for line in String::from_utf8_lossy(&output.stdout).lines() {
            if let Some(paths) = line.strip_prefix("libraries: =") {
                for path in paths.split(':') {
                    println!("cargo:rustc-link-search=native={}", path);
                }
            }
        }
    }
    #[cfg(any(
        target_os = "macos",
        target_os = "freebsd",
        target_os = "dragonfly",
        target_os = "openbsd",
        target_os = "netbsd"
    ))]
    {
        println!("cargo:rustc-link-lib=static=c++");

        let output = Command::new(env::var("c++").unwrap_or("c++".to_string()))
            .arg("-print-search-dirs")
            .output()
            .expect("Failed to start");
        for line in String::from_utf8_lossy(&output.stdout).lines() {
            if let Some(paths) = line.strip_prefix("libraries: =") {
                for path in paths.split(':') {
                    println!("cargo:rustc-link-search=native={}", path);
                }
            }
        }
    }
    // TODO: Windows and check above BSDs if they even work

    println!("cargo:rustc-link-lib=static=netstring");
    println!("cargo:rustc-link-lib=static=uv");
    println!("cargo:rustc-link-lib=static=openssl");
    println!("cargo:rustc-link-lib=static=srtp");
    println!("cargo:rustc-link-lib=static=usrsctp");
    println!("cargo:rustc-link-lib=static=webrtc");
    println!("cargo:rustc-link-lib=static=mediasoup-worker");
    println!("cargo:rustc-link-lib=static=abseil");
    #[cfg(windows)]
    println!("cargo:rustc-link-lib=static=getopt");
    println!("cargo:rustc-link-search=native={}", out_dir);
}
