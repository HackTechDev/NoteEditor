#include "DraftsBrowser.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPolygon>
#include <algorithm>

const char *const kRememberTooltip =
    "Une note vide sans fichier n'est pas proposée dans « Notes fermées récemment » ; "
    "cochez pour la mémoriser quand même. Sans effet si elle contient du texte : "
    "elle est alors toujours mémorisée.";

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

QString pathTooltip(const QString &filePath, const QString &label, const QString &draftId)
{
    if (!filePath.isEmpty())
        return filePath;
    return QString("%1\nPas encore enregistrée dans un fichier\nBrouillon : %2")
        .arg(label, Session::draftsDir() + "/" + draftId + ".txt");
}

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

QPixmap unpinPixmap(int size, const QColor &color)
{
    // La punaise, puis un trait diagonal dont le contour est effacé dans la
    // punaise pour rester lisible à petite taille.
    QPixmap pixmap = pinPixmap(size, color);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(size / 22.0, size / 22.0);
    QPen knockout(color);
    knockout.setWidth(5);
    knockout.setCapStyle(Qt::RoundCap);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setPen(knockout);
    painter.drawLine(4, 3, 18, 19);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    QPen stroke(color);
    stroke.setWidth(2);
    stroke.setCapStyle(Qt::RoundCap);
    painter.setPen(stroke);
    painter.drawLine(4, 3, 18, 19);
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
    setSelectionMode(QAbstractItemView::ExtendedSelection);
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
    if (needle.isEmpty())
        m_contentCache.clear(); // rien à garder en mémoire hors recherche
    for (const Session::DraftEntry &entry : entries) {
        const QString label = labelFor(entry);
        if (!needle.isEmpty() && !label.toLower().contains(needle) && !contentMatches(entry.id, needle))
            continue;
        if (!m_openIds.contains(entry.id) && Session::isExternalFile(entry.filePath))
            continue; // un fichier extérieur à ~/.noteeditor n'est listé que tant qu'il est ouvert

        QString display = label;
        if (m_openIds.contains(entry.id))
            display += " (ouvert)";

        auto *item = new QListWidgetItem(display);
        item->setToolTip(pathTooltip(entry.filePath, label, entry.id));
        item->setData(Qt::UserRole, entry.id);
        item->setIcon(rowIcon(entry.pinned, palette().highlightedText().color()));
        addItem(item);

        m_entriesById[entry.id] = entry;
    }
}

// Le texte de la note (son brouillon) contient-il la recherche ?
bool DraftsBrowser::contentMatches(const QString &draftId, const QString &needle)
{
    const QString path = Session::draftsDir() + "/" + draftId + ".txt";
    const QFileInfo info(path);
    if (!info.exists())
        return false;
    const QString stamp = QString::number(info.lastModified().toMSecsSinceEpoch()) + ":" + QString::number(info.size());
    auto it = m_contentCache.find(draftId);
    if (it == m_contentCache.end() || it->stamp != stamp) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return false;
        m_contentCache[draftId] = {stamp, QString::fromUtf8(f.readAll()).toLower()};
        it = m_contentCache.find(draftId);
    }
    return it->textLower.contains(needle);
}

void DraftsBrowser::selectIds(const QStringList &draftIds)
{
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *it = item(i);
        if (draftIds.contains(it->data(Qt::UserRole).toString()))
            it->setSelected(true);
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

    const QList<QListWidgetItem *> selectedItems = this->selectedItems();
    if (selectedItems.size() > 1 && it->isSelected()) {
        QVector<Session::DraftEntry> selected;
        bool anyPinned = false, anyUnpinned = false, anyClosable = false;
        for (QListWidgetItem *s : selectedItems) {
            const QString sid = s->data(Qt::UserRole).toString();
            if (!m_entriesById.contains(sid))
                continue;
            const Session::DraftEntry e = m_entriesById.value(sid);
            selected.append(e);
            anyPinned = anyPinned || e.pinned;
            anyUnpinned = anyUnpinned || !e.pinned;
            anyClosable = anyClosable || (m_openIds.contains(e.id) && !e.pinned);
        }
        const int n = selected.size();
        QMenu multi(this);
        QAction *closeSelected = multi.addAction(QString("Fermer les %1 notes sélectionnées").arg(n));
        closeSelected->setEnabled(anyClosable);
        multi.addSeparator();
        QAction *pinSelected = multi.addAction(QString("Épingler les %1 notes sélectionnées").arg(n));
        pinSelected->setEnabled(anyUnpinned);
        QAction *unpinSelected = multi.addAction(QString("Détacher les %1 notes sélectionnées").arg(n));
        unpinSelected->setEnabled(anyPinned);
        multi.addSeparator();
        // les notes épinglées ne peuvent pas être mises à la corbeille : elles sont ignorées
        QAction *trashSelected = multi.addAction(QString("Mettre les %1 notes sélectionnées à la corbeille").arg(n));
        trashSelected->setEnabled(anyUnpinned);
        QAction *chosen = multi.exec(mapToGlobal(pos));
        if (chosen == closeSelected)
            emit bulkActionRequested("close", selected);
        else if (chosen == pinSelected)
            emit bulkActionRequested("pin", selected);
        else if (chosen == unpinSelected)
            emit bulkActionRequested("unpin", selected);
        else if (chosen == trashSelected)
            emit bulkActionRequested("trash", selected);
        return;
    }

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
    QAction *rememberAction = menu.addAction("Mémoriser à la fermeture");
    rememberAction->setCheckable(true);
    rememberAction->setChecked(entry.remember);
    rememberAction->setToolTip(kRememberTooltip);
    menu.setToolTipsVisible(true);
    QAction *duplicateAction = menu.addAction("Dupliquer");
    QAction *historyAction = menu.addAction("Historique des versions...");
    historyAction->setEnabled(hasHistory);
    menu.addSeparator();

    // le nom marche pour toute note (fichier ou nom de la note) ; le chemin
    // n'existe que pour une note liée à un fichier
    QAction *copyNameAction = menu.addAction("Copier le nom du fichier");
    QAction *copyPathAction = menu.addAction("Copier le chemin complet du fichier");
    copyPathAction->setEnabled(!entry.filePath.isEmpty());
    QAction *openFolderAction = menu.addAction("Ouvrir le dossier du fichier");
    openFolderAction->setEnabled(!entry.filePath.isEmpty());
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
    else if (chosen == rememberAction)
        Session::setRemembered(entry.id, !entry.remember);
    else if (chosen == copyNameAction)
        QApplication::clipboard()->setText(labelFor(entry));
    else if (chosen == copyPathAction)
        QApplication::clipboard()->setText(entry.filePath);
    else if (chosen == openFolderAction)
        emit actionRequested("open_folder", entry);
}
