#include "DraftsBrowser.h"

#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPolygon>
#include <algorithm>

namespace {

constexpr int kPinIconSize = 14;

// Icône à gauche du nom dans la liste : une punaise (grise, blanche quand la
// ligne est sélectionnée) pour une note épinglée, sinon un carré transparent de
// même taille, pour que tous les noms restent alignés.
QIcon rowIcon(bool pinned, const QColor &selectedColor)
{
    QIcon icon;
    if (pinned) {
        icon.addPixmap(pinPixmap(kPinIconSize), QIcon::Normal);
        icon.addPixmap(pinPixmap(kPinIconSize, selectedColor), QIcon::Selected);
    } else {
        QPixmap blank(kPinIconSize, kPinIconSize);
        blank.fill(Qt::transparent);
        icon.addPixmap(blank);
    }
    return icon;
}

} // namespace

QPixmap pinPixmap(int size, const QColor &color)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(size / 22.0, size / 22.0);
    QPen pen(color);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(color);
    painter.drawLine(6, 3, 16, 3); // tête
    painter.drawPolygon(QPolygon({QPoint(8, 3), QPoint(8, 9), QPoint(5, 13), QPoint(17, 13), QPoint(14, 9), QPoint(14, 3)}));
    painter.drawLine(11, 13, 11, 20); // aiguille
    painter.end();
    return pixmap;
}

DraftsBrowser::DraftsBrowser(QWidget *parent)
    : QListWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested, this, &DraftsBrowser::showContextMenu);
    connect(this, &QListWidget::itemDoubleClicked, this, &DraftsBrowser::emitOpen);
    setIconSize(QSize(kPinIconSize, kPinIconSize));
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
        item->setIcon(rowIcon(entry.pinned, palette().highlightedText().color()));
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
    const bool isOpen = m_openIds.contains(entry.id);
    const bool isPinned = entry.pinned;
    const bool hasHistory = !Session::listVersions(entry.id).isEmpty();

    QMenu menu(this);
    QAction *renameAction = entry.filePath.isEmpty() ? menu.addAction("Renommer...") : nullptr;
    menu.addSeparator();

    QAction *closeAction = menu.addAction("Fermer");
    closeAction->setEnabled(isOpen && !isPinned);
    QAction *closeOthersAction = menu.addAction("Fermer les autres");
    closeOthersAction->setEnabled(isOpen);
    QAction *closeRightAction = menu.addAction("Fermer à droite");
    closeRightAction->setEnabled(isOpen);
    QAction *closeAllAction = menu.addAction("Fermer tout");
    menu.addSeparator();

    QAction *pinAction = menu.addAction(isPinned ? "Détacher" : "Épingler");
    QAction *duplicateAction = menu.addAction("Dupliquer");
    QAction *historyAction = menu.addAction("Historique des versions...");
    historyAction->setEnabled(hasHistory);
    menu.addSeparator();

    QAction *deleteAction = menu.addAction("Mettre à la corbeille");
    deleteAction->setEnabled(!isPinned);

    QAction *chosen = menu.exec(mapToGlobal(pos));
    if (chosen == deleteAction)
        emit deleteRequested(entry);
    else if (renameAction && chosen == renameAction)
        emit renameRequested(entry);
    else if (chosen == closeAction)
        emit actionRequested("close", entry);
    else if (chosen == closeOthersAction)
        emit actionRequested("close_others", entry);
    else if (chosen == closeRightAction)
        emit actionRequested("close_right", entry);
    else if (chosen == closeAllAction)
        emit actionRequested("close_all", entry);
    else if (chosen == duplicateAction)
        emit actionRequested("duplicate", entry);
    else if (chosen == historyAction)
        emit actionRequested("history", entry);
    else if (chosen == pinAction)
        emit actionRequested("toggle_pin", entry);
}
