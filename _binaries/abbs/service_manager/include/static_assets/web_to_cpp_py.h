// Auto-generated from web_to_cpp.py
#pragma once

namespace resources {
    constexpr const char* web_to_cpp_py = R"PY(
#!/usr/bin/env python3
import sys
import os
import re
import glob

def asset_to_varname(asset_path):
    base = os.path.basename(asset_path)
    name, ext = os.path.splitext(base)
    name = name.replace('-', '_')
    return f"{name}_{ext[1:]}" if ext else name

def asset_to_header(asset_path):
    base = os.path.basename(asset_path)
    name, ext = os.path.splitext(base)
    name = name.replace('-', '_')
    return f"{name}_{ext[1:]}.h" if ext else f"{name}.h"

def asset_to_namespace(asset_path):
    return "resources"

def write_asset_header(asset_path, out_dir):
    var_name = asset_to_varname(asset_path)
    header_name = asset_to_header(asset_path)
    ns = asset_to_namespace(asset_path)
    ext = os.path.splitext(asset_path)[1].lower()
    tag = ext[1:].upper() if ext else "RAW"
    with open(asset_path, 'rb') as f:
        content = f.read()
    # For text files, decode as utf-8, else hex encode
    try:
        content_str = content.decode('utf-8')
        cpp = f"// Auto-generated from {os.path.basename(asset_path)}\n#pragma once\n\nnamespace {ns} {{\n    constexpr const char* {var_name} = R\"{tag}(\n{content_str}\n){tag}\";\n}}\n"
    except UnicodeDecodeError:
        # Binary: encode as hex string
        hexstr = content.hex()
        cpp = f"// Auto-generated from {os.path.basename(asset_path)}\n#pragma once\n\nnamespace {ns} {{\n    constexpr const char* {var_name}_hex = \"{hexstr}\";\n}}\n"
    out_path = os.path.join(out_dir, header_name)
    with open(out_path, 'w') as f:
        f.write(cpp)
    print(f"[HEADER] {asset_path} -> {out_path}")

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: web_to_cpp.py <assets_dir>")
        sys.exit(1)
    assets_dir = sys.argv[1]
    out_dir = os.path.join(os.getcwd(), '_binaries/apps/app_service_manager/include/static_assets')
    os.makedirs(out_dir, exist_ok=True)
    # Find all files (html, js, css, images, fonts, etc.)
    for path in glob.glob(os.path.join(assets_dir, '**'), recursive=True):
        if os.path.isfile(path):
            write_asset_header(path, out_dir)

)PY";
}
