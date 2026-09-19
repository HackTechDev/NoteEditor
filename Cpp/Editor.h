#pragma once

#include "Highlighters.h"

#include <QPlainTextEdit>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <QWidget>

class QPaintEvent;
class QResizeEvent;
class Editor;

constexpr int kAutosaveDelayMs = 1500;

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
    bool pinned = false; // mirrors index.json, see Session::isPinned()
    qint64 diskMTime = -1; // -1 == unknown / not tracked yet

signals:
    // Fired ~1.5s after the last keystroke, while still unsaved. MainWindow
    // uses this to archive the draft continuously, not just on tab
    // creation/close/quit.
    void autosaveRequested();

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
    QTimer *m_autosaveTimer;
};
