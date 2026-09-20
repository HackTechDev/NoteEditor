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
RECENT_FILE = os.path.join(CONFIG_DIR, "recent.json")

MAX_VERSIONS = 10
DEFAULT_RECENT = 10
MIN_RECENT = 1
MAX_RECENT = 50


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


def _merged_meta(index, info):
    """Index entry for a tab, keeping the "pinned" flag it may already carry:
    it is owned by set_pinned(), not by the tab snapshots that rewrite the rest."""
    meta = {
        "file_path": info["file_path"],
        "default_name": info["default_name"],
        "modified": info["modified"],
    }
    if index.get(info["id"], {}).get("pinned"):
        meta["pinned"] = True
    if index.get(info["id"], {}).get("remember"):
        meta["remember"] = True
    return meta


def is_pinned(draft_id):
    return bool(_load_index().get(draft_id, {}).get("pinned"))


def set_pinned(draft_id, pinned):
    """Pins/unpins a draft. A pinned draft can be neither closed nor trashed."""
    index = _load_index()
    entry = index.get(draft_id, {})
    if pinned:
        entry["pinned"] = True
    else:
        entry.pop("pinned", None)
    index[draft_id] = entry
    _save_index(index)


def is_remembered(draft_id):
    """True when the note asked to be remembered in the recently-closed list even
    if it is empty (an empty untitled note is skipped otherwise)."""
    return bool(_load_index().get(draft_id, {}).get("remember"))


def set_remembered(draft_id, remembered):
    index = _load_index()
    entry = index.get(draft_id, {})
    if remembered:
        entry["remember"] = True
    else:
        entry.pop("remember", None)
    index[draft_id] = entry
    _save_index(index)


def save_session(tabs_info, active_id=None):
    os.makedirs(DRAFTS_DIR, exist_ok=True)

    entries = []
    index = _load_index()
    for info in tabs_info:
        draft_path = os.path.join(DRAFTS_DIR, info["id"] + ".txt")
        with open(draft_path, "w", encoding="utf-8") as f:
            f.write(info["content"])
        entry = {
            "id": info["id"],
            "file_path": info["file_path"],
            "default_name": info["default_name"],
            "modified": info["modified"],
        }
        if "cursor" in info:
            # où l'on travaillait dans la note (session.json seulement, pas index.json)
            entry["cursor"] = info["cursor"]
            entry["scroll"] = info.get("scroll", 0)
        entries.append(entry)
        index[info["id"]] = _merged_meta(index, info)

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
    index[info["id"]] = _merged_meta(index, info)
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
                "pinned": bool(meta.get("pinned")),
                "remember": bool(meta.get("remember")),
                "mtime": mtime,
            }
        )

    items.sort(key=lambda e: e["mtime"], reverse=True)
    return items


def is_external_file(file_path):
    """True for a real file living outside ~/.noteeditor. Its text is safe in the
    file itself, so once closed it no longer needs a spot in the drafts sidebar;
    untitled notes and files under ~/.noteeditor (docs/) keep theirs."""
    if not file_path:
        return False
    path = os.path.normpath(os.path.abspath(file_path))
    root = os.path.normpath(os.path.abspath(CONFIG_DIR))
    return not (path == root or path.startswith(root + os.sep))


def _load_recent():
    data = _load_json(RECENT_FILE, {})
    entries = data.get("recent", []) if isinstance(data, dict) else []
    return [e for e in entries if isinstance(e, dict) and e.get("id")]


def get_recent_limit():
    """How many recently closed notes are remembered (recent.json "limit")."""
    data = _load_json(RECENT_FILE, {})
    limit = data.get("limit") if isinstance(data, dict) else None
    if isinstance(limit, int) and not isinstance(limit, bool):
        return max(MIN_RECENT, min(MAX_RECENT, limit))
    return DEFAULT_RECENT


