import os

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import QListWidget, QListWidgetItem, QMenu

import session


class DraftsBrowser(QListWidget):
    open_requested = pyqtSignal(dict)
    delete_requested = pyqtSignal(dict)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)
        self.itemDoubleClicked.connect(self._emit_open)

    def refresh(self, exclude_ids=()):
        self.clear()
        for entry in session.list_drafts():
            if entry["id"] in exclude_ids:
                continue
            label = (
                os.path.basename(entry["file_path"])
                if entry["file_path"]
                else entry["default_name"] or entry["id"][:8]
            )
            item = QListWidgetItem(label)
            item.setToolTip(entry["file_path"] or label)
            item.setData(Qt.ItemDataRole.UserRole, entry)
            self.addItem(item)

    def _emit_open(self, item):
        self.open_requested.emit(item.data(Qt.ItemDataRole.UserRole))

    def _show_context_menu(self, pos):
        item = self.itemAt(pos)
        if item is None:
            return
        menu = QMenu(self)
        delete_action = menu.addAction("Supprimer le brouillon")
        chosen = menu.exec(self.mapToGlobal(pos))
        if chosen == delete_action:
            self.delete_requested.emit(item.data(Qt.ItemDataRole.UserRole))
