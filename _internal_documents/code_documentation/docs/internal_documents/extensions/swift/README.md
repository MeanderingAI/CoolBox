# Swift Extension

## Location

- `_deliverables/libraries/bindings/swift_bindings`

## Surface

- Swift package target: `CoolBoxSwiftBindings`
- Exported API: `CoolBoxSwiftBindings.version`, `CoolBoxSwiftBindings.ping()`

## Setup And Build

```bash
cd _deliverables/libraries/bindings/swift_bindings
swift build -c release
```

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows can publish `swift-extension-*` artifacts.
