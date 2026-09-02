# CoolBox Xamarin Bindings

This module provides Xamarin-friendly bindings for CoolBox using a native C ABI bridge plus a .NET P/Invoke surface.

## Included surfaces

- Metadata and capability access from the core C bindings
- Liquid UI animated bounds helpers for semi-transparent water-like element motion

## Native build

```bash
cmake -S . -B build
cmake --build build --target coolbox_xamarin_bindings
```

## .NET/Xamarin build

```bash
dotnet build _deliverables/libraries/bindings/xamarin_bindings/dotnet/CoolBox.XamarinBindings/CoolBox.XamarinBindings.csproj
```

## Xamarin usage notes

- Target framework is netstandard2.0 for Xamarin compatibility.
- Ensure the native library is packaged with your Xamarin app:
  - Android: include libcoolbox_xamarin_bindings.so in the proper ABI folders.
  - iOS/macOS: include libcoolbox_xamarin_bindings.dylib and set load path/signing as required.
  - Windows: include coolbox_xamarin_bindings.dll in output.
