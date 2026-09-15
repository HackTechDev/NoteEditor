#pragma once

#include <QDialog>

class QListWidget;
class QPushButton;

// Lets the user restore or permanently purge trashed drafts. Mirrors
// Python/trash_dialog.py.
class TrashDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TrashDialog(QWidget *parent = nullptr);

    void refresh();

signals:
    void restored();

private slots:
    void restoreSelected();
    void purgeSelected();

private:
    QListWidget *m_listWidget;
    QPushButton *m_restoreBtn;
    QPushButton *m_purgeBtn;
};
