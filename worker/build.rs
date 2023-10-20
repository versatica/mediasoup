use std::env;
use std::process::Command;

fn main() {
    if env::var("DOCS_RS").is_ok() {
        // Skip everything when building docs on docs.rs
        return;
    }

    // On Windows Rust always links against release version of MSVC runtime, thus requires Release
    // build here.
    let build_type = if cfg!(all(debug_assertions, not(windows))) {
        "Debug"
    } else {
        "Release"
    };

    let out_dir = env::var("OUT_DIR").unwrap();
    // Force forward slashes on Windows too so that is plays well with our dumb `Makefile`.
    let mediasoup_out_dir = format!("{}/out", out_dir.replace('\\', "/"));

    // Store original PATH so we make `make clean-all` use it. This is because, in Windows,
    // we may need to fetch `make` and we store it in out/msys and then we add out/msys/bin
    // to the PATH, and that folder may contain rm.exe, so `make clean-all` would use
    // that rm.exe and delete itself and make the task fail.
    let original_path = env::var("PATH").unwrap();

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
            .arg("--show-sdk-path")
            .output()
            .expect("Failed to start")
            .stdout;

        let libpath = format!(
            "{}/usr/lib",
            String::from_utf8(path)
                .expect("Failed to decode path")
                .trim()
        );
        println!("cargo:rustc-link-search={libpath}");
        println!("cargo:rustc-link-lib=dylib=c++");
        println!("cargo:rustc-link-lib=dylib=c++abi");
    }
    #[cfg(target_os = "windows")]
    {
        if !std::path::Path::new("worker/out/msys/bin/make.exe").exists() {
            let python = if let Ok(python) = env::var("PYTHON") {
                python
            } else if Command::new("where")
                .arg("python3")
                .status()
                .expect("Failed to start")
                .success()
            {
                "python3".to_string()
            } else {
                "python".to_string()
            };

            let dir = format!("{}/msys", mediasoup_out_dir.replace('\\', "/"));

            if !Command::new(python)
                .arg("scripts\\getmake.py")
                .arg("--dir")
                .arg(dir.clone())
                .status()
                .expect("Failed to start")
                .success()
            {
                panic!("Failed to install MSYS/make")
            }

            env::set_var(
                "PATH",
                format!("{}\\bin;{}", dir, env::var("PATH").unwrap()),
            );
        }

        env::set_var(
            "PATH",
            format!(
                "{}\\worker\\out\\msys\\bin;{}",
                env::current_dir()
                    .unwrap()
                    .into_os_string()
                    .into_string()
                    .unwrap(),
                env::var("PATH").unwrap()
            ),
        );
    }

    // Build
    if !Command::new("make")
        .arg("libmediasoup-worker")
        .env("MEDIASOUP_OUT_DIR", &mediasoup_out_dir)
        .env("MEDIASOUP_BUILDTYPE", build_type)
        // Force forward slashes on Windows too, otherwise Meson thinks path is not absolute 🤷
        .env("INSTALL_DIR", &out_dir.replace('\\', "/"))
        .spawn()
        .expect("Failed to start")
        .wait()
        .expect("Wasn't running")
        .success()
    {
        panic!("Failed to build libmediasoup-worker")
    }

    #[cfg(target_os = "windows")]
    {
        let dot_a = format!("{out_dir}/libmediasoup-worker.a");
        let dot_lib = format!("{out_dir}/mediasoup-worker.lib");

        // Meson builds `libmediasoup-worker.a` on Windows instead of `*.lib` file under MinGW
        if std::path::Path::new(&dot_a).exists() {
            std::fs::copy(&dot_a, &dot_lib).unwrap_or_else(|error| {
                panic!("Failed to copy static library from {dot_a} to {dot_lib}: {error}");
            });
        }

        // These are required by libuv on Windows
        println!("cargo:rustc-link-lib=psapi");
        println!("cargo:rustc-link-lib=user32");
        println!("cargo:rustc-link-lib=advapi32");
        println!("cargo:rustc-link-lib=iphlpapi");
        println!("cargo:rustc-link-lib=userenv");
        println!("cargo:rustc-link-lib=ws2_32");

        // These are required by OpenSSL on Windows
        println!("cargo:rustc-link-lib=ws2_32");
        println!("cargo:rustc-link-lib=gdi32");
        println!("cargo:rustc-link-lib=advapi32");
        println!("cargo:rustc-link-lib=crypt32");
        println!("cargo:rustc-link-lib=user32");
    }

    if env::var("KEEP_BUILD_ARTIFACTS") != Ok("1".to_string()) {
        // Clean
        if !Command::new("make")
            .arg("clean-all")
            .env("PATH", original_path)
            .env("MEDIASOUP_OUT_DIR", &mediasoup_out_dir)
            .spawn()
            .expect("Failed to start")
            .wait()
            .expect("Wasn't running")
            .success()
        {
            panic!("Failed to clean libmediasoup-worker")
        }
    }

    println!("cargo:rustc-link-lib=static=mediasoup-worker");
    println!("cargo:rustc-link-search=native={out_dir}");
}
