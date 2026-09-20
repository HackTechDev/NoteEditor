#include "Editor.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QScrollBar>
#include <QShowEvent>
#include <QTextBlock>
#include <QTextEdit>

LineNumberArea::LineNumberArea(Editor *editor)
    : QWidget(editor)
    , m_editor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}

Editor::Editor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
{
    sessionId = QUuid::createUuid().toString(QUuid::Id128);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(11);
    setFont(font);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));

    connect(this, &QPlainTextEdit::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::highlightCurrentLine);

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setSingleShot(true);
    m_autosaveTimer->setInterval(kAutosaveDelayMs);
    connect(m_autosaveTimer, &QTimer::timeout, this, &Editor::autosaveRequested);
    connect(document(), &QTextDocument::contentsChanged, m_autosaveTimer, qOverload<>(&QTimer::start));

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

int Editor::lineNumberAreaWidth() const
{
    int digits = QString::number(qMax(1, blockCount())).length();
    return 12 + fontMetrics().horizontalAdvance('9') * digits;
}

void Editor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Editor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::restoreView(int cursor, int scroll)
{
    QTextCursor c = textCursor();
    c.setPosition(qBound(0, cursor, qMax(0, document()->characterCount() - 1)));
    setTextCursor(c);
    m_pendingScroll = scroll > 0 ? scroll : -1;
}

void Editor::viewState(int *cursor, int *scroll) const
{
    *cursor = textCursor().position();
    *scroll = m_pendingScroll >= 0 ? m_pendingScroll : verticalScrollBar()->value();
}

void Editor::showEvent(QShowEvent *event)
{
    QPlainTextEdit::showEvent(event);
    if (m_pendingScroll >= 0) {
        QTimer::singleShot(0, this, [this] {
            if (m_pendingScroll >= 0) {
                const int value = m_pendingScroll;
                m_pendingScroll = -1;
                verticalScrollBar()->setValue(value);
            }
        });
    }
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#f0f0f0"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    painter.setPen(QColor("#8a8a8a"));
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top, m_lineNumberArea->width() - 6, fontMetrics().height(),
                              Qt::AlignRight, QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::highlightCurrentLine()
{
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor("#e8f2ff"));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    setExtraSelections({selection});
}

void Editor::setFilePath(const QString &path)
{
    filePath = path;
    const HighlighterKind kind = highlighterKindFor(path);
    if (kind == m_highlighterKind)
        return;

    if (m_highlighter) {
        m_highlighter->setDocument(nullptr);
        delete m_highlighter;
        m_highlighter = nullptr;
    }
    m_highlighterKind = kind;
    m_highlighter = createHighlighter(kind, document());
}
