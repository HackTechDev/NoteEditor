#include "VersionHistoryDialog.h"
#include "Session.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString formatStamp(const QString &stamp)
{
    const QDateTime dt = QDateTime::fromString(stamp, "yyyyMMddTHHmmsszzz");
    return dt.isValid() ? dt.toString("dd/MM/yyyy HH:mm:ss") : stamp;
}

} // namespace

VersionHistoryDialog::VersionHistoryDialog(const QString &draftId, QWidget *parent)
    : QDialog(parent)
    , m_draftId(draftId)
{
    setWindowTitle("Historique des versions");
    resize(600, 420);

    m_listWidget = new QListWidget(this);
    connect(m_listWidget, &QListWidget::currentItemChanged, this, &VersionHistoryDialog::showPreview);

    m_preview = new QPlainTextEdit(this);
    m_preview->setReadOnly(true);

    m_restoreBtn = new QPushButton("Restaurer cette version", this);
    auto *closeBtn = new QPushButton("Fermer", this);
    connect(m_restoreBtn, &QPushButton::clicked, this, &VersionHistoryDialog::restoreSelected);
    connect(closeBtn, &QPushButton::clicked, this, &VersionHistoryDialog::close);

    auto *contentRow = new QHBoxLayout();
    contentRow->addWidget(m_listWidget, 1);
    contentRow->addWidget(m_preview, 2);

    auto *buttonsRow = new QHBoxLayout();
    buttonsRow->addStretch();
    buttonsRow->addWidget(m_restoreBtn);
    buttonsRow->addWidget(closeBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(contentRow);
    layout->addLayout(buttonsRow);

    for (const QString &stamp : Session::listVersions(draftId)) {
        auto *item = new QListWidgetItem(formatStamp(stamp));
        item->setData(Qt::UserRole, stamp);
        m_listWidget->addItem(item);
    }
    if (m_listWidget->count() > 0)
        m_listWidget->setCurrentRow(0);
}

void VersionHistoryDialog::showPreview(QListWidgetItem *current)
{
    if (!current) {
        m_preview->setPlainText(QString());
        return;
    }
    m_preview->setPlainText(Session::readVersion(m_draftId, current->data(Qt::UserRole).toString()));
}

void VersionHistoryDialog::restoreSelected()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item)
        return;
    emit restoreRequested(Session::readVersion(m_draftId, item->data(Qt::UserRole).toString()));
    close();
}
