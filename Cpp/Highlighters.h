#pragma once

#include <QRegularExpression>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

// Mirrors Python/highlighters.py: one QSyntaxHighlighter subclass per
// supported language, selected by file extension.

class PythonHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit PythonHighlighter(QTextDocument *document);

protected:
    void highlightBlock(const QString &text) override;

private:
    void highlightTriple(const QString &text, const QRegularExpression &pattern, int stateId);

    QVector<QPair<QRegularExpression, QTextCharFormat>> m_rules;
    QTextCharFormat m_tripleFormat;
    QRegularExpression m_tripleSingle;
    QRegularExpression m_tripleDouble;
};

class JsonHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit JsonHighlighter(QTextDocument *document);

protected:
    void highlightBlock(const QString &text) override;

private:
    QVector<QPair<QRegularExpression, QTextCharFormat>> m_rules;
};

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit MarkdownHighlighter(QTextDocument *document);

protected:
    void highlightBlock(const QString &text) override;

private:
    QVector<QPair<QRegularExpression, QTextCharFormat>> m_rules;
};

enum class HighlighterKind { None, Python, Json, Markdown };

// Which highlighter (if any) applies to a given file path, based on its
// extension. Used by Editor::setFilePath() to decide whether the current
// highlighter needs to be swapped.
HighlighterKind highlighterKindFor(const QString &filePath);

// Instantiates the right QSyntaxHighlighter subclass for a kind (nullptr for
// HighlighterKind::None), attached to the given document.
QSyntaxHighlighter *createHighlighter(HighlighterKind kind, QTextDocument *document);
