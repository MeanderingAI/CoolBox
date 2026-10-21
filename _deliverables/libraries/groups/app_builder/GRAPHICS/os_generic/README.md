# OS-generic native menus

Include `native_menu.hpp` and link `graphics::os_generic` (or
`full_application_window`). The API is in `graphics::os_generic`; rendering
and event dispatch reuse the window's native backend:

| Platform | Presentation |
| --- | --- |
| Windows | Win32 window menu bar |
| macOS | Cocoa application menu bar |
| Linux/X11 | GTK 3 menu bar |

```cpp
#include "native_menu.hpp"
#include "full_application_window.hpp"

graphics::full_application_window::FullApplicationWindow window;
graphics::os_generic::NativeMenuBar menus(window);
menus.set_menus({
    {"&File", {
        {"file.open", "Open...", "Ctrl+O", [&] { open_document(); }},
        {"file.save", "Save", "Ctrl+S", [&] { save_document(); }},
        graphics::os_generic::MenuAction::divider(),
        {"file.close", "Close", "", [&] { window.close(); }}
    }},
    {"&Edit", {
        {"edit.undo", "Undo", "Ctrl+Z", [&] { undo(); }},
        {"edit.redo", "Redo", "Ctrl+Shift+Z", [&] { redo(); }}
    }}
});
menus.set_enabled("edit.undo", can_undo());
menus.set_checked("file.save", false);
window.create();
```

Applications supply labels and actions; the API does not assume a document
model or implement file operations itself. Command IDs are unique across the
menu bar and stay stable when menu items are reordered. Separators need no ID.
Actions require nonempty IDs, labels and callbacks. Invalid definitions throw
`std::invalid_argument` without replacing the installed menu. Unknown command
IDs also throw; disabled actions cannot dispatch.

Set menus before or after creating the window. Changes to enabled/checked
states rebuild the native presentation. Callbacks can replace the menu safely.
Use this API on the window's UI thread. Callbacks retain their captures until
the menu is replaced or the window is destroyed; captured objects must remain
alive. The window must outlive the controller. Do not mix direct
`set_menu_bar`/`set_menu_command_handler` calls with this controller.

`supported()` reports whether a native menu backend was built. `set_menus`
throws on unsupported/headless backends rather than silently drawing a
non-native menu. Linux requires GTK 3.20+, GDK-X11, X11 and pkg-config.
macOS uses the system application menu bar, not a menu inside each canvas.

Shortcut strings use `Ctrl+O`, `Ctrl+Shift+Z`, etc. Cocoa maps Ctrl to Command;
GTK registers accelerators. Win32 displays shortcut labels; applications
continue handling the matching key events through their existing key handler.
Top-level `&` mnemonics are handled by the native backend.

The existing low-level `MenuBarModel` and window APIs remain supported for
other consumers. `FullApplicationWindowTests` covers validation, command
dispatch, state changes, callback lifetime and real Win32/GTK activation.
