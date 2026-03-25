# Apps

## Tutorial editor

Run the local tutorial editor with:

- macOS/Linux: `python apps/tutorial_editor.py`
- Windows: `python apps\tutorial_editor.py`

The editor is a lightweight Tkinter app for generating `.tut` files in the top-level [tutorials](../tutorials) folder.

### Supported insert actions

- `@title1` / `@title2`
- `@tags`
- `@image`
- `@video`
- `@link`
- paragraph blocks

Save tutorials as `.tut` files and the tutorials pipeline will turn them into static HTML pages for the documentation site.
