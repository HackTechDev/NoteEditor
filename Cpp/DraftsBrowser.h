#pragma once

#include "Session.h"

#include <QColor>
#include <QHash>
#include <QListWidget>
#include <QPixmap>
#include <QSet>
#include <QString>
#include <QVector>

// Tooltip text of a note (sidebar and tab): its file path; for a note without a
// file, where its draft is stored.
QString pathTooltip(const QString &filePath, const QString &label, const QString &draftId);

// Small filled push-pin glyph, used next to pinned notes (sidebar and tab).
QPixmap pinPixmap(int size = 14, const QColor &color = Qt::darkGray);

// Same push-pin, struck through (the "Détacher" toolbar icon).
QPixmap unpinPixmap(int size = 14, const QColor &color = Qt::darkGray);

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
    // Actions shared with the tab context menu: "close", "close_others",
    // "close_right", "close_all", "duplicate", "history", "toggle_pin".
    void actionRequested(const QString &action, const Session::DraftEntry &entry);
    // Fermeture de plusieurs notes sélectionnées d'un coup.
    void closeSelectedRequested(const QVector<Session::DraftEntry> &entries);

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
