#pragma once

#include "Session.h"

#include <QHash>
#include <QListWidget>
#include <QSet>
#include <QString>

// Sidebar listing every known draft (open or closed). Mirrors
// Python/drafts_browser.py.
class DraftsBrowser : public QListWidget
{
    Q_OBJECT
public:
    explicit DraftsBrowser(QWidget *parent = nullptr);

    // Rebuilds the list from Session::listDrafts(). Entries whose id is in
    // openIds get a "(ouvert)" suffix instead of being hidden.
    void refresh(const QSet<QString> &openIds = {});

    void selectDraft(const QString &draftId);

    void setSortMode(const QString &mode); // "date" or "name"
    void setFilterText(const QString &text);

signals:
    void openRequested(const Session::DraftEntry &entry);
    void deleteRequested(const Session::DraftEntry &entry);
    void renameRequested(const Session::DraftEntry &entry);

private slots:
    void emitOpen(QListWidgetItem *item);
    void showContextMenu(const QPoint &pos);

private:
    static QString labelFor(const Session::DraftEntry &entry);

    QHash<QString, Session::DraftEntry> m_entriesById;
    QSet<QString> m_openIds;
    QString m_sortMode = "date";
    QString m_filterText;
};
