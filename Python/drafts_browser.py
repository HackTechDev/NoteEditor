import os

from PyQt6.QtCore import QPoint, Qt, pyqtSignal
from PyQt6.QtGui import QColor, QPainter, QPen, QPixmap, QPolygon
from PyQt6.QtWidgets import (
    QApplication,
    QListWidget,
    QListWidgetItem,
    QMenu,
    QStyle,
    QStyledItemDelegate,
    QStyleOptionViewItem,
)

import session

PIN_ICON_SIZE = 14
PIN_ICON_MARGIN = 6


def pin_pixmap(size=PIN_ICON_SIZE, color=None):
    """Dessine une petite icône « punaise » : pleine et d'une couleur unie, pour
    rester lisible à petite taille, sur fond clair comme sur une ligne
    sélectionnée."""
    pixmap = QPixmap(size, size)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    painter.scale(size / 22, size / 22)
    color = color or QColor(Qt.GlobalColor.darkGray)
    pen = QPen(color)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    painter.setBrush(color)
    painter.drawLine(6, 3, 16, 3)  # tête
    painter.drawPolygon(
        QPolygon([QPoint(8, 3), QPoint(8, 9), QPoint(5, 13), QPoint(17, 13), QPoint(14, 9), QPoint(14, 3)])
    )
    painter.drawLine(11, 13, 11, 20)  # aiguille
    painter.end()
    return pixmap


class _PinDelegate(QStyledItemDelegate):
    """Dessine le nom de la note comme d'habitude, plus une petite punaise au
    bord droit de la ligne quand la note est épinglée."""

    def paint(self, painter, option, index):
        entry = index.data(Qt.ItemDataRole.UserRole)
        if not (entry and entry.get("pinned")):
            super().paint(painter, option, index)
            return

        opt = QStyleOptionViewItem(option)
        self.initStyleOption(opt, index)
        widget = opt.widget
        style = widget.style() if widget else QApplication.style()
        # fond (y compris sélection) sur toute la ligne, puis texte sans la zone de la punaise
        style.drawPrimitive(QStyle.PrimitiveElement.PE_PanelItemViewItem, opt, painter, widget)
        text_opt = QStyleOptionViewItem(opt)
        right = opt.rect.right()
        if widget is not None:
            right = min(right, widget.viewport().width() - 1)
        text_opt.rect = opt.rect.adjusted(0, 0, -(opt.rect.right() - right + PIN_ICON_SIZE + 2 * PIN_ICON_MARGIN), 0)
        super().paint(painter, text_opt, index)

        selected = bool(opt.state & QStyle.StateFlag.State_Selected)
        color = opt.palette.highlightedText().color() if selected else QColor(Qt.GlobalColor.darkGray)
        x = right - PIN_ICON_SIZE - PIN_ICON_MARGIN + 1
        y = opt.rect.top() + (opt.rect.height() - PIN_ICON_SIZE) // 2
        painter.drawPixmap(x, y, pin_pixmap(PIN_ICON_SIZE, color))


class DraftsBrowser(QListWidget):
    open_requested = pyqtSignal(dict)
    delete_requested = pyqtSignal(dict)
    rename_requested = pyqtSignal(dict)
    # Actions partagées avec le menu contextuel des onglets : "close",
    # "close_others", "close_right", "close_all", "duplicate", "history",
    # "toggle_pin".
    action_requested = pyqtSignal(str, dict)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)
        self.itemDoubleClicked.connect(self._emit_open)
        self.setItemDelegate(_PinDelegate(self))
        # Pas de défilement horizontal : la punaise est ancrée au bord droit
        # visible de la ligne. Les noms trop longs sont tronqués (« … »), le nom
        # complet reste dans l'infobulle.
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.setTextElideMode(Qt.TextElideMode.ElideRight)
        self._open_ids = set()
        self._sort_mode = "date"
        self._filter_text = ""

    def set_sort_mode(self, mode):
        self._sort_mode = mode
        self.refresh(self._open_ids)

    def set_filter_text(self, text):
        self._filter_text = text
        self.refresh(self._open_ids)

    def refresh(self, open_ids=()):
        self._open_ids = set(open_ids)
        self.clear()

        entries = session.list_drafts()
        if self._sort_mode == "name":
            entries.sort(key=lambda e: self._label_for(e).lower())

        needle = self._filter_text.strip().lower()
        for entry in entries:
            label = self._label_for(entry)
            if needle and needle not in label.lower():
                continue
            display = label + " (ouvert)" if entry["id"] in self._open_ids else label
            item = QListWidgetItem(display)
            item.setToolTip(entry["file_path"] or label)
            item.setData(Qt.ItemDataRole.UserRole, entry)
            self.addItem(item)

    @staticmethod
    def _label_for(entry):
        if entry["file_path"]:
            return os.path.basename(entry["file_path"])
        return entry["default_name"] or entry["id"][:8]

    def select_draft(self, draft_id):
        for i in range(self.count()):
            item = self.item(i)
            entry = item.data(Qt.ItemDataRole.UserRole)
            if entry and entry.get("id") == draft_id:
                self.setCurrentItem(item)
                return
        self.setCurrentItem(None)

    def _emit_open(self, item):
        self.open_requested.emit(item.data(Qt.ItemDataRole.UserRole))

    def _show_context_menu(self, pos):
        item = self.itemAt(pos)
        if item is None:
            return
        entry = item.data(Qt.ItemDataRole.UserRole)
        is_open = entry["id"] in self._open_ids
        is_pinned = bool(entry.get("pinned"))
        has_history = bool(session.list_versions(entry["id"]))

        menu = QMenu(self)
        rename_action = None
        if not entry.get("file_path"):
            rename_action = menu.addAction("Renommer...")
        menu.addSeparator()

        close_action = menu.addAction("Fermer")
        close_action.setEnabled(is_open and not is_pinned)
        close_others_action = menu.addAction("Fermer les autres")
        close_others_action.setEnabled(is_open)
        close_right_action = menu.addAction("Fermer à droite")
        close_right_action.setEnabled(is_open)
        close_all_action = menu.addAction("Fermer tout")
        menu.addSeparator()

        pin_action = menu.addAction("Détacher" if is_pinned else "Épingler")
        duplicate_action = menu.addAction("Dupliquer")
        history_action = menu.addAction("Historique des versions...")
        history_action.setEnabled(has_history)
        menu.addSeparator()

        delete_action = menu.addAction("Mettre à la corbeille")
        delete_action.setEnabled(not is_pinned)

        chosen = menu.exec(self.mapToGlobal(pos))
        if chosen == delete_action:
            self.delete_requested.emit(entry)
        elif rename_action is not None and chosen == rename_action:
            self.rename_requested.emit(entry)
        elif chosen == close_action:
            self.action_requested.emit("close", entry)
        elif chosen == close_others_action:
            self.action_requested.emit("close_others", entry)
        elif chosen == close_right_action:
            self.action_requested.emit("close_right", entry)
        elif chosen == close_all_action:
            self.action_requested.emit("close_all", entry)
        elif chosen == duplicate_action:
            self.action_requested.emit("duplicate", entry)
        elif chosen == history_action:
            self.action_requested.emit("history", entry)
        elif chosen == pin_action:
            self.action_requested.emit("toggle_pin", entry)
