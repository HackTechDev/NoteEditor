#include "TrashDialog.h"
#include "Session.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString labelFor(const Session::TrashEntry &entry)
{
    if (!entry.filePath.isEmpty())
        return QFileInfo(entry.filePath).fileName();
    return !entry.defaultName.isEmpty() ? entry.defaultName : entry.id.left(8);
}

QString formatDeletedAt(const QString &iso)
{
    const QDateTime dt = QDateTime::fromString(iso, Qt::ISODate);
    return dt.isValid() ? dt.toString("dd/MM/yyyy HH:mm") : iso;
}

} // namespace

TrashDialog::TrashDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Corbeille");
    resize(420, 360);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_restoreBtn = new QPushButton("Restaurer", this);
    m_purgeBtn = new QPushButton("Supprimer définitivement", this);
    auto *closeBtn = new QPushButton("Fermer", this);

    connect(m_restoreBtn, &QPushButton::clicked, this, &TrashDialog::restoreSelected);
    connect(m_purgeBtn, &QPushButton::clicked, this, &TrashDialog::purgeSelected);
    connect(closeBtn, &QPushButton::clicked, this, &TrashDialog::close);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_listWidget);

    auto *buttonsRow = new QHBoxLayout();
    buttonsRow->addWidget(m_restoreBtn);
    buttonsRow->addWidget(m_purgeBtn);
    buttonsRow->addStretch();
    buttonsRow->addWidget(closeBtn);
    layout->addLayout(buttonsRow);

    refresh();
}

void TrashDialog::refresh()
{
    m_listWidget->clear();
    for (const Session::TrashEntry &entry : Session::listTrash()) {
        const QString label = QString("%1 — supprimé le %2").arg(labelFor(entry), formatDeletedAt(entry.deletedAt));
        auto *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, entry.id);
        m_listWidget->addItem(item);
    }
}

void TrashDialog::restoreSelected()
{
    const QList<QListWidgetItem *> items = m_listWidget->selectedItems();
    if (items.isEmpty())
        return;
    for (QListWidgetItem *item : items)
        Session::restoreDraft(item->data(Qt::UserRole).toString());
    refresh();
    emit restored();
}

void TrashDialog::purgeSelected()
{
    const QList<QListWidgetItem *> items = m_listWidget->selectedItems();
    if (items.isEmpty())
        return;

    QStringList ids;
    for (QListWidgetItem *item : items)
        ids << item->data(Qt::UserRole).toString();

    QString question;
    if (ids.size() == 1) {
        Session::TrashEntry entry;
        for (const Session::TrashEntry &e : Session::listTrash()) {
            if (e.id == ids.first()) {
                entry = e;
                break;
            }
        }
        question = QString("Supprimer définitivement « %1 » ? Cette action est irréversible.").arg(labelFor(entry));
    } else {
        question = QString("Supprimer définitivement ces %1 brouillons ? Cette action est irréversible.").arg(ids.size());
    }

    const auto result = QMessageBox::question(this, "Supprimer définitivement", question,
        QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) {
        for (const QString &id : ids)
            Session::purgeDraft(id);
        refresh();
    }
}
