#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;

// Lists the saved versions kept for a tab (see Session::saveVersion), with a
// read-only preview and a way to restore one. Mirrors
// Python/version_history_dialog.py.
class VersionHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    VersionHistoryDialog(const QString &draftId, QWidget *parent = nullptr);

signals:
    void restoreRequested(const QString &content);

private slots:
    void showPreview(QListWidgetItem *current);
    void restoreSelected();

private:
    QString m_draftId;
    QListWidget *m_listWidget;
    QPlainTextEdit *m_preview;
    QPushButton *m_restoreBtn;
};