def _save_recent(entries, limit=None):
    os.makedirs(CONFIG_DIR, exist_ok=True)
    # relire la limite AVANT d'ouvrir le fichier en écriture (qui le vide)
    data = {"recent": entries, "limit": get_recent_limit() if limit is None else limit}
    with open(RECENT_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def set_recent_limit(limit):
    """Changes the size of the recently-closed list; the list is trimmed at once."""
    limit = max(MIN_RECENT, min(MAX_RECENT, int(limit)))
    _save_recent(_load_recent()[:limit], limit)


def add_recent(entry):
    """Remembers a note that was just closed (newest first, at most get_recent_limit()).
    entry: {"id", "file_path", "default_name"}."""
    entries = [e for e in _load_recent() if e["id"] != entry["id"]]
    entries.insert(
        0,
        {"id": entry["id"], "file_path": entry.get("file_path"), "default_name": entry.get("default_name")},
    )
    _save_recent(entries[: get_recent_limit()])


def remove_recent(draft_id):
    entries = _load_recent()
    kept = [e for e in entries if e["id"] != draft_id]
    if len(kept) != len(entries):
        _save_recent(kept)


def clear_recent():
    if _load_recent():
        _save_recent([])


def list_recent():
    """Recently closed notes that can still be reopened: a file outside
    ~/.noteeditor must still exist, any other note must still have its draft."""
    drafts = {e["id"] for e in list_drafts()}
    result = []
    for entry in _load_recent():
        path = entry.get("file_path")
        if path and is_external_file(path):
            valid = os.path.isfile(path)
        else:
            valid = entry["id"] in drafts
        if valid:
            result.append(entry)
    return result


def find_draft_for_path(file_path):
    """The most recent draft bound to this file, or None. Opening a file must
    reuse it rather than mint a new draft, or every open/close cycle of the same
    file adds another entry to the drafts sidebar."""
    for entry in list_drafts():  # newest first
        if entry["file_path"] == file_path:
            return entry
    return None


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
    remove_recent(draft_id)


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
    remove_recent(draft_id)

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

    _prune_versions(draft_id)


def _prune_versions(draft_id):
    version_dir = os.path.join(VERSIONS_DIR, draft_id)
    try:
        versions = sorted(os.listdir(version_dir), reverse=True)
    except OSError:
        return
    for stale in versions[MAX_VERSIONS:]:
        try:
            os.remove(os.path.join(version_dir, stale))
        except OSError:
            pass


def merge_versions(from_id, to_id):
    """Moves the saved versions of one draft under another id (the newest
    MAX_VERSIONS are kept) — used when a note adopts the id of an existing note
    for the same file."""
    src = os.path.join(VERSIONS_DIR, from_id)
    if not os.path.isdir(src):
        return
    dst = os.path.join(VERSIONS_DIR, to_id)
    os.makedirs(dst, exist_ok=True)
    for name in os.listdir(src):
        try:
            os.replace(os.path.join(src, name), os.path.join(dst, name))
        except OSError:
            pass
    try:
        os.rmdir(src)
    except OSError:
        pass
    _prune_versions(to_id)


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


def save_window_state(width, height, splitter_sizes, x=None, y=None, word_wrap=None, markdown_preview=None):
    """Persists the window size, screen position, sidebar-splitter position and
    display settings (word wrap, Markdown preview) across launches. Everything
    but the size and splitter is optional: omitted from the file when unknown."""
    os.makedirs(CONFIG_DIR, exist_ok=True)
    state = {"width": width, "height": height, "splitter_sizes": list(splitter_sizes)}
    if x is not None and y is not None:
        state["x"] = x
        state["y"] = y
    if word_wrap is not None:
        state["word_wrap"] = bool(word_wrap)
    if markdown_preview is not None:
        state["markdown_preview"] = bool(markdown_preview)
    with open(WINDOW_FILE, "w", encoding="utf-8") as f:
        json.dump(state, f, ensure_ascii=False, indent=2)


def load_window_state():
    """Returns {"width", "height", "splitter_sizes"[, "x", "y", "word_wrap", "markdown_preview"]}
    or None if never saved."""
    return _load_json(WINDOW_FILE, None)
