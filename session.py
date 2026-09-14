import json
import os

CONFIG_DIR = os.path.expanduser("~/.noteeditor")
DRAFTS_DIR = os.path.join(CONFIG_DIR, "drafts")
SESSION_FILE = os.path.join(CONFIG_DIR, "session.json")
INDEX_FILE = os.path.join(CONFIG_DIR, "index.json")


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
    path = os.path.join(DRAFTS_DIR, draft_id + ".txt")
    try:
        os.remove(path)
    except OSError:
        pass
    index = _load_index()
    if index.pop(draft_id, None) is not None:
        _save_index(index)
