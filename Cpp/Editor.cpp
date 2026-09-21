#include "Editor.h"

#include <QFont>
#include <QJsonArray>
#include <QKeyEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QScrollBar>
#include <QShowEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>

namespace {

// Historique annuler/rétablir mémorisé d'un lancement à l'autre : nombre maximal
// d'étapes de chaque côté du point courant, et volume total de texte conservé.
constexpr int kHistoryMaxSteps = 200;
constexpr int kHistoryMaxChars = 1000000;
// Au-delà de cette taille de note, l'historique n'est plus suivi (chaque frappe
// coûterait une comparaison du texte entier) ; annuler/rétablir continue de marcher.
constexpr int kHistoryTrackMaxChars = 300000;

struct Step {
    int start;
    QString removed;
    QString added;
};

// Étape qui transforme `oldText` en `newText` (préfixe et suffixe communs écartés,
// sans jamais couper une paire de substitution UTF-16).
Step diffTexts(const QString &oldText, const QString &newText)
{
    const int limit = qMin(oldText.size(), newText.size());
    int start = 0;
    while (start < limit && oldText[start] == newText[start])
        ++start;
    if (start > 0 && oldText[start - 1].isHighSurrogate())
        --start;
    int end = 0;
    while (end < limit - start && oldText[oldText.size() - 1 - end] == newText[newText.size() - 1 - end])
        ++end;
    if (end > 0 && oldText[oldText.size() - end].isLowSurrogate())
        --end;
    return {start, oldText.mid(start, oldText.size() - end - start), newText.mid(start, newText.size() - end - start)};
}

QString applyStep(const QString &text, int start, const QString &removed, const QString &added)
{
    return text.left(start) + added + text.mid(start + removed.size());
}

QString revertStep(const QString &text, int start, const QString &removed, const QString &added)
{
    return text.left(start) + removed + text.mid(start + added.size());
}

} // namespace

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
    connect(document(), &QTextDocument::undoCommandAdded, this, [this] { m_histNewStep = true; });
    connect(document(), &QTextDocument::contentsChange, this, [this](int, int, int) { trackHistory(); });

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

// Mode commande (Échap) : le curseur devient un bloc et la frappe n'insère plus de texte ;
// Échap de nouveau, ou une commande comme `o`, ramène au mode insertion.
void Editor::setCommandMode(bool enabled)
{
    if (enabled == m_commandMode)
        return;
    m_commandMode = enabled;
    setCursorWidth(enabled ? fontMetrics().horizontalAdvance(' ') : 1);
    emit modeChanged();
}

// Commande `o` : insère une ligne vide sous la ligne du curseur et s'y place.
void Editor::openLineBelow()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertBlock();
    setTextCursor(cursor);
}

// Commande `Maj+J` : joint la ligne du curseur à la suivante. Le saut de ligne et
// l'indentation de la suivante disparaissent, remplacés par une seule espace (sauf si l'une
// des deux lignes est vide ou si la ligne courante finit déjà par une espace) ; le curseur se
// place sur le raccord. Rien sur la dernière ligne.
void Editor::joinNextLine()
{
    QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QTextBlock following = block.next();
    if (!following.isValid())
        return;
    const QString text = following.text();
    int lead = 0;
    while (lead < text.size() && (text[lead] == ' ' || text[lead] == '\t'))
        ++lead;
    const QString rest = text.mid(lead);
    const QString current = block.text();
    const bool separator = !current.isEmpty() && !current.back().isSpace() && !rest.isEmpty() && !rest.startsWith(')');
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::EndOfBlock);
    const int join = cursor.position();
    cursor.setPosition(following.position() + lead, QTextCursor::KeepAnchor);
    cursor.insertText(separator ? " " : "");
    cursor.endEditBlock();
    cursor.setPosition(join);
    setTextCursor(cursor);
}

