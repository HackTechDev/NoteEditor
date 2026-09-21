import uuid

from PyQt6.QtCore import QRect, QSize, Qt, QTimer, pyqtSignal
from PyQt6.QtGui import QColor, QFont, QPainter, QTextCursor, QTextFormat
from PyQt6.QtWidgets import QPlainTextEdit, QTextEdit, QWidget

from highlighters import highlighter_class_for

AUTOSAVE_DELAY_MS = 1500
INDENT = "    "

# Historique annuler/rétablir mémorisé d'un lancement à l'autre : nombre maximal
# d'étapes de chaque côté du point courant, et volume total de texte conservé.
HISTORY_MAX_STEPS = 200
HISTORY_MAX_CHARS = 1_000_000
# Au-delà de cette taille de note, l'historique n'est plus suivi (chaque frappe
# coûterait une comparaison du texte entier) ; annuler/rétablir continue de marcher.
HISTORY_TRACK_MAX_CHARS = 300_000


def _u16(text):
    """Longueur en unités UTF-16, celles des positions de QTextDocument."""
    return len(text.encode("utf-16-le")) // 2


def _from_u16(text, count):
    """Indice (en caractères) correspondant à `count` unités UTF-16 de `text`."""
    return len(text.encode("utf-16-le")[: 2 * count].decode("utf-16-le"))


def _diff(old, new):
    """Étape (début, texte retiré, texte ajouté) qui transforme `old` en `new`."""
    limit = min(len(old), len(new))
    # recherche dichotomique du préfixe puis du suffixe communs (comparaisons de tranches, en C)
    low, high = 0, limit
    while low < high:
        mid = (low + high + 1) // 2
        if old[:mid] == new[:mid]:
            low = mid
        else:
            high = mid - 1
    start = low
    low, high = 0, limit - start
    while low < high:
        mid = (low + high + 1) // 2
        if old[len(old) - mid:] == new[len(new) - mid:]:
            low = mid
        else:
            high = mid - 1
    end = low
    return start, old[start:len(old) - end], new[start:len(new) - end]


def _apply(text, step):
    start, removed, added = step
    return text[:start] + added + text[start + len(removed):]


def _revert(text, step):
    start, removed, added = step
    return text[:start] + removed + text[start + len(added):]


class LineNumberArea(QWidget):
    def __init__(self, editor):
        super().__init__(editor)
        self.editor = editor

    def sizeHint(self):
        return QSize(self.editor.line_number_area_width(), 0)

    def paintEvent(self, event):
        self.editor.line_number_area_paint_event(event)


