use std::env;
use std::process::Command;

fn main() {
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
    println!(
        "cargo:rustc-link-search=native={}/out/Release",
        std::env::current_dir()
            .unwrap()
            .into_os_string()
            .into_string()
            .unwrap()
    );
}
