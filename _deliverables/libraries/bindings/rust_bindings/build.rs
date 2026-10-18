use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let manifest_dir =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR is set by Cargo"));
    let manifest_dir = fs::canonicalize(&manifest_dir).unwrap_or(manifest_dir);

    let c_bindings_build_dir = env::var("COOLBOX_C_BINDINGS_BUILD_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| manifest_dir.join("../c_bindings/build"));
    let c_bindings_build_dir =
        fs::canonicalize(&c_bindings_build_dir).unwrap_or(c_bindings_build_dir);

    let c_bindings_release_dir = env::var("COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR")
        .map(PathBuf::from)
        .unwrap_or_else(|_| c_bindings_build_dir.join("Release"));
    let c_bindings_release_dir =
        fs::canonicalize(&c_bindings_release_dir).unwrap_or(c_bindings_release_dir);

    let mut build = cxx_build::bridge("src/lib.rs");
    build
        .file("src/bridge.cpp")
        .file("../go_bindings/bandit_bridge.cc")
        .file("../go_bindings/bandit_bridge_sources.cc")
        .file("../go_bindings/dl_bridge.cc")
        .file("../go_bindings/dl_bridge_sources.cc")
        .file("../go_bindings/ds_bridge.cc")
        .file("../go_bindings/ds_bridge_sources.cc")
        .file("../go_bindings/forecast_bridge.cc")
        .file("../go_bindings/graphics_bridge.cc")
        .file("../go_bindings/nlp_bridge.cc")
        .file("../go_bindings/nlp_bridge_sources.cc")
        .file("../go_bindings/series_bridge.cc")
        .file("../go_bindings/series_bridge_sources.cc")
        .file("../go_bindings/tree_bridge.cc")
        .file("../go_bindings/tree_bridge_sources.cc")
        .file(
            "../../groups/cool_car/ML/generalized_linear_model/source/generalized_linear_model.cpp",
        )
        .file("../../groups/cool_car/ML/generalized_linear_model/source/linear_regression.cpp")
        .include("include")
        .include("../go_bindings")
        .include("../python_bindings/include")
        .include("../python_bindings/include/decision_tree")
        .include("../python_bindings/include/nlp")
        .include("../python_bindings/include/time_series")
        .include("../python_bindings/include/multi_arm_bandit")
        .include("../python_bindings/vendor_include")
        .include("../python_bindings/vendor_include/GRAPHICS/charts/headers")
        .include("../../groups/cool_car")
        .include("../../groups/cool_car/DL/layers/include")
        .include("../../groups/cool_car/DL/loss/include")
        .include("../../groups/cool_car/DL/optimizer/include")
        .include("../../groups/cool_car/DL/wrapper/include")
        .include("../../groups/cool_car/DL/agents/include")
        .include("../../groups/cool_car/MATRIX/headers")
        .include("../../groups/cool_car/ML/decision_tree/headers")
        .include("../../groups/cool_car/ML/generalized_linear_model/headers")
        .include("../../groups/app_builder/GRAPHICS/charts/headers")
        .include("../../groups/trekker/DATASTRUCTURE/trees/headers")
        .include("../../groups/trekker/DATASTRUCTURE/maps/headers")
        .include("../../groups/trekker/DATASTRUCTURE/list/headers")
        .include("../../groups/trekker/DATASTRUCTURE/filters/headers")
        .include("../../groups/trekker/DATASTRUCTURE/xarray/headers")
        .std("c++17");
    build.compile("coolbox_rust_bindings");

    println!(
        "cargo:rustc-link-search=native={}",
        c_bindings_build_dir.display()
    );
    println!(
        "cargo:rustc-link-search=native={}",
        c_bindings_release_dir.display()
    );
    println!("cargo:rustc-link-lib=coolbox_c_bindings");
    println!("cargo:rerun-if-env-changed=CARGO_MANIFEST_DIR");
    println!("cargo:rerun-if-env-changed=COOLBOX_C_BINDINGS_BUILD_DIR");
    println!("cargo:rerun-if-env-changed=COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=include/bridge.h");
    for source in [
        "../go_bindings/bandit_bridge.cc",
        "../go_bindings/bandit_bridge_sources.cc",
        "../go_bindings/dl_bridge.cc",
        "../go_bindings/dl_bridge_sources.cc",
        "../go_bindings/ds_bridge.cc",
        "../go_bindings/ds_bridge_sources.cc",
        "../go_bindings/forecast_bridge.cc",
        "../go_bindings/graphics_bridge.cc",
        "../go_bindings/nlp_bridge.cc",
        "../go_bindings/nlp_bridge_sources.cc",
        "../go_bindings/series_bridge.cc",
        "../go_bindings/series_bridge_sources.cc",
        "../go_bindings/tree_bridge.cc",
        "../go_bindings/tree_bridge_sources.cc",
    ] {
        println!("cargo:rerun-if-changed={source}");
    }
    println!("cargo:rerun-if-changed=../c_bindings/include/coolbox/coolbox_c.h");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/headers/generalized_linear_model.h");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/headers/linear_regression.h");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/source/generalized_linear_model.cpp");
    println!("cargo:rerun-if-changed=../../../groups/cool_car/ML/generalized_linear_model/source/linear_regression.cpp");
}
