#include "DraftsBrowser.h"

#include <QFileInfo>
#include <QMenu>

DraftsBrowser::DraftsBrowser(QWidget *parent)
    : QListWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested, this, &DraftsBrowser::showContextMenu);
    connect(this, &QListWidget::itemClicked, this, &DraftsBrowser::emitOpen);
}

void DraftsBrowser::refresh(const QSet<QString> &openIds)
{
    clear();
    m_entriesById.clear();

    for (const Session::DraftEntry &entry : Session::listDrafts()) {
        QString label = !entry.filePath.isEmpty()
            ? QFileInfo(entry.filePath).fileName()
            : (!entry.defaultName.isEmpty() ? entry.defaultName : entry.id.left(8));
        if (openIds.contains(entry.id))
            label += " (ouvert)";

        auto *item = new QListWidgetItem(label);
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

    QMenu menu(this);
    QAction *deleteAction = menu.addAction("Supprimer le brouillon");
    QAction *chosen = menu.exec(mapToGlobal(pos));
    if (chosen == deleteAction) {
        const QString id = it->data(Qt::UserRole).toString();
        if (m_entriesById.contains(id))
            emit deleteRequested(m_entriesById.value(id));
    }
}
