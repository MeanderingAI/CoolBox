fn main() {
    cxx_build::bridge("src/lib.rs")
        .file("src/bridge.cpp")
        .file("../packages/ML/generalized_linear_model/source/generalized_linear_model.cpp")
        .file("../packages/ML/generalized_linear_model/source/linear_regression.cpp")
        .include("include")
        .include("../packages/ML/generalized_linear_model/headers")
        .std("c++17")
        .compile("coolbox_rust_bindings");

    println!("cargo:rustc-link-search=native=../c_bindings/build");
    println!("cargo:rustc-link-search=native=../c_bindings/build/Release");
    println!("cargo:rustc-link-lib=coolbox_c_bindings");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge.cpp");
    println!("cargo:rerun-if-changed=include/bridge.h");
    println!("cargo:rerun-if-changed=../c_bindings/include/coolbox/coolbox_c.h");
    println!("cargo:rerun-if-changed=../packages/ML/generalized_linear_model/headers/generalized_linear_model.h");
    println!("cargo:rerun-if-changed=../packages/ML/generalized_linear_model/headers/linear_regression.h");
    println!("cargo:rerun-if-changed=../packages/ML/generalized_linear_model/source/generalized_linear_model.cpp");
    println!("cargo:rerun-if-changed=../packages/ML/generalized_linear_model/source/linear_regression.cpp");
}
