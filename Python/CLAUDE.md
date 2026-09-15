# CLAUDE.md (Python/)

See the repository root `CLAUDE.md` first for the architecture shared with `Cpp/`. This file only covers what's specific to the Python implementation.

## Commands

```bash
pip install -r requirements.txt   # only dependency: PyQt6
python3 main.py                   # run the app
```

No package/build system — it's a flat set of modules imported directly, run from within this directory.

## Testing a GUI change

Run from `Python/`:

```bash
QT_QPA_PLATFORM=offscreen python3 -c "
import main
app = main.QApplication([])
w = main.MainWindow()
# drive the API: w.new_tab(...), w.close_tab(...), etc., and assert on state
"
```

For a visual check, add `w.grab().save('/tmp/out.png')` and read the PNG back — see the root `CLAUDE.md` for why this is safe to do even on a real `DISPLAY`, unlike a full-screen capture tool. Always override `HOME` (`env HOME=/tmp/.../fakehome ...`) when the test touches session/draft persistence.
