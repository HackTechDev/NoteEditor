#include "DraftsBrowser.h"

#include <QApplication>
#include <QFileInfo>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QPolygon>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <algorithm>

namespace {

constexpr int kPinIconSize = 14;
constexpr int kPinIconMargin = 6;
constexpr int kPinnedRole = Qt::UserRole + 1; // bool, set in refresh()

// Dessine le nom de la note comme d'habitude, plus une petite punaise au bord
// droit visible de la ligne quand la note est épinglée.
class PinDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (!index.data(kPinnedRole).toBool()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        // fond (y compris sélection) sur toute la ligne, puis texte sans la zone de la punaise
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

        int right = opt.rect.right();
        if (auto *view = qobject_cast<const QAbstractItemView *>(widget))
            right = std::min(right, view->viewport()->width() - 1);

        QStyleOptionViewItem textOpt(opt);
        textOpt.rect = opt.rect.adjusted(0, 0, -(opt.rect.right() - right + kPinIconSize + 2 * kPinIconMargin), 0);
        QStyledItemDelegate::paint(painter, textOpt, index);

        const bool selected = opt.state & QStyle::State_Selected;
        const QColor color = selected ? opt.palette.highlightedText().color() : QColor(Qt::darkGray);
        const int x = right - kPinIconSize - kPinIconMargin + 1;
        const int y = opt.rect.top() + (opt.rect.height() - kPinIconSize) / 2;
        painter->drawPixmap(x, y, pinPixmap(kPinIconSize, color));
    }
};

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
    setItemDelegate(new PinDelegate(this));
    // Pas de défilement horizontal : la punaise est ancrée au bord droit
    // visible de la ligne. Les noms trop longs sont tronqués (« … »), le nom
    // complet reste dans l'infobulle.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTextElideMode(Qt::ElideRight);
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
        item->setData(kPinnedRole, entry.pinned);
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
