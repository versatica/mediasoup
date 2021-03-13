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

    println!("cargo:rustc-link-lib=static=stdc++");
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
    // TODO: Fix: this is just for my machine
    println!("cargo:rustc-link-search=native=/usr/lib/gcc/x86_64-linux-gnu/10");
}
