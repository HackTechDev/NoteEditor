import os

from PyQt6.QtCore import QPoint, QSize, Qt, pyqtSignal
from PyQt6.QtGui import QColor, QIcon, QPainter, QPen, QPixmap, QPolygon
from PyQt6.QtWidgets import QListWidget, QListWidgetItem, QMenu

import session

PIN_ICON_SIZE = 14


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


def unpin_pixmap(size=PIN_ICON_SIZE, color=None):
    """Punaise barrée (Détacher) : la punaise, puis un trait diagonal dont le
    contour est effacé dans la punaise pour rester lisible à petite taille."""
    pixmap = pin_pixmap(size, color)
    color = color or QColor(Qt.GlobalColor.darkGray)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    painter.scale(size / 22, size / 22)
    knockout = QPen(color)
    knockout.setWidth(5)
    knockout.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_Clear)
    painter.setPen(knockout)
    painter.drawLine(4, 3, 18, 19)
    painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_SourceOver)
    stroke = QPen(color)
    stroke.setWidth(2)
    stroke.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(stroke)
    painter.drawLine(4, 3, 18, 19)
    painter.end()
    return pixmap


def _row_icon(pinned, selected_color):
    """Icône à gauche du nom dans la liste : une punaise (grise, blanche quand la
    ligne est sélectionnée) pour une note épinglée, sinon un carré transparent de
    même taille, pour que tous les noms restent alignés."""
    icon = QIcon()
    if pinned:
        icon.addPixmap(pin_pixmap(PIN_ICON_SIZE), QIcon.Mode.Normal)
        icon.addPixmap(pin_pixmap(PIN_ICON_SIZE, selected_color), QIcon.Mode.Selected)
    else:
        blank = QPixmap(PIN_ICON_SIZE, PIN_ICON_SIZE)
        blank.fill(Qt.GlobalColor.transparent)
        icon.addPixmap(blank)
    return icon


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
        self.setIconSize(QSize(PIN_ICON_SIZE, PIN_ICON_SIZE))
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
            item.setIcon(_row_icon(entry.get("pinned"), self.palette().highlightedText().color()))
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
