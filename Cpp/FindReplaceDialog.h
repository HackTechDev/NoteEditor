#pragma once

#include <QDialog>
#include <QTextDocument>

class QCheckBox;
class QLineEdit;
class QPushButton;
class MainWindow;

// Non-modal find/replace panel. Mirrors Python/find_replace.py.
class FindReplaceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FindReplaceDialog(MainWindow *mainWindow);

    void showForFind();
    void showForReplace();

    bool findNext();
    bool findPrevious();

public slots:
    void replaceOne();
    void replaceAll();

private:
    bool find(bool backward);
    QTextDocument::FindFlags flags(bool backward) const;

    MainWindow *m_mainWindow;
    QLineEdit *m_findEdit;
    QLineEdit *m_replaceEdit;
    QCheckBox *m_caseCheck;
    QCheckBox *m_wholeWordsCheck;
};
