import os
from datetime import datetime

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import (
    QAbstractItemView,
    QDialog,
    QHBoxLayout,
    QListWidget,
    QListWidgetItem,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
)

import session


def _label_for(entry):
    if entry.get("file_path"):
        return os.path.basename(entry["file_path"])
    return entry.get("default_name") or entry["id"][:8]


def _format_deleted_at(iso_text):
    try:
        return datetime.fromisoformat(iso_text).strftime("%d/%m/%Y %H:%M")
    except ValueError:
        return iso_text


class TrashDialog(QDialog):
    restored = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Corbeille")
        self.resize(420, 360)

        self.list_widget = QListWidget(self)
        self.list_widget.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)

        self.restore_btn = QPushButton("Restaurer", self)
        self.purge_btn = QPushButton("Supprimer définitivement", self)
        self.close_btn = QPushButton("Fermer", self)

        self.restore_btn.clicked.connect(self._restore_selected)
        self.purge_btn.clicked.connect(self._purge_selected)
        self.close_btn.clicked.connect(self.close)

        layout = QVBoxLayout(self)
        layout.addWidget(self.list_widget)

        buttons_row = QHBoxLayout()
        buttons_row.addWidget(self.restore_btn)
        buttons_row.addWidget(self.purge_btn)
        buttons_row.addStretch()
        buttons_row.addWidget(self.close_btn)
        layout.addLayout(buttons_row)

        self.refresh()

    def refresh(self):
        self.list_widget.clear()
        for entry in session.list_trash():
            label = f"{_label_for(entry)} — supprimé le {_format_deleted_at(entry['deleted_at'])}"
            item = QListWidgetItem(label)
            item.setData(Qt.ItemDataRole.UserRole, entry)
            self.list_widget.addItem(item)

    def _selected_entries(self):
        return [item.data(Qt.ItemDataRole.UserRole) for item in self.list_widget.selectedItems()]

    def _restore_selected(self):
        entries = self._selected_entries()
        if not entries:
            return
        for entry in entries:
            session.restore_draft(entry["id"])
        self.refresh()
        self.restored.emit()

    def _purge_selected(self):
        entries = self._selected_entries()
        if not entries:
            return
        if len(entries) == 1:
            question = f"Supprimer définitivement « {_label_for(entries[0])} » ? Cette action est irréversible."
        else:
            question = f"Supprimer définitivement ces {len(entries)} brouillons ? Cette action est irréversible."
        if any(e.get("file_path") for e in entries):
            # purge_draft() ne touche que le brouillon dans trash/, jamais le fichier réel
            plural = len(entries) > 1
            question += "\n\n" + (
                "Les fichiers associés ne sont pas supprimés du disque : ils restent à leur emplacement."
                if plural else
                "Le fichier associé n'est pas supprimé du disque : il reste à son emplacement."
            )
        result = QMessageBox.question(
            self,
            "Supprimer définitivement",
            question,
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if result == QMessageBox.StandardButton.Yes:
            for entry in entries:
                session.purge_draft(entry["id"])
            self.refresh()