// Tab : décale de 4 espaces vers la droite les lignes touchées par la sélection ;
// Maj+Tab : les décale vers la gauche (jusqu'à 4 espaces, ou une tabulation, en moins).
void Editor::keyPressEvent(QKeyEvent *event)
{
    const Qt::KeyboardModifiers mods = event->modifiers();
    const bool plain = !(mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
    const bool shift = mods & Qt::ShiftModifier;
    const int key = event->key();
    if (plain && key == Qt::Key_Escape) {
        setCommandMode(!m_commandMode);
        event->accept();
        return;
    }
    if (m_commandMode && plain) {
        const bool upper = shift || (!event->text().isEmpty() && event->text()[0].isUpper()); // Maj, ou majuscules verrouillées
        if (key == Qt::Key_O && !upper) {
            openLineBelow();
            setCommandMode(false);
            event->accept();
            return;
        }
        if (event->text() == "0" || event->text() == "$") { // début / fin de la ligne (logique, même avec le retour à la ligne)
            QTextCursor cursor = textCursor();
            cursor.movePosition(event->text() == "0" ? QTextCursor::StartOfBlock : QTextCursor::EndOfBlock);
            setTextCursor(cursor);
            event->accept();
            return;
        }
        if (key == Qt::Key_J && upper) {
            joinNextLine();
            event->accept();
            return;
        }
        if (!event->text().isEmpty() || key == Qt::Key_Delete || key == Qt::Key_Insert
            || key == Qt::Key_Tab || key == Qt::Key_Backtab) {
            event->accept(); // pas de saisie en mode commande ; les flèches, Début, Fin… passent
            return;
        }
    }
    if (plain && (key == Qt::Key_Backtab || (shift && key == Qt::Key_Tab))) {
        shiftLines(false);
        event->accept();
    } else if (plain && !shift && key == Qt::Key_Tab && textCursor().hasSelection()) {
        shiftLines(true);
        event->accept();
    } else {
        QPlainTextEdit::keyPressEvent(event);
    }
}

// Une seule étape d'annulation ; la sélection suit le texte.
void Editor::shiftLines(bool indent)
{
    static const QString kIndent = "    ";
    QTextDocument *doc = document();
    QTextCursor cursor = textCursor();
    const int anchor = cursor.anchor(), position = cursor.position();
    const int start = qMin(anchor, position), end = qMax(anchor, position);
    QTextBlock block = doc->findBlock(start);
    QTextBlock last = doc->findBlock(end);
    if (end > start && end == last.position())
        last = last.previous(); // la ligne où la sélection ne fait que commencer n'est pas touchée

    struct Change { int at; int delta; }; // début de ligne avant modification, variation de longueur
    QVector<Change> changes;
    int applied = 0;
    cursor.beginEditBlock();
    QTextCursor edit(doc);
    while (block.isValid()) {
        const QString text = block.text();
        int delta = 0;
        if (indent) {
            delta = text.isEmpty() ? 0 : kIndent.size(); // une ligne vide reste vide
            if (delta) {
                edit.setPosition(block.position());
                edit.insertText(kIndent);
            }
        } else {
            if (text.startsWith('\t')) {
                delta = -1;
            } else {
                int spaces = 0;
                while (spaces < text.size() && text[spaces] == ' ')
                    ++spaces;
                delta = -qMin(int(kIndent.size()), spaces);
            }
            if (delta) {
                edit.setPosition(block.position());
                edit.setPosition(block.position() - delta, QTextCursor::KeepAnchor);
                edit.removeSelectedText();
            }
        }
        if (delta) {
            changes.append({block.position() - applied, delta});
            applied += delta;
        }
        if (block == last)
            break;
        block = block.next();
    }
    cursor.endEditBlock();

    auto moved = [&changes](int pos, bool isStart) {
        int shift = 0;
        for (const Change &c : changes) {
            if (c.delta > 0) {
                if (pos > c.at || (pos == c.at && !isStart))
                    shift += c.delta;
            } else if (pos >= c.at - c.delta) {
                shift += c.delta;
            } else if (pos > c.at) {
                shift -= pos - c.at;
            }
        }
        return pos + shift;
    };
    QTextCursor result = textCursor();
    result.setPosition(moved(anchor, anchor == start));
    result.setPosition(moved(position, position == start), QTextCursor::KeepAnchor);
    setTextCursor(result);
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::restoreView(int cursor, int scroll, int anchor)
{
    const int last = qMax(0, document()->characterCount() - 1);
    QTextCursor c = textCursor();
    if (anchor >= 0) {
        c.setPosition(qBound(0, anchor, last));
        c.setPosition(qBound(0, cursor, last), QTextCursor::KeepAnchor);
    } else {
        c.setPosition(qBound(0, cursor, last));
    }
    setTextCursor(c);
    m_pendingScroll = scroll > 0 ? scroll : -1;
}

void Editor::viewState(int *cursor, int *scroll, int *anchor) const
{
    const QTextCursor c = textCursor();
    *cursor = c.position();
    *anchor = c.anchor();
    *scroll = m_pendingScroll >= 0 ? m_pendingScroll : verticalScrollBar()->value();
}

void Editor::trackHistory()
{
    if (m_histBusy)
        return;
    const bool newStep = m_histNewStep;
    m_histNewStep = false;
    QTextDocument *doc = document();
    if (doc->characterCount() > kHistoryTrackMaxChars) {
        m_histSteps.clear();
        m_histPos = 0;
        m_histText.clear();
        return;
    }
    const QString text = toPlainText();
    if (!doc->isUndoAvailable() && !doc->isRedoAvailable()) {
        // pile vidée (setPlainText : ouverture, rechargement, version restaurée)
        m_histSteps.clear();
        m_histPos = 0;
        m_histText = text;
        return;
    }
    if (!newStep) {
        if (m_histPos > 0) {
            const HistoryStep &prev = m_histSteps[m_histPos - 1];
            const QString before = revertStep(m_histText, prev.start, prev.removed, prev.added);
            if (text == before) { // annuler
                --m_histPos;
                m_histText = text;
                return;
            }
        }
        if (m_histPos < m_histSteps.size()) {
            const HistoryStep &next = m_histSteps[m_histPos];
            if (text == applyStep(m_histText, next.start, next.removed, next.added)) { // rétablir
                ++m_histPos;
                m_histText = text;
                return;
            }
        }
        if (m_histPos > 0) {
            // frappe fusionnée par Qt dans la dernière étape
            HistoryStep &last = m_histSteps[m_histPos - 1];
            const QString before = revertStep(m_histText, last.start, last.removed, last.added);
            const Step step = diffTexts(before, text);
            last = {step.start, step.removed, step.added};
            m_histText = text;
            return;
        }
    }
    m_histSteps.resize(m_histPos);
    const Step step = diffTexts(m_histText, text);
    m_histSteps.append({step.start, step.removed, step.added});
    m_histPos = m_histSteps.size();
    m_histText = text;
}

QJsonObject Editor::historyState() const
{
    if (m_histSteps.isEmpty())
        return QJsonObject();
    auto cost = [](const HistoryStep &s) { return s.removed.size() + s.added.size(); };
    const int firstBefore = qMax(0, m_histPos - kHistoryMaxSteps);
    const int endAfter = qMin(int(m_histSteps.size()), m_histPos + kHistoryMaxSteps);
    int begin = firstBefore, end = endAfter;
    int total = 0;
    for (int i = begin; i < end; ++i)
        total += cost(m_histSteps[i]);
    while (begin < m_histPos && total > kHistoryMaxChars)
        total -= cost(m_histSteps[begin++]);
    while (end > m_histPos && total > kHistoryMaxChars)
        total -= cost(m_histSteps[--end]);
    if (begin == end)
        return QJsonObject();
    // les positions se rapportent au texte avant chaque étape
    QString text = m_histText;
    for (int i = m_histPos - 1; i >= begin; --i)
        text = revertStep(text, m_histSteps[i].start, m_histSteps[i].removed, m_histSteps[i].added);
    QJsonArray steps;
    for (int i = begin; i < end; ++i) {
        const HistoryStep &s = m_histSteps[i];
        steps.append(QJsonObject{{"at", s.start}, {"del", s.removed}, {"ins", s.added}});
        text = applyStep(text, s.start, s.removed, s.added);
    }
    return QJsonObject{{"pos", m_histPos - begin}, {"length", int(m_histText.size())}, {"steps", steps}};
}

bool Editor::restoreHistory(const QJsonObject &data)
{
    const QJsonArray raw = data.value("steps").toArray();
    const int pos = data.value("pos").toInt(-1);
    const int length = data.value("length").toInt(-1);
    const QString current = toPlainText();
    if (raw.isEmpty() || pos < 0 || pos > raw.size() || length != current.size())
        return false;
    QVector<HistoryStep> steps(raw.size());
    for (int k = 0; k < raw.size(); ++k) {
        const QJsonObject o = raw[k].toObject();
        if (!o.contains("at") || !o.value("del").isString() || !o.value("ins").isString())
            return false;
        steps[k] = {o.value("at").toInt(-1), o.value("del").toString(), o.value("ins").toString()};
        if (steps[k].start < 0)
            return false;
    }
    // de retour au texte de départ : chaque étape à rebours doit retomber juste
    QString text = current;
    for (int k = pos - 1; k >= 0; --k) {
        const HistoryStep &s = steps[k];
        if (text.mid(s.start, s.added.size()) != s.added || s.start + s.added.size() > text.size())
            return false;
        text = revertStep(text, s.start, s.removed, s.added);
    }
    const QString base = text;
    text = current;
    for (int k = pos; k < steps.size(); ++k) {
        const HistoryStep &s = steps[k];
        if (text.mid(s.start, s.removed.size()) != s.removed || s.start + s.removed.size() > text.size())
            return false;
        text = applyStep(text, s.start, s.removed, s.added);
    }
    const bool modified = document()->isModified();
    m_histBusy = true;
    setPlainText(base);
    QTextCursor cursor(document());
    for (const HistoryStep &s : steps) {
        cursor.beginEditBlock();
        cursor.setPosition(s.start);
        cursor.setPosition(s.start + s.removed.size(), QTextCursor::KeepAnchor);
        cursor.insertText(s.added);
        cursor.endEditBlock();
    }
    for (int k = pos; k < steps.size(); ++k)
        document()->undo();
    const bool ok = toPlainText() == current;
    if (!ok)
        setPlainText(current);
    m_histBusy = false;
    m_histNewStep = false;
    document()->setModified(modified);
    m_histSteps = ok ? steps : QVector<HistoryStep>();
    m_histPos = ok ? pos : 0;
    m_histText = current;
    return ok;
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
