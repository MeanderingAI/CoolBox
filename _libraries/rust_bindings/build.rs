fn main() {
    cxx_build::bridge("src/lib.rs")
        .file("src/bridge.cpp")
        .file("../backages/ML/generalized_linear_model/source/generalized_linear_model.cpp")
        .file("../backages/ML/generalized_linear_model/source/linear_regression.cpp")
        .include("include")
        .include("../backages/ML/generalized_linear_model/headers")
        .std("c++17")
        .compile("coolbox_rust_bindings");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=include/bridge.h");
    println!("cargo:rerun-if-changed=../backages/ML/generalized_linear_model/headers/generalized_linear_model.h");
    println!("cargo:rerun-if-changed=../backages/ML/generalized_linear_model/headers/linear_regression.h");
    println!("cargo:rerun-if-changed=../backages/ML/generalized_linear_model/source/generalized_linear_model.cpp");
    println!("cargo:rerun-if-changed=../backages/ML/generalized_linear_model/source/linear_regression.cpp");
}
