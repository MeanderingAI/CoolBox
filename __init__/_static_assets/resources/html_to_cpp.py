#!/usr/bin/env python3
import sys
import os
import re

def html_to_cpp(html_file, output_file, var_name):
    with open(html_file, 'r') as f:
        html_content = f.read()
    # Inline assets
    base_dir = os.path.dirname(os.path.abspath(html_file))
    html_content = inline_assets(html_content, base_dir)
    # Copy module JS files to output folder for static serving
    module_scripts = getattr(inline_assets, 'module_scripts', [])
    if module_scripts:
        output_static_dir = os.path.join(os.path.dirname(output_file), 'static_assets')
        os.makedirs(output_static_dir, exist_ok=True)
        for src in module_scripts:
            rel_path = src.lstrip('/')
            src_path = resolve_asset_path(src, base_dir, os.path.abspath(os.path.join(base_dir, '../../../')))
            dst_path = os.path.join(output_static_dir, os.path.basename(rel_path))
            if os.path.exists(src_path):
                import shutil
                shutil.copy2(src_path, dst_path)
                print(f"[COPY] {src_path} -> {dst_path}")
            else:
                print(f"[ERROR] Module JS file not found for copy: {src_path}", file=sys.stderr)
    # Escape special characters
    html_content = html_content.replace('\\', '\\\\')
    html_content = html_content.replace('"', '\\"')
    html_content = html_content.replace('\n', '\\n')
    # Generate C++ header
    cpp_content = f'''// Auto-generated from {os.path.basename(html_file)}
#pragma once

namespace resources {{
    constexpr const char* {var_name} = "{html_content}";
}}
'''
    with open(output_file, 'w') as f:
        f.write(cpp_content)
    print(f"Generated {output_file}")

def resolve_asset_path(asset_path, base_dir, project_root):
    if asset_path.startswith('/'):
        # Remove leading slash and join with project root
        return os.path.join(project_root, asset_path.lstrip('/'))
    else:
        return os.path.join(base_dir, asset_path)

def inline_assets(html_content, base_dir):
    # Find project root (one level up from _static_assets/resources/html)
    project_root = os.path.abspath(os.path.join(base_dir, '../../../'))
    print(f"[DEBUG] base_dir: {base_dir}")
    print(f"[DEBUG] project_root: {project_root}")
    # Print all <link> and <script> tags found
    for m in re.finditer(r'<link.*?>', html_content, flags=re.IGNORECASE|re.DOTALL):
        print(f"[DEBUG] Found <link>: {m.group(0)}")
    for m in re.finditer(r'<script.*?>', html_content, flags=re.IGNORECASE|re.DOTALL):
        print(f"[DEBUG] Found <script>: {m.group(0)}")
    def replace_css(match):
        href = match.group(1)
        css_path = resolve_asset_path(href, base_dir, project_root)
        print(f"[DEBUG] Attempting to inline CSS: {href} -> {css_path}")
        if os.path.exists(css_path):
            with open(css_path, 'r') as f:
                css = f.read()
            print(f"[INLINE] CSS: {href} -> {css_path}")
            return f'<style>\n{css}\n</style>'
        print(f"[ERROR] CSS file not found: {css_path}", file=sys.stderr)
        return match.group(0)
    module_scripts = []
    def replace_js(match):
        # Combine all attributes before and after src
        attrs = match.group(1) + match.group(3)
        src = match.group(2)
        # Check if type="module" is present in any attribute (robust to spacing, order, multiline)
        if re.search(r'type\s*=\s*["\']module["\']', attrs, re.IGNORECASE):
            module_scripts.append(src)
            print(f"[SKIP INLINE] <script type=module src={src}>")
            return match.group(0)  # Leave tag as-is
        js_path = resolve_asset_path(src, base_dir, project_root)
        print(f"[DEBUG] Attempting to inline JS: {src} -> {js_path}")
        if os.path.exists(js_path):
            with open(js_path, 'r') as f:
                js = f.read()
            print(f"[INLINE] JS: {src} -> {js_path}")
            return f'<script>\n{js}\n</script>'
        print(f"[ERROR] JS file not found: {js_path}", file=sys.stderr)
        return match.group(0)
    # More robust regex: allow for extra whitespace, newlines, and attributes
    html_content = re.sub(r'<link\s+[^>]*rel=["\']stylesheet["\'][^>]*href=["\']([^"\']+)["\'][^>]*/?>', replace_css, html_content, flags=re.IGNORECASE|re.DOTALL)
    # Improved regex: match <script ... src="..."> with any attribute order, multiline, and spacing
    html_content = re.sub(r'<script([^>]*)src=["\']([^"\']+)["\']([^>]*)></script>', replace_js, html_content, flags=re.IGNORECASE|re.DOTALL)
    inline_assets.module_scripts = module_scripts
    return html_content


import glob
def asset_to_varname(asset_path):
    # e.g. js/service_manager.js -> service_manager_js
    base = os.path.basename(asset_path)
    name, ext = os.path.splitext(base)
    name = name.replace('-', '_')
    return f"{name}_{ext[1:]}" if ext else name

def asset_to_header(asset_path):
    # e.g. js/service_manager.js -> service_manager_js.h
    base = os.path.basename(asset_path)
    name, ext = os.path.splitext(base)
    name = name.replace('-', '_')
    return f"{name}_{ext[1:]}.h" if ext else f"{name}.h"

def asset_to_namespace(asset_path):
    # e.g. js/service_manager.js -> resources
    return "resources"

def write_asset_header(asset_path, out_dir):
    var_name = asset_to_varname(asset_path)
    header_name = asset_to_header(asset_path)
    ns = asset_to_namespace(asset_path)
    ext = os.path.splitext(asset_path)[1]
    tag = "HTML" if ext == ".html" else ("CSS" if ext == ".css" else "JS")
    with open(asset_path, 'r') as f:
        content = f.read()
    # Escape for C++ raw string
    cpp = f"// Auto-generated from {os.path.basename(asset_path)}\n#pragma once\n\nnamespace {ns} {{\n    constexpr const char* {var_name} = R\"{tag}(\n{content}\n){tag}\";\n}}\n"
    out_path = os.path.join(out_dir, header_name)
    with open(out_path, 'w') as f:
        f.write(cpp)
    print(f"[HEADER] {asset_path} -> {out_path}")

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: html_to_cpp.py <assets_dir>")
        sys.exit(1)
    assets_dir = sys.argv[1]
    out_dir = os.path.join(os.getcwd(), '_binaries/apps/app_service_manager/include/static_assets')
    os.makedirs(out_dir, exist_ok=True)
    # Find all .html, .js, .css files in subfolders
    for ext in ("html", "js", "css"):
        for path in glob.glob(os.path.join(assets_dir, "**", f"*.{ext}"), recursive=True):
            write_asset_header(path, out_dir)
