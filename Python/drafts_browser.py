import os

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import QListWidget, QListWidgetItem, QMenu

import session


class DraftsBrowser(QListWidget):
    open_requested = pyqtSignal(dict)
    delete_requested = pyqtSignal(dict)
    rename_requested = pyqtSignal(dict)
    # Actions partagées avec le menu contextuel des onglets : "close",
    # "close_others", "close_right", "close_all", "duplicate", "history".
    action_requested = pyqtSignal(str, dict)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self._show_context_menu)
        self.itemDoubleClicked.connect(self._emit_open)
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
        has_history = bool(session.list_versions(entry["id"]))

        menu = QMenu(self)
        rename_action = None
        if not entry.get("file_path"):
            rename_action = menu.addAction("Renommer...")
        menu.addSeparator()

        close_action = menu.addAction("Fermer")
        close_action.setEnabled(is_open)
        close_others_action = menu.addAction("Fermer les autres")
        close_others_action.setEnabled(is_open)
        close_right_action = menu.addAction("Fermer à droite")
        close_right_action.setEnabled(is_open)
        close_all_action = menu.addAction("Fermer tout")
        menu.addSeparator()

        duplicate_action = menu.addAction("Dupliquer")
        history_action = menu.addAction("Historique des versions...")
        history_action.setEnabled(has_history)
        menu.addSeparator()

        delete_action = menu.addAction("Mettre à la corbeille")

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
