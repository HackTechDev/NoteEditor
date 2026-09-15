# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

NoteEditor: a tabbed text editor GUI, Python + PyQt6, single-window `QMainWindow` app in `Python/main.py`. No package/build system — it's a flat set of modules imported directly. The Python implementation lives under `Python/`; other language implementations, if any get added, should get their own top-level directory alongside it rather than mixing into the same one.

## Commands

```bash
cd Python
pip install -r requirements.txt   # only dependency: PyQt6
python3 main.py                   # run the app
```

There is no test suite, linter, or build step configured. Verify changes by actually running the app (see "Testing a GUI change" below) — do not assume correctness from reading the code alone, especially anything involving Qt layout/geometry.

### Testing a GUI change

This is a desktop GUI; there's no headless assertion suite to run. Two techniques used throughout development:

- **Offscreen smoke test** (no display needed, safe for functional checks), run from `Python/`: `QT_QPA_PLATFORM=offscreen python3 -c "import main; app = main.QApplication([]); w = main.MainWindow(); ..."` — drive the API directly (`w.new_tab(...)`, `w.close_tab(...)`, etc.) and assert on state.
- **Visual check**: render with `widget.grab().save("out.png")` and read the PNG back. This renders the widget's own paint buffer — it is *not* a screen capture and is safe to use even on a real, shared `DISPLAY`.
- **Never** run a full-screen capture tool (e.g. `import -window root`, `scrot`) against a real `DISPLAY` — if it's the user's actual desktop rather than an isolated virtual one, this captures whatever else is on screen (other windows, private content). Stick to `widget.grab()`.
- When testing session/draft persistence, always override `HOME` to a scratch directory (`env HOME=/tmp/.../fakehome`). The real `~/.noteeditor` is the user's actual data — never write to it from a test run.

## Architecture

### Module map

| Module | Responsibility |
|---|---|
| `main.py` | `MainWindow` — owns the tab widget, menus/actions, the drafts sidebar, and all file I/O (open/save/close) |
| `editor_widget.py` | `Editor(QPlainTextEdit)` — one per tab; owns line-number gutter, current-line highlight, and its own `session_id` |
| `highlighters.py` | `QSyntaxHighlighter` subclasses (Python/JSON/Markdown) selected by file extension |
| `find_replace.py` | `FindReplaceDialog` — non-modal find/replace panel |
| `drafts_browser.py` | `DraftsBrowser(QListWidget)` — sidebar listing every known draft, reads from `session.py` |
| `session.py` | All disk persistence under `~/.noteeditor/` — the only module that touches that directory |

### The `~/.noteeditor` persistence model

This is the part that spans multiple files and isn't obvious from any single one. Four things live under `~/.noteeditor/`:

- `session.json` — the tabs currently open (file path if any, default name, modified flag) plus which one is active. Read once at startup to restore the window; rewritten wholesale on every quit.
- `index.json` — accumulated metadata (file path, default name, modified flag) for **every** draft ever seen, keyed by `session_id`. Never pruned automatically — entries only disappear via explicit deletion from the drafts sidebar.
- `drafts/<session_id>.txt` — the raw text backup for one tab. Written immediately when a tab is created, again when it's closed (if modified), and unconditionally for every open tab when the app quits.
- `docs/<default_name>.txt` — where `Ctrl+S` lands for a tab that has no real file yet (see `save_file()`). This is a genuinely saved file, not a backup.

Key invariant: the `drafts/` backup **never** writes to the user's real file location — only `_write_file()` (triggered by an explicit Save/Save As) touches a path outside `~/.noteeditor`. Don't blur this line when adding auto-save behavior.

Every `Editor` gets a `session_id` (`uuid4().hex`) at construction (`editor_widget.py`). That id — not tab index, not file path — is the stable key threading together the open tab, its draft file on disk, its `index.json` entry, and its row in the drafts sidebar (`QListWidgetItem` stores the whole entry dict via `Qt.ItemDataRole.UserRole`). Tab reordering (drag & drop is enabled) never invalidates this.

There is deliberately **no "unsaved changes?" confirmation dialog anywhere** — closing a tab or quitting always archives silently to `drafts/` and proceeds. The drafts sidebar is the safety net that makes this acceptable; don't reintroduce a blocking confirmation without reconsidering that tradeoff.

Sidebar behavior: it lists every draft in `index.json`/on disk, open or closed — currently-open ones get a `(ouvert)` suffix rather than being hidden. Clicking an already-open one switches to it instead of duplicating the tab (`_open_draft` in `main.py`).

### The tab-bar "+" button

`main.py` maintains **two** separate button widgets for "new tab" (`new_tab_button` and `new_tab_corner_button`, toggled by `_toggle_new_tab_button_mode`), not one. This looks redundant but isn't: PyQt6/Qt has no reliable way to place a widget "just after the last tab" that also survives the tab bar overflowing into scroll arrows. The two-button split (inline child positioned via manual `.move()` math while tabs fit; a `QTabWidget` corner widget — `_CornerToolButton`, which overrides `sizeHint()` rather than using `setFixedSize()` because Qt silently collapses a corner widget's height to 0 otherwise — once they overflow) was arrived at after several failed single-widget approaches. If touching this code, re-test both the "few tabs" and "many tabs, scroll arrows visible" cases, not just one.

`app.setStyle("Fusion")` is forced in `main()` — without it, some native/GTK Qt styles silently ignore parts of the custom QSS (this was root-caused, not a stylistic preference). Removing it will reintroduce theme-dependent rendering bugs.

### Syntax highlighting

`highlighters.highlighter_class_for(path)` maps a file extension to a `QSyntaxHighlighter` subclass; `Editor.set_file_path()` calls it and swaps the highlighter whenever a tab's associated path changes (open, Save As to a new extension). Colors are tuned for a **light** background only — there's no dark-mode variant yet (see `IMPROVEMENTS.md`).
