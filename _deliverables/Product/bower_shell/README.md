# Bower Shell Product

This product is a standalone shell application under `_Product/bower_shell`.

Structure:

- `CMakeLists.txt` — build entry for the shell product
- `src/main.cpp` — interactive REPL and single-command execution entry point

The product embeds `bower_shell_lib` so the shell engine can also be reused from other products, including the editor product.