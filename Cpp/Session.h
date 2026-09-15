#pragma once

#include <QString>
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
};

// Metadata only (no content), as read back from index.json / session.json
// for the drafts sidebar.
struct DraftEntry {
    QString id;
    QString filePath;
    QString defaultName;
    bool modified = true;
    qint64 mtimeMs = 0;
};

QString configDir();
QString draftsDir();
QString docsDir();

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
void deleteDraft(const QString &draftId);

} // namespace Session