class Editor(QPlainTextEdit):
    autosave_requested = pyqtSignal()
    mode_changed = pyqtSignal()  # entrée / sortie du mode commande

    def __init__(self, parent=None):
        super().__init__(parent)
        self.file_path = None
        self.default_name = None
        self.highlighter = None
        self.session_id = uuid.uuid4().hex
        self.disk_mtime = None
        self._pending_scroll = None
        # mode « commande » à la Vim (Échap) ; sinon on est en mode insertion
        self.command_mode = False
        # suivi de l'historique annuler/rétablir : Qt ne permet pas de le lire ni de
        # l'exporter, on en garde donc une copie sous forme d'étapes (début, retiré, ajouté)
        self._hist_steps = []
        self._hist_pos = 0
        self._hist_text = ""
        self._hist_new_step = False
        self._hist_busy = False

        font = QFont("Monospace")
        font.setStyleHint(QFont.StyleHint.TypeWriter)
        font.setPointSize(11)
        self.setFont(font)
        self.setTabStopDistance(4 * self.fontMetrics().horizontalAdvance(" "))

        self.line_number_area = LineNumberArea(self)
        self.blockCountChanged.connect(self.update_line_number_area_width)
        self.updateRequest.connect(self.update_line_number_area)
        self.cursorPositionChanged.connect(self.highlight_current_line)

        self._autosave_timer = QTimer(self)
        self._autosave_timer.setSingleShot(True)
        self._autosave_timer.setInterval(AUTOSAVE_DELAY_MS)
        self._autosave_timer.timeout.connect(self.autosave_requested)
        self.document().contentsChanged.connect(self._autosave_timer.start)

        self.document().undoCommandAdded.connect(self._on_undo_command_added)
        self.document().contentsChange.connect(self._track_history)

        self.update_line_number_area_width()
        self.highlight_current_line()

    def _on_undo_command_added(self):
        self._hist_new_step = True

    def _track_history(self, *_):
        if self._hist_busy:
            return
        new_step, self._hist_new_step = self._hist_new_step, False
        doc = self.document()
        if doc.characterCount() > HISTORY_TRACK_MAX_CHARS:
            self._hist_steps, self._hist_pos, self._hist_text = [], 0, ""
            return
        text = self.toPlainText()
        if not doc.isUndoAvailable() and not doc.isRedoAvailable():
            # pile vidée (setPlainText : ouverture, rechargement, version restaurée)
            self._hist_steps, self._hist_pos, self._hist_text = [], 0, text
            return
        current, steps, pos = self._hist_text, self._hist_steps, self._hist_pos
        if not new_step:
            if pos > 0 and text == _revert(current, steps[pos - 1]):
                self._hist_pos, self._hist_text = pos - 1, text  # annuler
                return
            if pos < len(steps) and text == _apply(current, steps[pos]):
                self._hist_pos, self._hist_text = pos + 1, text  # rétablir
                return
            if pos > 0:
                # frappe fusionnée par Qt dans la dernière étape
                steps[pos - 1] = _diff(_revert(current, steps[pos - 1]), text)
                self._hist_text = text
                return
        del steps[pos:]
        steps.append(_diff(current, text))
        self._hist_pos, self._hist_text = len(steps), text

    def history_state(self):
        """Historique annuler/rétablir sérialisable (positions en unités UTF-16, comme
        Qt), ou None s'il est vide. Réduit aux dernières étapes de chaque côté du point
        courant, et à un volume de texte raisonnable."""
        steps, pos = self._hist_steps, self._hist_pos
        if not steps:
            return None
        before = steps[max(0, pos - HISTORY_MAX_STEPS):pos]
        after = steps[pos:pos + HISTORY_MAX_STEPS]
        cost = lambda s: len(s[1]) + len(s[2])
        budget = HISTORY_MAX_CHARS
        while before and sum(map(cost, before)) > budget:
            before.pop(0)
        budget -= sum(map(cost, before))
        while after and sum(map(cost, after)) > budget:
            after.pop()
        kept = before + after
        if not kept:
            return None
        # les positions se rapportent au texte avant chaque étape
        text = self._hist_text
        for step in reversed(steps[pos - len(before):pos]):
            text = _revert(text, step)
        out = []
        for start, removed, added in kept:
            out.append({"at": _u16(text[:start]), "del": removed, "ins": added})
            text = _apply(text, (start, removed, added))
        return {"pos": len(before), "length": _u16(self._hist_text), "steps": out}

    def restore_history(self, data):
        """Reconstruit la pile annuler/rétablir à partir de `history_state()`. Sans effet
        (et renvoie False) si elle ne correspond pas au texte actuel de l'onglet."""
        try:
            pos, length, raw = int(data["pos"]), int(data["length"]), data["steps"]
            wire = [(int(s["at"]), str(s["del"]), str(s["ins"])) for s in raw]
        except (KeyError, TypeError, ValueError):
            return False
        current = self.toPlainText()
        if not wire or not 0 <= pos <= len(wire) or _u16(current) != length:
            return False
        # de retour au texte de départ : chaque étape à rebours doit retomber juste
        steps = [None] * len(wire)
        text = current
        try:
            for k in range(pos - 1, -1, -1):
                at, removed, added = wire[k]
                start = _from_u16(text, at)
                if text[start:start + len(added)] != added:
                    return False
                steps[k] = (start, removed, added)
                text = _revert(text, steps[k])
            base = text
            text = current
            for k in range(pos, len(wire)):
                at, removed, added = wire[k]
                start = _from_u16(text, at)
                if text[start:start + len(removed)] != removed:
                    return False
                steps[k] = (start, removed, added)
                text = _apply(text, steps[k])
        except (UnicodeError, ValueError):
            return False
        modified = self.document().isModified()
        self._hist_busy = True
        try:
            self.setPlainText(base)
            cursor = QTextCursor(self.document())
            for (_, removed, added), (at, _r, _a) in zip(steps, wire):
                cursor.beginEditBlock()
                cursor.setPosition(at)
                cursor.setPosition(at + _u16(removed), QTextCursor.MoveMode.KeepAnchor)
                cursor.insertText(added)
                cursor.endEditBlock()
            for _ in range(len(wire) - pos):
                self.document().undo()
            ok = self.toPlainText() == current
            if not ok:
                self.setPlainText(current)
        finally:
            self._hist_busy = False
            self._hist_new_step = False
            self.document().setModified(modified)
        if ok:
            self._hist_steps, self._hist_pos, self._hist_text = steps, pos, current
        else:
            self._hist_steps, self._hist_pos, self._hist_text = [], 0, current
        return ok

    def restore_view(self, cursor, scroll, anchor=None):
        """Replace le curseur (et la sélection, si `anchor` en est le début), et programme
        le défilement : la barre de défilement n'a pas encore sa plage tant que l'onglet
        n'a jamais été affiché."""
        last = self.document().characterCount() - 1
        doc_cursor = self.textCursor()
        if anchor is not None:
            doc_cursor.setPosition(max(0, min(int(anchor), last)))
            doc_cursor.setPosition(max(0, min(int(cursor), last)), QTextCursor.MoveMode.KeepAnchor)
        else:
            doc_cursor.setPosition(max(0, min(int(cursor), last)))
        self.setTextCursor(doc_cursor)
        self._pending_scroll = int(scroll) if scroll else None

    def view_state(self):
        """(position du curseur, défilement, début de la sélection) ; le défilement
        programmé mais pas encore appliqué (onglet jamais affiché depuis la
        restauration) prime."""
        scroll = self._pending_scroll if self._pending_scroll is not None else self.verticalScrollBar().value()
        cursor = self.textCursor()
        return cursor.position(), scroll, cursor.anchor()

    def showEvent(self, event):
        super().showEvent(event)
        if self._pending_scroll is not None:
            QTimer.singleShot(0, self._apply_pending_scroll)

    def _apply_pending_scroll(self):
        if self._pending_scroll is not None:
            value, self._pending_scroll = self._pending_scroll, None
            self.verticalScrollBar().setValue(value)

    def line_number_area_width(self):
        digits = len(str(max(1, self.blockCount())))
        return 12 + self.fontMetrics().horizontalAdvance("9") * digits

    def update_line_number_area_width(self):
        self.setViewportMargins(self.line_number_area_width(), 0, 0, 0)

    def update_line_number_area(self, rect, dy):
        if dy:
            self.line_number_area.scroll(0, dy)
        else:
            self.line_number_area.update(0, rect.y(), self.line_number_area.width(), rect.height())
        if rect.contains(self.viewport().rect()):
            self.update_line_number_area_width()

    def set_command_mode(self, enabled):
        """Mode commande (Échap) : le curseur devient un bloc et la frappe n'insère plus de
        texte ; Échap de nouveau, ou une commande comme `o`, ramène au mode insertion."""
        if enabled == self.command_mode:
            return
        self.command_mode = enabled
        self.setCursorWidth(self.fontMetrics().horizontalAdvance(" ") if enabled else 1)
        self.mode_changed.emit()

    def _open_line_below(self):
        """Commande `o` : insère une ligne vide sous la ligne du curseur et s'y place."""
        cursor = self.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.EndOfBlock)
        cursor.insertBlock()
        self.setTextCursor(cursor)

    def keyPressEvent(self, event):
        key, mods = event.key(), event.modifiers()
        plain = not mods & (Qt.KeyboardModifier.ControlModifier | Qt.KeyboardModifier.AltModifier
                            | Qt.KeyboardModifier.MetaModifier)
        shift = bool(mods & Qt.KeyboardModifier.ShiftModifier)
        if plain and key == Qt.Key.Key_Escape:
            self.set_command_mode(not self.command_mode)
            event.accept()
            return
        if self.command_mode and plain:
            if event.text() == "o":
                self._open_line_below()
                self.set_command_mode(False)
                event.accept()
                return
            if event.text() or key in (Qt.Key.Key_Delete, Qt.Key.Key_Insert,
                                       Qt.Key.Key_Tab, Qt.Key.Key_Backtab):
                event.accept()  # pas de saisie en mode commande ; les flèches, Début, Fin… passent
                return
        if plain and (key == Qt.Key.Key_Backtab or (shift and key == Qt.Key.Key_Tab)):
            self._shift_lines(indent=False)
            event.accept()
        elif plain and not shift and key == Qt.Key.Key_Tab and self.textCursor().hasSelection():
            self._shift_lines(indent=True)
            event.accept()
        else:
            super().keyPressEvent(event)

    def _shift_lines(self, indent):
        """Tab : décale de 4 espaces vers la droite les lignes touchées par la sélection ;
        Maj+Tab : les décale vers la gauche (jusqu'à 4 espaces, ou une tabulation, en
        moins). Une seule étape d'annulation ; la sélection suit le texte."""
        doc = self.document()
        cursor = self.textCursor()
        anchor, position = cursor.anchor(), cursor.position()
        start, end = min(anchor, position), max(anchor, position)
        block = doc.findBlock(start)
        last = doc.findBlock(end)
        if end > start and end == last.position():
            last = last.previous()  # la ligne où la sélection ne fait que commencer n'est pas touchée
        changes = []  # (début de ligne avant modification, variation de longueur)
        applied = 0
        cursor.beginEditBlock()
        edit = QTextCursor(doc)
        while block.isValid():
            text = block.text()
            if indent:
                delta = len(INDENT) if text else 0  # une ligne vide reste vide
                if delta:
                    edit.setPosition(block.position())
                    edit.insertText(INDENT)
            else:
                if text.startswith("\t"):
                    delta = -1
                else:
                    delta = -min(len(INDENT), len(text) - len(text.lstrip(" ")))
                if delta:
                    edit.setPosition(block.position())
                    edit.setPosition(block.position() - delta, QTextCursor.MoveMode.KeepAnchor)
                    edit.removeSelectedText()
            if delta:
                changes.append((block.position() - applied, delta))
                applied += delta
            if block == last:
                break
            block = block.next()
        cursor.endEditBlock()

        def moved(pos, is_start):
            shift = 0
            for at, delta in changes:
                if delta > 0:
                    if pos > at or (pos == at and not is_start):
                        shift += delta
                elif pos >= at - delta:
                    shift += delta
                elif pos > at:
                    shift -= pos - at
            return pos + shift

        result = self.textCursor()
        result.setPosition(moved(anchor, anchor == start))
        result.setPosition(moved(position, position == start), QTextCursor.MoveMode.KeepAnchor)
        self.setTextCursor(result)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        cr = self.contentsRect()
        self.line_number_area.setGeometry(QRect(cr.left(), cr.top(), self.line_number_area_width(), cr.height()))

    def line_number_area_paint_event(self, event):
        painter = QPainter(self.line_number_area)
        painter.fillRect(event.rect(), QColor("#f0f0f0"))

        block = self.firstVisibleBlock()
        block_number = block.blockNumber()
        top = round(self.blockBoundingGeometry(block).translated(self.contentOffset()).top())
        bottom = top + round(self.blockBoundingRect(block).height())

        painter.setPen(QColor("#8a8a8a"))
        while block.isValid() and top <= event.rect().bottom():
            if block.isVisible() and bottom >= event.rect().top():
                painter.drawText(
                    0, top, self.line_number_area.width() - 6, self.fontMetrics().height(),
                    Qt.AlignmentFlag.AlignRight, str(block_number + 1),
                )
            block = block.next()
            top = bottom
            bottom = top + round(self.blockBoundingRect(block).height())
            block_number += 1

    def highlight_current_line(self):
        selection = QTextEdit.ExtraSelection()
        selection.format.setBackground(QColor("#e8f2ff"))
        selection.format.setProperty(QTextFormat.Property.FullWidthSelection, True)
        selection.cursor = self.textCursor()
        selection.cursor.clearSelection()
        self.setExtraSelections([selection])

    def set_file_path(self, path):
        self.file_path = path
        cls = highlighter_class_for(path)
        current_cls = type(self.highlighter) if self.highlighter else None
        if cls is current_cls:
            return
        if self.highlighter is not None:
            self.highlighter.setDocument(None)
            self.highlighter = None
        if cls is not None:
            self.highlighter = cls(self.document())
