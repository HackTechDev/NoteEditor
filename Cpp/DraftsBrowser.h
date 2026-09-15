#pragma once

#include "Session.h"

#include <QHash>
#include <QListWidget>
#include <QSet>

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

signals:
    void openRequested(const Session::DraftEntry &entry);
    void deleteRequested(const Session::DraftEntry &entry);

private slots:
    void emitOpen(QListWidgetItem *item);
    void showContextMenu(const QPoint &pos);

private:
    QHash<QString, Session::DraftEntry> m_entriesById;
};
