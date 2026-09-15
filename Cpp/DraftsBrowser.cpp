#include "DraftsBrowser.h"

#include <QFileInfo>
#include <QMenu>
#include <algorithm>

DraftsBrowser::DraftsBrowser(QWidget *parent)
    : QListWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested, this, &DraftsBrowser::showContextMenu);
    connect(this, &QListWidget::itemClicked, this, &DraftsBrowser::emitOpen);
}

QString DraftsBrowser::labelFor(const Session::DraftEntry &entry)
{
    if (!entry.filePath.isEmpty())
        return QFileInfo(entry.filePath).fileName();
    return !entry.defaultName.isEmpty() ? entry.defaultName : entry.id.left(8);
}

void DraftsBrowser::setSortMode(const QString &mode)
{
    m_sortMode = mode;
    refresh(m_openIds);
}

void DraftsBrowser::setFilterText(const QString &text)
{
    m_filterText = text;
    refresh(m_openIds);
}

void DraftsBrowser::refresh(const QSet<QString> &openIds)
{
    m_openIds = openIds;
    clear();
    m_entriesById.clear();

    QVector<Session::DraftEntry> entries = Session::listDrafts();
    if (m_sortMode == "name") {
        std::sort(entries.begin(), entries.end(), [](const Session::DraftEntry &a, const Session::DraftEntry &b) {
            return labelFor(a).compare(labelFor(b), Qt::CaseInsensitive) < 0;
        });
    }

    const QString needle = m_filterText.trimmed().toLower();
    for (const Session::DraftEntry &entry : entries) {
        const QString label = labelFor(entry);
        if (!needle.isEmpty() && !label.toLower().contains(needle))
            continue;

        QString display = label;
        if (m_openIds.contains(entry.id))
            display += " (ouvert)";

        auto *item = new QListWidgetItem(display);
        item->setToolTip(!entry.filePath.isEmpty() ? entry.filePath : label);
        item->setData(Qt::UserRole, entry.id);
        addItem(item);

        m_entriesById[entry.id] = entry;
    }
}

void DraftsBrowser::selectDraft(const QString &draftId)
{
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *it = item(i);
        if (it->data(Qt::UserRole).toString() == draftId) {
            setCurrentItem(it);
            return;
        }
    }
    setCurrentItem(nullptr);
}

void DraftsBrowser::emitOpen(QListWidgetItem *item)
{
    const QString id = item->data(Qt::UserRole).toString();
    if (m_entriesById.contains(id))
        emit openRequested(m_entriesById.value(id));
}

void DraftsBrowser::showContextMenu(const QPoint &pos)
{
    QListWidgetItem *it = itemAt(pos);
    if (!it)
        return;
    const QString id = it->data(Qt::UserRole).toString();
    if (!m_entriesById.contains(id))
        return;
    const Session::DraftEntry entry = m_entriesById.value(id);

    QMenu menu(this);
    QAction *renameAction = entry.filePath.isEmpty() ? menu.addAction("Renommer...") : nullptr;
    QAction *deleteAction = menu.addAction("Mettre à la corbeille");
    QAction *chosen = menu.exec(mapToGlobal(pos));
    if (chosen == deleteAction)
        emit deleteRequested(entry);
    else if (renameAction && chosen == renameAction)
        emit renameRequested(entry);
}
