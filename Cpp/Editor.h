#pragma once

#include "Highlighters.h"

#include <QPlainTextEdit>
#include <QString>
#include <QUuid>
#include <QWidget>

class QPaintEvent;
class QResizeEvent;
class Editor;

// The line-number gutter painted to the left of an Editor.
class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    explicit LineNumberArea(Editor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Editor *m_editor;
};

// One per tab. Owns the line-number gutter, current-line highlight, and a
// stable session_id used throughout the persistence layer (Session.h).
class Editor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);

    void setFilePath(const QString &path);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    QString filePath;    // empty == no associated file
    QString defaultName; // empty == not applicable (has filePath instead)
    QString sessionId;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    LineNumberArea *m_lineNumberArea;
    QSyntaxHighlighter *m_highlighter = nullptr;
    HighlighterKind m_highlighterKind = HighlighterKind::None;
};
