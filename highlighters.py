import keyword
import os

from PyQt6.QtCore import QRegularExpression
from PyQt6.QtGui import QColor, QFont, QSyntaxHighlighter, QTextCharFormat


def _fmt(color, bold=False, italic=False):
    f = QTextCharFormat()
    f.setForeground(QColor(color))
    if bold:
        f.setFontWeight(QFont.Weight.Bold)
    if italic:
        f.setFontItalic(True)
    return f


class PythonHighlighter(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        self.rules = []

        keyword_fmt = _fmt("#c586c0", bold=True)
        pattern = r"\b(" + "|".join(keyword.kwlist) + r")\b"
        self.rules.append((QRegularExpression(pattern), keyword_fmt))

        builtin_fmt = _fmt("#4ec9b0")
        builtins = ["self", "cls", "None", "True", "False", "print", "len", "range", "int", "str", "float", "list", "dict", "set", "tuple"]
        pattern = r"\b(" + "|".join(builtins) + r")\b"
        self.rules.append((QRegularExpression(pattern), builtin_fmt))

        self.rules.append((QRegularExpression(r"\bdef\s+(\w+)"), _fmt("#dcdcaa")))
        self.rules.append((QRegularExpression(r"\bclass\s+(\w+)"), _fmt("#4ec9b0", bold=True)))
        self.rules.append((QRegularExpression(r"@\w+"), _fmt("#dcdcaa", italic=True)))
        self.rules.append((QRegularExpression(r"\b[0-9]+\.?[0-9]*\b"), _fmt("#b5cea8")))
        self.rules.append((QRegularExpression(r"'[^'\\]*(\\.[^'\\]*)*'"), _fmt("#ce9178")))
        self.rules.append((QRegularExpression(r'"[^"\\]*(\\.[^"\\]*)*"'), _fmt("#ce9178")))
        self.rules.append((QRegularExpression(r"#[^\n]*"), _fmt("#6a9955", italic=True)))

        self.triple_fmt = _fmt("#ce9178")
        self.triple_single = QRegularExpression(r"'''")
        self.triple_double = QRegularExpression(r'"""')

    def highlightBlock(self, text):
        for pattern, fmt in self.rules:
            it = pattern.globalMatch(text)
            while it.hasNext():
                m = it.next()
                self.setFormat(m.capturedStart(), m.capturedLength(), fmt)

        self._highlight_triple(text, self.triple_double, 1)
        self._highlight_triple(text, self.triple_single, 2)

    def _highlight_triple(self, text, pattern, state_id):
        start = 0
        if self.previousBlockState() == state_id:
            match = pattern.match(text)
            end = match.capturedStart() if match.hasMatch() else -1
            if end == -1:
                self.setFormat(0, len(text), self.triple_fmt)
                self.setCurrentBlockState(state_id)
                return
            self.setFormat(0, end + 3, self.triple_fmt)
            start = end + 3

        while True:
            match = pattern.match(text, start)
            if not match.hasMatch():
                break
            open_start = match.capturedStart()
            match2 = pattern.match(text, open_start + 3)
            if not match2.hasMatch():
                self.setFormat(open_start, len(text) - open_start, self.triple_fmt)
                self.setCurrentBlockState(state_id)
                return
            close_end = match2.capturedEnd()
            self.setFormat(open_start, close_end - open_start, self.triple_fmt)
            start = close_end


class JsonHighlighter(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        self.rules = [
            (QRegularExpression(r'"[^"\\]*(\\.[^"\\]*)*"\s*(?=:)'), _fmt("#9cdcfe")),
            (QRegularExpression(r'(?<=:)\s*"[^"\\]*(\\.[^"\\]*)*"'), _fmt("#ce9178")),
            (QRegularExpression(r"\b-?[0-9]+\.?[0-9]*([eE][+-]?[0-9]+)?\b"), _fmt("#b5cea8")),
            (QRegularExpression(r"\b(true|false|null)\b"), _fmt("#569cd6", bold=True)),
        ]

    def highlightBlock(self, text):
        for pattern, fmt in self.rules:
            it = pattern.globalMatch(text)
            while it.hasNext():
                m = it.next()
                self.setFormat(m.capturedStart(), m.capturedLength(), fmt)


class MarkdownHighlighter(QSyntaxHighlighter):
    def __init__(self, document):
        super().__init__(document)
        self.rules = [
            (QRegularExpression(r"^#{1,6}\s.*"), _fmt("#569cd6", bold=True)),
            (QRegularExpression(r"\*\*[^*]+\*\*"), _fmt("#d4d4d4", bold=True)),
            (QRegularExpression(r"(?<!\*)\*[^*]+\*(?!\*)"), _fmt("#d4d4d4", italic=True)),
            (QRegularExpression(r"`[^`]+`"), _fmt("#ce9178")),
            (QRegularExpression(r"\[[^\]]*\]\([^)]*\)"), _fmt("#4ec9b0")),
            (QRegularExpression(r"^\s*[-*+]\s"), _fmt("#c586c0")),
            (QRegularExpression(r"^>.*"), _fmt("#6a9955", italic=True)),
        ]

    def highlightBlock(self, text):
        for pattern, fmt in self.rules:
            it = pattern.globalMatch(text)
            while it.hasNext():
                m = it.next()
                self.setFormat(m.capturedStart(), m.capturedLength(), fmt)


EXTENSION_MAP = {
    ".py": PythonHighlighter,
    ".pyw": PythonHighlighter,
    ".json": JsonHighlighter,
    ".md": MarkdownHighlighter,
    ".markdown": MarkdownHighlighter,
}


def highlighter_class_for(file_path):
    if not file_path:
        return None
    ext = os.path.splitext(file_path)[1].lower()
    return EXTENSION_MAP.get(ext)
