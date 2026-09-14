import json
import os

CONFIG_DIR = os.path.expanduser("~/.noteeditor")
DRAFTS_DIR = os.path.join(CONFIG_DIR, "drafts")
SESSION_FILE = os.path.join(CONFIG_DIR, "session.json")


def save_session(tabs_info):
    os.makedirs(DRAFTS_DIR, exist_ok=True)

    entries = []
    keep_ids = set()
    for info in tabs_info:
        draft_path = os.path.join(DRAFTS_DIR, info["id"] + ".txt")
        with open(draft_path, "w", encoding="utf-8") as f:
            f.write(info["content"])
        keep_ids.add(info["id"])
        entries.append(
            {
                "id": info["id"],
                "file_path": info["file_path"],
                "default_name": info["default_name"],
                "modified": info["modified"],
            }
        )

    with open(SESSION_FILE, "w", encoding="utf-8") as f:
        json.dump(entries, f, ensure_ascii=False, indent=2)

    for name in os.listdir(DRAFTS_DIR):
        if os.path.splitext(name)[0] not in keep_ids:
            try:
                os.remove(os.path.join(DRAFTS_DIR, name))
            except OSError:
                pass


def load_session():
    if not os.path.isfile(SESSION_FILE):
        return []
    try:
        with open(SESSION_FILE, "r", encoding="utf-8") as f:
            entries = json.load(f)
    except (OSError, ValueError):
        return []

    result = []
    for entry in entries:
        draft_path = os.path.join(DRAFTS_DIR, entry["id"] + ".txt")
        try:
            with open(draft_path, "r", encoding="utf-8") as f:
                content = f.read()
        except OSError:
            continue
        result.append({**entry, "content": content})
    return result
