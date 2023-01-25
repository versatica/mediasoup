use flatc_rust;
use std::fs;
use std::path::Path;

/// NOTE: flatc binary path must be present in PATH environment variable.
fn main() {
    println!("cargo:rerun-if-changed=../fbs/");

    // Retrieve a Vec<&Path> with the schema files.
    let schemas_path = fs::read_dir("../fbs").unwrap();
    let mut inputs_str: Vec<String> = vec![];
    let mut inputs_path: Vec<&Path> = vec![];
    let extra = ["--gen-all", "--filename-suffix", "", "--rust-serialize"];

    for schema in schemas_path {
        inputs_str.push(schema.unwrap().path().to_string_lossy().to_string());
    }

    for (index, _) in inputs_str.iter().enumerate() {
        inputs_path.push(Path::new(&inputs_str[index]));
    }

    // Generate flatbuffers code.
    flatc_rust::run(flatc_rust::Args {
        inputs: &inputs_path,
        out_dir: Path::new("./src/fbs/"),
        extra: &extra,
        ..Default::default()
    }).expect("flatc");
}
