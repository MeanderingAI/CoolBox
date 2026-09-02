// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "CoolBoxSwiftBindings",
    products: [
        .library(name: "CoolBoxSwiftBindings", targets: ["CoolBoxSwiftBindings"]),
    ],
    targets: [
        .target(name: "CoolBoxSwiftBindings"),
    ]
)
