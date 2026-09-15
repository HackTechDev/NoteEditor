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

Private slots (e.g. `saveFile()`) are still reachable from such a driver via `QMetaObject::invokeMethod(window, "saveFile")` without having to change their access level just for testing. For private *methods* that aren't slots (e.g. `duplicateTab`, `renameTab`, `closeOtherTabs`) — which most of the tab-context-menu actions are — `invokeMethod` doesn't apply. Use the `#define private public` / `#include "MainWindow.h"` / `#undef private` trick in the throwaway driver instead: it's a text substitution the preprocessor happily does even though `private` is a keyword, and since it's scoped to that one driver's translation unit it never touches the shipped headers or the real `MainWindow.cpp`'s own compilation. `signals:` members don't need this at all — Qt defines `signals` as a synonym for `public`, so `emit editor->autosaveRequested()` (or just calling it directly) is always fine from outside the class. Protected event handlers (`dropEvent`, etc.) likewise don't need it if you dispatch through `QApplication::sendEvent(widget, &event)` rather than calling the override directly.

Testing any code path that pops a modal `QMessageBox`/`QInputDialog` (trashing, renaming, the external-change prompt) needs a `QTimer::singleShot(0, ...)` scheduled *before* the call that blocks — it fires once the dialog's nested event loop starts spinning. Two traps here, both hit during this port:
- `QMessageBox::question()`/`::warning()` determine their return value from `clickedButton()`, not from the raw `exec()`/`done()` result code. Calling `box->done(QMessageBox::Yes)` closes the dialog but the static wrapper still reports `NoButton`. Fetch the actual button and click it: `box->button(QMessageBox::Yes)->click()`.
- If the code path you're testing has an early-return that skips opening a dialog in some case (e.g. `renameTab()` on a tab that has a real `filePath`), don't schedule a responder for that case — the `singleShot` fires on the *next* nested event loop it finds one to run in, which is very likely a **later** dialog in the same test, corrupting an unrelated assertion instead of a harmless no-op.

## Gotchas hit during the Python → C++ port

- `QJsonObject::remove(key)` returns `void`, not `bool` — check `contains()` first if you need to know whether something was actually removed (`Session.cpp` does this for `index.json`/`trash_index.json` cleanup).
- `QTabWidget::removeTab()` does **not** delete the removed page widget (unlike PyQt6, where Python's GC eventually collects it once unreferenced). `MainWindow::closeTab()` calls `editor->deleteLater()` explicitly after `removeTab()` — don't drop that when touching tab-closing code, it'll leak.
- Qt6 C++ uses the same enum scoping PyQt6 exposes (e.g. `Qt::AlignRight`, `QTextDocument::FindBackward`), so porting Python Qt code is close to mechanical — the main translation cost is Python's dynamic dict-shaped "entry" objects becoming explicit structs (`Session::TabSnapshot`, `Session::DraftEntry`, `Session::TrashEntry`).
- `Editor::autosaveRequested()` is a real signal wired straight to a `QTimer::timeout` (`connect(timer, &QTimer::timeout, this, &Editor::autosaveRequested)`) — connecting a signal directly to another signal re-emits it, no slot needed in between, same as PyQt6 allows.
