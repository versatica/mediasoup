use std::ffi::CString;
use std::os::raw::{c_char, c_int};

#[link(name = "mediasoup-worker", kind = "static")]
extern "C" {
    fn run(argc: c_int, argv: *const *const c_char) -> c_int;
}

fn main() {
    let args = std::env::args();
    let argc = args.len() as c_int;
    let args_cstring = args
        .into_iter()
        .map(|s| -> CString { CString::new(s).unwrap() });
    let argv = args_cstring
        .map(|arg| arg.as_ptr() as *const c_char)
        .collect::<Vec<_>>();
    let status_code = unsafe { run(argc, argv.as_ptr()) };

    std::process::exit(status_code);
}
