import datetime
import json
import os

CONFIG_DIR = os.path.expanduser("~/.noteeditor")
DRAFTS_DIR = os.path.join(CONFIG_DIR, "drafts")
DOCS_DIR = os.path.join(CONFIG_DIR, "docs")
TRASH_DIR = os.path.join(CONFIG_DIR, "trash")
VERSIONS_DIR = os.path.join(CONFIG_DIR, "versions")
SESSION_FILE = os.path.join(CONFIG_DIR, "session.json")
INDEX_FILE = os.path.join(CONFIG_DIR, "index.json")
TRASH_INDEX_FILE = os.path.join(CONFIG_DIR, "trash_index.json")
WINDOW_FILE = os.path.join(CONFIG_DIR, "window.json")

MAX_VERSIONS = 10


def _load_json(path, default):
    if not os.path.isfile(path):
        return default
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return default


def _load_index():
    return _load_json(INDEX_FILE, {})


def _save_index(index):
    with open(INDEX_FILE, "w", encoding="utf-8") as f:
        json.dump(index, f, ensure_ascii=False, indent=2)


def _load_trash_index():
    return _load_json(TRASH_INDEX_FILE, {})


def _save_trash_index(index):
    with open(TRASH_INDEX_FILE, "w", encoding="utf-8") as f:
        json.dump(index, f, ensure_ascii=False, indent=2)


def save_session(tabs_info, active_id=None):
    os.makedirs(DRAFTS_DIR, exist_ok=True)

    entries = []
    index = _load_index()
    for info in tabs_info:
        draft_path = os.path.join(DRAFTS_DIR, info["id"] + ".txt")
        with open(draft_path, "w", encoding="utf-8") as f:
            f.write(info["content"])
        entries.append(
            {
                "id": info["id"],
                "file_path": info["file_path"],
                "default_name": info["default_name"],
                "modified": info["modified"],
            }
        )
        index[info["id"]] = {
            "file_path": info["file_path"],
            "default_name": info["default_name"],
            "modified": info["modified"],
        }

    with open(SESSION_FILE, "w", encoding="utf-8") as f:
        json.dump({"active_id": active_id, "tabs": entries}, f, ensure_ascii=False, indent=2)

    _save_index(index)


def save_draft(info):
    """Persist a single tab's content to the drafts store, without touching the active session list."""
    os.makedirs(DRAFTS_DIR, exist_ok=True)
    draft_path = os.path.join(DRAFTS_DIR, info["id"] + ".txt")
    with open(draft_path, "w", encoding="utf-8") as f:
        f.write(info["content"])

    index = _load_index()
    index[info["id"]] = {
        "file_path": info["file_path"],
        "default_name": info["default_name"],
        "modified": info["modified"],
    }
    _save_index(index)


def rename_draft(draft_id, new_default_name):
    """Renames an untitled draft (one with no real file_path)."""
    index = _load_index()
    entry = index.get(draft_id, {})
    entry["default_name"] = new_default_name
    index[draft_id] = entry
    _save_index(index)


def load_session():
    data = _load_json(SESSION_FILE, None)
    if data is None:
        return [], None

    if isinstance(data, list):
        data = {"active_id": None, "tabs": data}

    result = []
    for entry in data.get("tabs", []):
        draft_path = os.path.join(DRAFTS_DIR, entry["id"] + ".txt")
        try:
            with open(draft_path, "r", encoding="utf-8") as f:
                content = f.read()
        except OSError:
            continue
        result.append({**entry, "content": content})
    return result, data.get("active_id")


def list_drafts():
    """All draft files on disk, newest first, with the best label metadata available."""
    if not os.path.isdir(DRAFTS_DIR):
        return []

    index = _load_index()
    session_data = _load_json(SESSION_FILE, {})
    if isinstance(session_data, list):
        session_data = {"tabs": session_data}
    session_meta = {entry["id"]: entry for entry in session_data.get("tabs", [])}

    items = []
    for name in os.listdir(DRAFTS_DIR):
        if not name.endswith(".txt"):
            continue
        draft_id = name[:-4]
        path = os.path.join(DRAFTS_DIR, name)
        try:
            mtime = os.path.getmtime(path)
        except OSError:
            continue
        meta = index.get(draft_id) or session_meta.get(draft_id) or {}
        items.append(
            {
                "id": draft_id,
                "file_path": meta.get("file_path"),
                "default_name": meta.get("default_name"),
                "modified": meta.get("modified", True),
                "mtime": mtime,
            }
        )

    items.sort(key=lambda e: e["mtime"], reverse=True)
    return items


