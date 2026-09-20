import os

from PyQt6.QtCore import QPoint, QSize, Qt, pyqtSignal
from PyQt6.QtGui import QColor, QIcon, QPainter, QPen, QPixmap, QPolygon
from PyQt6.QtWidgets import QAbstractItemView, QApplication, QListWidget, QListWidgetItem, QMenu

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


def path_tooltip(file_path, label, draft_id):
    """Texte d'infobulle d'une note (panneau et onglet) : le chemin de son fichier ;
    pour une note sans fichier, où son brouillon est stocké."""
    if file_path:
        return file_path
    backup = os.path.join(session.DRAFTS_DIR, draft_id + ".txt")
    return f"{label}\nPas encore enregistrée dans un fichier\nBrouillon : {backup}"


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
    # Action sur plusieurs notes sélectionnées d'un coup : ("close" | "pin" |
    # "unpin" | "trash", liste de dict).
    bulk_action_requested = pyqtSignal(str, list)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)
        self.itemDoubleClicked.connect(self._emit_open)
        self.setIconSize(QSize(PIN_ICON_SIZE, PIN_ICON_SIZE))
        self.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
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
            if entry["id"] not in self._open_ids and session.is_external_file(entry["file_path"]):
                # un fichier extérieur à ~/.noteeditor n'est listé que tant qu'il est
                # ouvert : une fois fermé, son texte reste dans le fichier lui-même
                continue
            display = label + " (ouvert)" if entry["id"] in self._open_ids else label
            item = QListWidgetItem(display)
            item.setIcon(_row_icon(entry.get("pinned"), self.palette().highlightedText().color()))
            item.setToolTip(path_tooltip(entry["file_path"], label, entry["id"]))
            item.setData(Qt.ItemDataRole.UserRole, entry)
            self.addItem(item)

    @staticmethod
    def _label_for(entry):
        if entry["file_path"]:
            return os.path.basename(entry["file_path"])
        return entry["default_name"] or entry["id"][:8]

    def select_ids(self, draft_ids):
        """Sélectionne (en plus de l'éventuelle sélection) les entrées de ces notes."""
        wanted = set(draft_ids)
        for i in range(self.count()):
            item = self.item(i)
            entry = item.data(Qt.ItemDataRole.UserRole)
            if entry and entry.get("id") in wanted:
                item.setSelected(True)

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

        selected = [i.data(Qt.ItemDataRole.UserRole) for i in self.selectedItems()]
        if len(selected) > 1 and item.isSelected():
            n = len(selected)
            pinned = [e for e in selected if e.get("pinned")]
            unpinned = [e for e in selected if not e.get("pinned")]
            closable = [e for e in unpinned if e["id"] in self._open_ids]
            menu = QMenu(self)
            close_selected = menu.addAction(f"Fermer les {n} notes sélectionnées")
            close_selected.setEnabled(bool(closable))
            menu.addSeparator()
            pin_selected = menu.addAction(f"Épingler les {n} notes sélectionnées")
            pin_selected.setEnabled(bool(unpinned))
            unpin_selected = menu.addAction(f"Détacher les {n} notes sélectionnées")
            unpin_selected.setEnabled(bool(pinned))
            menu.addSeparator()
            # les notes épinglées ne peuvent pas être mises à la corbeille : elles sont ignorées
            trash_selected = menu.addAction(f"Mettre les {n} notes sélectionnées à la corbeille")
            trash_selected.setEnabled(bool(unpinned))
            chosen = menu.exec(self.mapToGlobal(pos))
            for action, name in (
                (close_selected, "close"),
                (pin_selected, "pin"),
                (unpin_selected, "unpin"),
                (trash_selected, "trash"),
            ):
                if chosen == action:
                    self.bulk_action_requested.emit(name, selected)
            return

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

        # le nom marche pour toute note (fichier ou nom de la note) ; le chemin
        # n'existe que pour une note liée à un fichier
        copy_name_action = menu.addAction("Copier le nom du fichier")
        copy_path_action = menu.addAction("Copier le chemin complet du fichier")
        copy_path_action.setEnabled(bool(entry.get("file_path")))
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
        elif chosen == copy_name_action:
            QApplication.clipboard().setText(self._label_for(entry))
        elif chosen == copy_path_action:
            QApplication.clipboard().setText(entry["file_path"])
