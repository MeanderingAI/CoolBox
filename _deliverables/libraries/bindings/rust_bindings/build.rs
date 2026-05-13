use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR is set by Cargo"));
    let manifest_dir = fs::canonicalize(&manifest_dir).unwrap_or(manifest_dir);

    let c_bindings_build_dir = env::var("COOLBOX_C_BINDINGS_BUILD_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| manifest_dir.join("../c_bindings/build"));
    let c_bindings_build_dir = fs::canonicalize(&c_bindings_build_dir).unwrap_or(c_bindings_build_dir);

    let c_bindings_release_dir = env::var("COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| c_bindings_build_dir.join("Release"));
    let c_bindings_release_dir = fs::canonicalize(&c_bindings_release_dir).unwrap_or(c_bindings_release_dir);

    cxx_build::bridge("src/lib.rs")
        .file("src/bridge.cpp")
        .file("../../groups/cool_car/ML/generalized_linear_model/source/generalized_linear_model.cpp")
        .file("../../groups/cool_car/ML/generalized_linear_model/source/linear_regression.cpp")
        .include("include")
        .include("../../groups/cool_car/ML/generalized_linear_model/headers")
        .std("c++17")
        .compile("coolbox_rust_bindings");

    println!("cargo:rustc-link-search=native={}", c_bindings_build_dir.display());
    println!("cargo:rustc-link-search=native={}", c_bindings_release_dir.display());
    println!("cargo:rustc-link-lib=coolbox_c_bindings");
    println!("cargo:rerun-if-env-changed=CARGO_MANIFEST_DIR");
    println!("cargo:rerun-if-env-changed=COOLBOX_C_BINDINGS_BUILD_DIR");
    println!("cargo:rerun-if-env-changed=COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=include/bridge.h");
    println!("cargo:rerun-if-changed=../c_bindings/include/coolbox/coolbox_c.h");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/headers/generalized_linear_model.h");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/headers/linear_regression.h");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/source/generalized_linear_model.cpp");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/source/linear_regression.cpp");
}
