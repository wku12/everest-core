use everestrs_build::Builder;


pub fn main() {
    Builder::new("manifest.yaml", vec!["../../.."])
        .generate()
        .unwrap();

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=manifest.yaml");
}