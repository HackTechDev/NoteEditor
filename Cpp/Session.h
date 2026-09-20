#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

// All disk persistence under ~/.noteeditor/. Mirrors Python/session.py.
namespace Session {

// A tab's full state, including its text content (used to write drafts /
// the session file, and to restore them).
struct TabSnapshot {
    QString id;
    QString filePath;    // empty == no associated file
    QString defaultName; // empty == not applicable (has a real filePath)
    bool modified = false;
    QString content;
    int cursor = -1; // -1 == unknown (not written to session.json)
    int scroll = 0;
};

// Metadata only (no content), as read back from index.json / session.json
// for the drafts sidebar.
struct DraftEntry {
    QString id;
    QString filePath;
    QString defaultName;
    bool modified = true;
    bool pinned = false;
    bool remember = false; // asked to be remembered in the recently-closed list even if empty
    qint64 mtimeMs = 0;
};

// A trashed draft's metadata, as read back from trash_index.json.
struct TrashEntry {
    QString id;
    QString filePath;
    QString defaultName;
    QString deletedAt; // ISO 8601, empty if unknown
};

// The window size and sidebar-splitter position, as read back from
// window.json. `valid` is false when nothing has ever been saved (first run).
struct WindowState {
    int width = 0;
    int height = 0;
    QList<int> splitterSizes;
    int x = 0;
    int y = 0;
    bool hasPosition = false; // false for a window.json saved before x/y existed
    bool hasWordWrap = false;
    bool wordWrap = true;
    bool hasMarkdownPreview = false;
    bool markdownPreview = false;
    bool valid = false;
};

QString configDir();
QString draftsDir();
QString docsDir();
QString trashDir();
QString versionsDir();

// Persists every currently-open tab (overwrites session.json wholesale) and
// merges their metadata into index.json.
void saveSession(const QVector<TabSnapshot> &tabs, const QString &activeId);

// Persists a single tab's content without touching the open-tabs list.
void saveDraft(const TabSnapshot &info);

// Returns the tabs to restore (with content) and the id of the active one.
QVector<TabSnapshot> loadSession(QString *activeId);

// Every draft file on disk, newest first, with the best label metadata
// available (open or closed).
QVector<DraftEntry> listDrafts();

QString readDraft(const QString &draftId);

// True for a real file living outside ~/.noteeditor. Its text is safe in the
// file itself, so once closed it no longer needs a spot in the drafts sidebar;
// untitled notes and files under ~/.noteeditor (docs/) keep theirs.
bool isExternalFile(const QString &filePath);

// Recently closed notes (recent.json), newest first, at most kMaxRecent. Entries
// use id / filePath / defaultName of DraftEntry.
void addRecent(const DraftEntry &entry);
void removeRecent(const QString &draftId);
void clearRecent();
// Only the ones that can still be reopened: a file outside ~/.noteeditor must
// still exist, any other note must still have its draft.
QVector<DraftEntry> listRecent();

// The most recent draft bound to this file (empty id if none). Opening a file
// must reuse it rather than mint a new draft, or every open/close cycle of the
// same file adds another entry to the drafts sidebar.
DraftEntry findDraftForPath(const QString &filePath);

// A pinned draft can be neither closed nor trashed. Stored in index.json.
bool isPinned(const QString &draftId);
void setPinned(const QString &draftId, bool pinned);

// True when the note asked to be remembered in the recently-closed list even if
// it is empty (an empty untitled note is skipped otherwise). Stored in index.json.
bool isRemembered(const QString &draftId);
void setRemembered(const QString &draftId, bool remembered);

// Permanently removes a draft (file + index entry), no trash involved.
void deleteDraft(const QString &draftId);

// Renames an untitled draft (one with no real filePath).
void renameDraft(const QString &draftId, const QString &newDefaultName);

// Moves a draft to the trash instead of deleting it outright.
void trashDraft(const QString &draftId);

// All trashed drafts, most recently deleted first.
QVector<TrashEntry> listTrash();

// Moves a trashed draft back into the active drafts store.
void restoreDraft(const QString &draftId);

// Permanently deletes a trashed draft.
void purgeDraft(const QString &draftId);

// Snapshots a tab's pre-save content, keeping only the most recent versions
// (see kMaxVersions in Session.cpp).
void saveVersion(const QString &draftId, const QString &content);

// Timestamps (newest first) of the saved versions kept for a tab.
QStringList listVersions(const QString &draftId);

// Moves the saved versions of one draft under another id (the newest
// kMaxVersions are kept) — used when a note adopts the id of an existing note
// for the same file.
void mergeVersions(const QString &fromId, const QString &toId);

QString readVersion(const QString &draftId, const QString &stamp);

// Persists the window size, screen position, sidebar-splitter position and
// display settings (word wrap, Markdown preview) across launches.
void saveWindowState(int width, int height, const QList<int> &splitterSizes, int x, int y, bool wordWrap,
                     bool markdownPreview);

WindowState loadWindowState();

} // namespace Session
