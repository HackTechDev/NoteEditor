#include "Highlighters.h"

#include <QFileInfo>
#include <QFont>

namespace {

QTextCharFormat fmt(const QString &color, bool bold = false, bool italic = false)
{
    QTextCharFormat f;
    f.setForeground(QColor(color));
    if (bold)
        f.setFontWeight(QFont::Bold);
    if (italic)
        f.setFontItalic(true);
    return f;
}

// Python 3's keyword.kwlist.
const QStringList kPythonKeywords = {
    "False",  "None",     "True",  "and",    "as",       "assert", "async",  "await",
    "break",  "class",    "continue", "def",  "del",      "elif",   "else",   "except",
    "finally", "for",     "from",  "global", "if",       "import", "in",     "is",
    "lambda", "nonlocal", "not",   "or",     "pass",     "raise",  "return", "try",
    "while",  "with",     "yield",
};

const QStringList kPythonBuiltins = {
    "self", "cls", "None", "True", "False", "print", "len", "range",
    "int", "str", "float", "list", "dict", "set", "tuple",
};

} // namespace

PythonHighlighter::PythonHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    m_rules.append({QRegularExpression("\\b(" + kPythonKeywords.join('|') + ")\\b"), fmt("#af00db", true)});
    m_rules.append({QRegularExpression("\\b(" + kPythonBuiltins.join('|') + ")\\b"), fmt("#267f99")});
    m_rules.append({QRegularExpression("\\bdef\\s+(\\w+)"), fmt("#795e26")});
    m_rules.append({QRegularExpression("\\bclass\\s+(\\w+)"), fmt("#267f99", true)});
    m_rules.append({QRegularExpression("@\\w+"), fmt("#795e26", false, true)});
    m_rules.append({QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"), fmt("#098658")});
    m_rules.append({QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'"), fmt("#a31515")});
    m_rules.append({QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\""), fmt("#a31515")});
    m_rules.append({QRegularExpression("#[^\\n]*"), fmt("#008000", false, true)});

    m_tripleFormat = fmt("#a31515");
    m_tripleSingle = QRegularExpression("'''");
    m_tripleDouble = QRegularExpression("\"\"\"");
}

void PythonHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : m_rules) {
        auto it = rule.first.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.second);
        }
    }

    highlightTriple(text, m_tripleDouble, 1);
    highlightTriple(text, m_tripleSingle, 2);
}

void PythonHighlighter::highlightTriple(const QString &text, const QRegularExpression &pattern, int stateId)
{
    int start = 0;
    if (previousBlockState() == stateId) {
        const QRegularExpressionMatch match = pattern.match(text);
        const int end = match.hasMatch() ? match.capturedStart() : -1;
        if (end == -1) {
            setFormat(0, text.length(), m_tripleFormat);
            setCurrentBlockState(stateId);
            return;
        }
        setFormat(0, end + 3, m_tripleFormat);
        start = end + 3;
    }

    while (true) {
        const QRegularExpressionMatch match = pattern.match(text, start);
        if (!match.hasMatch())
            break;
        const int openStart = match.capturedStart();
        const QRegularExpressionMatch match2 = pattern.match(text, openStart + 3);
        if (!match2.hasMatch()) {
            setFormat(openStart, text.length() - openStart, m_tripleFormat);
            setCurrentBlockState(stateId);
            return;
        }
        const int closeEnd = match2.capturedEnd();
        setFormat(openStart, closeEnd - openStart, m_tripleFormat);
        start = closeEnd;
    }
}

JsonHighlighter::JsonHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    m_rules.append({QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"\\s*(?=:)"), fmt("#0451a5")});
    m_rules.append({QRegularExpression("(?<=:)\\s*\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\""), fmt("#a31515")});
    m_rules.append({QRegularExpression("\\b-?[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b"), fmt("#098658")});
    m_rules.append({QRegularExpression("\\b(true|false|null)\\b"), fmt("#0000ff", true)});
}

void JsonHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : m_rules) {
        auto it = rule.first.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.second);
        }
    }
}

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    m_rules.append({QRegularExpression("^#{1,6}\\s.*"), fmt("#0451a5", true)});
    m_rules.append({QRegularExpression("\\*\\*[^*]+\\*\\*"), fmt("#24292f", true)});
    m_rules.append({QRegularExpression("(?<!\\*)\\*[^*]+\\*(?!\\*)"), fmt("#24292f", false, true)});
    m_rules.append({QRegularExpression("`[^`]+`"), fmt("#a31515")});
    m_rules.append({QRegularExpression("\\[[^\\]]*\\]\\([^)]*\\)"), fmt("#0969da")});
    m_rules.append({QRegularExpression("^\\s*[-*+]\\s"), fmt("#af00db")});
    m_rules.append({QRegularExpression("^>.*"), fmt("#6a737d", false, true)});
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : m_rules) {
        auto it = rule.first.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.second);
        }
    }
}

HighlighterKind highlighterKindFor(const QString &filePath)
{
    if (filePath.isEmpty())
        return HighlighterKind::None;
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "py" || ext == "pyw")
        return HighlighterKind::Python;
    if (ext == "json")
        return HighlighterKind::Json;
    if (ext == "md" || ext == "markdown")
        return HighlighterKind::Markdown;
    return HighlighterKind::None;
}

QSyntaxHighlighter *createHighlighter(HighlighterKind kind, QTextDocument *document)
{
    switch (kind) {
    case HighlighterKind::Python:
        return new PythonHighlighter(document);
    case HighlighterKind::Json:
        return new JsonHighlighter(document);
    case HighlighterKind::Markdown:
        return new MarkdownHighlighter(document);
    case HighlighterKind::None:
    default:
        return nullptr;
    }
}
