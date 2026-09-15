#include "FindReplaceDialog.h"
#include "Editor.h"
#include "MainWindow.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextCursor>

FindReplaceDialog::FindReplaceDialog(MainWindow *mainWindow)
    : QDialog(mainWindow)
    , m_mainWindow(mainWindow)
{
    setWindowTitle("Rechercher / Remplacer");
    setWindowFlag(Qt::Tool);

    m_findEdit = new QLineEdit(this);
    m_replaceEdit = new QLineEdit(this);
    m_caseCheck = new QCheckBox("Sensible à la casse", this);
    m_wholeWordsCheck = new QCheckBox("Mot entier", this);

    auto *findNextBtn = new QPushButton("Suivant", this);
    auto *findPrevBtn = new QPushButton("Précédent", this);
    auto *replaceBtn = new QPushButton("Remplacer", this);
    auto *replaceAllBtn = new QPushButton("Tout remplacer", this);
    auto *closeBtn = new QPushButton("Fermer", this);

    connect(findNextBtn, &QPushButton::clicked, this, [this] { findNext(); });
    connect(findPrevBtn, &QPushButton::clicked, this, [this] { findPrevious(); });
    connect(replaceBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceOne);
    connect(replaceAllBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
    connect(closeBtn, &QPushButton::clicked, this, &FindReplaceDialog::close);

    connect(m_findEdit, &QLineEdit::returnPressed, this, [this] { findNext(); });
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &FindReplaceDialog::replaceOne);

    auto *layout = new QGridLayout(this);
    layout->addWidget(new QLabel("Rechercher :", this), 0, 0);
    layout->addWidget(m_findEdit, 0, 1);
    layout->addWidget(new QLabel("Remplacer par :", this), 1, 0);
    layout->addWidget(m_replaceEdit, 1, 1);

    auto *optionsRow = new QHBoxLayout();
    optionsRow->addWidget(m_caseCheck);
    optionsRow->addWidget(m_wholeWordsCheck);
    layout->addLayout(optionsRow, 2, 0, 1, 2);

    auto *buttonsRow = new QHBoxLayout();
    buttonsRow->addWidget(findPrevBtn);
    buttonsRow->addWidget(findNextBtn);
    buttonsRow->addWidget(replaceBtn);
    buttonsRow->addWidget(replaceAllBtn);
    buttonsRow->addWidget(closeBtn);
    layout->addLayout(buttonsRow, 3, 0, 1, 2);

    setLayout(layout);
}

void FindReplaceDialog::showForFind()
{
    show();
    raise();
    activateWindow();
    m_findEdit->setFocus();
    m_findEdit->selectAll();
}

void FindReplaceDialog::showForReplace()
{
    showForFind();
    m_replaceEdit->setFocus();
}

QTextDocument::FindFlags FindReplaceDialog::flags(bool backward) const
{
    QTextDocument::FindFlags f;
    if (backward)
        f |= QTextDocument::FindBackward;
    if (m_caseCheck->isChecked())
        f |= QTextDocument::FindCaseSensitively;
    if (m_wholeWordsCheck->isChecked())
        f |= QTextDocument::FindWholeWords;
    return f;
}

bool FindReplaceDialog::findNext()
{
    return find(false);
}

bool FindReplaceDialog::findPrevious()
{
    return find(true);
}

bool FindReplaceDialog::find(bool backward)
{
    Editor *editor = m_mainWindow->currentEditor();
    const QString text = m_findEdit->text();
    if (!editor || text.isEmpty())
        return false;

    const QTextDocument::FindFlags f = flags(backward);
    bool found = editor->find(text, f);
    if (!found) {
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
        editor->setTextCursor(cursor);
        found = editor->find(text, f);
    }
    if (!found)
        QMessageBox::information(this, "Rechercher", QString("« %1 » est introuvable.").arg(text));
    return found;
}

void FindReplaceDialog::replaceOne()
{
    Editor *editor = m_mainWindow->currentEditor();
    const QString findText = m_findEdit->text();
    if (!editor || findText.isEmpty())
        return;

    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection()) {
        const QString selected = cursor.selectedText();
        const bool matches = m_caseCheck->isChecked()
            ? selected == findText
            : selected.compare(findText, Qt::CaseInsensitive) == 0;
        if (matches) {
            cursor.insertText(m_replaceEdit->text());
            editor->setTextCursor(cursor);
        }
    }
    findNext();
}

void FindReplaceDialog::replaceAll()
{
    Editor *editor = m_mainWindow->currentEditor();
    const QString findText = m_findEdit->text();
    if (!editor || findText.isEmpty())
        return;
    const QString replaceText = m_replaceEdit->text();
    const QTextDocument::FindFlags f = flags(false);

    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    editor->setTextCursor(cursor);

    QTextCursor editCursor = editor->textCursor();
    editCursor.beginEditBlock();
    int count = 0;
    while (editor->find(findText, f)) {
        QTextCursor matchCursor = editor->textCursor();
        matchCursor.insertText(replaceText);
        editor->setTextCursor(matchCursor);
        ++count;
    }
    editCursor.endEditBlock();

    QMessageBox::information(this, "Tout remplacer", QString("%1 occurrence(s) remplacée(s).").arg(count));
}