def read_draft(draft_id):
    path = os.path.join(DRAFTS_DIR, draft_id + ".txt")
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def delete_draft(draft_id):
    """Permanently removes a draft (file + index entry), no trash involved."""
    path = os.path.join(DRAFTS_DIR, draft_id + ".txt")
    try:
        os.remove(path)
    except OSError:
        pass
    index = _load_index()
    if index.pop(draft_id, None) is not None:
        _save_index(index)


def trash_draft(draft_id):
    """Moves a draft to the trash instead of deleting it outright."""
    os.makedirs(TRASH_DIR, exist_ok=True)
    src = os.path.join(DRAFTS_DIR, draft_id + ".txt")
    dst = os.path.join(TRASH_DIR, draft_id + ".txt")
    try:
        os.replace(src, dst)
    except OSError:
        return

    index = _load_index()
    meta = index.pop(draft_id, {})
    _save_index(index)

    meta["deleted_at"] = datetime.datetime.now().isoformat()
    trash_index = _load_trash_index()
    trash_index[draft_id] = meta
    _save_trash_index(trash_index)


def list_trash():
    """All trashed drafts, most recently deleted first."""
    trash_index = _load_trash_index()
    items = []
    for draft_id, meta in trash_index.items():
        items.append(
            {
                "id": draft_id,
                "file_path": meta.get("file_path"),
                "default_name": meta.get("default_name"),
                "deleted_at": meta.get("deleted_at", ""),
            }
        )
    items.sort(key=lambda e: e["deleted_at"], reverse=True)
    return items


def restore_draft(draft_id):
    """Moves a trashed draft back into the active drafts store."""
    src = os.path.join(TRASH_DIR, draft_id + ".txt")
    dst = os.path.join(DRAFTS_DIR, draft_id + ".txt")
    os.makedirs(DRAFTS_DIR, exist_ok=True)
    try:
        os.replace(src, dst)
    except OSError:
        return

    trash_index = _load_trash_index()
    meta = trash_index.pop(draft_id, {})
    _save_trash_index(trash_index)
    meta.pop("deleted_at", None)

    index = _load_index()
    index[draft_id] = meta
    _save_index(index)


def purge_draft(draft_id):
    """Permanently deletes a trashed draft."""
    path = os.path.join(TRASH_DIR, draft_id + ".txt")
    try:
        os.remove(path)
    except OSError:
        pass
    trash_index = _load_trash_index()
    if trash_index.pop(draft_id, None) is not None:
        _save_trash_index(trash_index)


def save_version(draft_id, content):
    """Snapshots a tab's pre-save content, keeping only the most recent MAX_VERSIONS."""
    version_dir = os.path.join(VERSIONS_DIR, draft_id)
    os.makedirs(version_dir, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%dT%H%M%S%f")
    with open(os.path.join(version_dir, stamp + ".txt"), "w", encoding="utf-8") as f:
        f.write(content)

    versions = sorted(os.listdir(version_dir), reverse=True)
    for stale in versions[MAX_VERSIONS:]:
        try:
            os.remove(os.path.join(version_dir, stale))
        except OSError:
            pass


def list_versions(draft_id):
    """Timestamps (newest first) of the saved versions kept for a tab."""
    version_dir = os.path.join(VERSIONS_DIR, draft_id)
    if not os.path.isdir(version_dir):
        return []
    names = sorted((n for n in os.listdir(version_dir) if n.endswith(".txt")), reverse=True)
    return [n[:-4] for n in names]


def read_version(draft_id, stamp):
    path = os.path.join(VERSIONS_DIR, draft_id, stamp + ".txt")
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def save_window_state(width, height, splitter_sizes):
    """Persists the window size and sidebar-splitter position across launches."""
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(WINDOW_FILE, "w", encoding="utf-8") as f:
        json.dump(
            {"width": width, "height": height, "splitter_sizes": list(splitter_sizes)},
            f,
            ensure_ascii=False,
            indent=2,
        )


def load_window_state():
    """Returns {"width", "height", "splitter_sizes"} or None if never saved."""
    return _load_json(WINDOW_FILE, None)
