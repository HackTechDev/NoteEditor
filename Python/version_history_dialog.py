from datetime import datetime

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import (
    QDialog,
    QHBoxLayout,
    QListWidget,
    QListWidgetItem,
    QPlainTextEdit,
    QPushButton,
    QVBoxLayout,
)

import session


def _format_stamp(stamp):
    try:
        return datetime.strptime(stamp, "%Y%m%dT%H%M%S%f").strftime("%d/%m/%Y %H:%M:%S")
    except ValueError:
        return stamp


class VersionHistoryDialog(QDialog):
    restore_requested = pyqtSignal(str)

    def __init__(self, draft_id, parent=None):
        super().__init__(parent)
        self.draft_id = draft_id
        self.setWindowTitle("Historique des versions")
        self.resize(600, 420)

        self.list_widget = QListWidget(self)
        self.list_widget.currentItemChanged.connect(self._show_preview)

        self.preview = QPlainTextEdit(self)
        self.preview.setReadOnly(True)

        self.restore_btn = QPushButton("Restaurer cette version", self)
        self.close_btn = QPushButton("Fermer", self)
        self.restore_btn.clicked.connect(self._restore_selected)
        self.close_btn.clicked.connect(self.close)

        content_row = QHBoxLayout()
        content_row.addWidget(self.list_widget, 1)
        content_row.addWidget(self.preview, 2)

        buttons_row = QHBoxLayout()
        buttons_row.addStretch()
        buttons_row.addWidget(self.restore_btn)
        buttons_row.addWidget(self.close_btn)

        layout = QVBoxLayout(self)
        layout.addLayout(content_row)
        layout.addLayout(buttons_row)

        for stamp in session.list_versions(draft_id):
            item = QListWidgetItem(_format_stamp(stamp))
            item.setData(Qt.ItemDataRole.UserRole, stamp)
            self.list_widget.addItem(item)
        if self.list_widget.count() > 0:
            self.list_widget.setCurrentRow(0)

    def _show_preview(self, current, _previous):
        if current is None:
            self.preview.setPlainText("")
            return
        stamp = current.data(Qt.ItemDataRole.UserRole)
        self.preview.setPlainText(session.read_version(self.draft_id, stamp))

    def _restore_selected(self):
        item = self.list_widget.currentItem()
        if item is None:
            return
        stamp = item.data(Qt.ItemDataRole.UserRole)
        self.restore_requested.emit(session.read_version(self.draft_id, stamp))
        self.close()
