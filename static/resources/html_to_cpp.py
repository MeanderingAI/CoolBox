#!/usr/bin/env python3
import sys
import os

def html_to_cpp(html_file, output_file, var_name):
    with open(html_file, 'r') as f:
        html_content = f.read()
    
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

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: html_to_cpp.py <input.html> <output.h> <variable_name>")
        sys.exit(1)
    
    html_to_cpp(sys.argv[1], sys.argv[2], sys.argv[3])
