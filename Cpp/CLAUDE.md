# CLAUDE.md (Cpp/)

See the repository root `CLAUDE.md` first for the architecture shared with `Python/`. This file only covers what's specific to the C++ implementation.

## Commands

Requires `qt6-base-dev` (provides the headers and `Qt6Config.cmake`), a C++17 compiler, and CMake ≥ 3.16. On a machine with only the Qt6 *runtime* libraries installed (no `-dev` package), `cmake -B build` fails with "Could not find a package configuration file provided by Qt6" — install `qt6-base-dev` first.

```bash
cmake -B build
cmake --build build -j"$(nproc)"
./build/NoteEditor
```

`CMAKE_AUTOMOC` is on, so any `QObject` subclass (anything with `Q_OBJECT`) must live in a header (`.h`) that gets `#include`d, not defined inline in a `.cpp` — AUTOMOC only scans headers listed as sources in `CMakeLists.txt` by default.

## Testing a GUI change

There's no test target in `CMakeLists.txt` — verify by building a throwaway driver against the real sources instead of editing the shipped files. Pattern (used during the initial port, not checked in):

1. In a scratch directory, write a minimal `CMakeLists.txt` that `add_executable`s a small `main()` alongside the actual `MainWindow.cpp`/`Editor.cpp`/etc. from this directory (reference them by absolute path — no need to copy).
2. In that `main()`, construct a `MainWindow`, drive its public API (`newTab(...)`, `closeTab(...)`, `currentEditor()`, ...), and assert with plain `if`/`fprintf` (no gtest dependency needed for something this small).
3. Build and run with `QT_QPA_PLATFORM=offscreen` and an overridden `HOME` (see root `CLAUDE.md` — same rules apply: never touch the real `~/.noteeditor`, never full-screen-capture a real `DISPLAY`; `widget->grab()` is safe).

Private slots (e.g. `saveFile()`) are still reachable from such a driver via `QMetaObject::invokeMethod(window, "saveFile")` without having to change their access level just for testing.

## Gotchas hit during the Python → C++ port

- `QJsonObject::remove(key)` returns `void`, not `bool` — check `contains()` first if you need to know whether something was actually removed (`Session.cpp` does this for `index.json` cleanup).
- `QTabWidget::removeTab()` does **not** delete the removed page widget (unlike PyQt6, where Python's GC eventually collects it once unreferenced). `MainWindow::closeTab()` calls `editor->deleteLater()` explicitly after `removeTab()` — don't drop that when touching tab-closing code, it'll leak.
- Qt6 C++ uses the same enum scoping PyQt6 exposes (e.g. `Qt::AlignRight`, `QTextDocument::FindBackward`), so porting Python Qt code is close to mechanical — the main translation cost is Python's dynamic dict-shaped "entry" objects becoming explicit structs (`Session::TabSnapshot`, `Session::DraftEntry`).
