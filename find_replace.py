from PyQt6.QtCore import Qt
from PyQt6.QtGui import QTextCursor, QTextDocument
from PyQt6.QtWidgets import (
    QCheckBox,
    QDialog,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
)


class FindReplaceDialog(QDialog):
    def __init__(self, main_window):
        super().__init__(main_window)
        self.main_window = main_window
        self.setWindowTitle("Rechercher / Remplacer")
        self.setWindowFlag(Qt.WindowType.Tool)

        self.find_edit = QLineEdit()
        self.replace_edit = QLineEdit()
        self.case_check = QCheckBox("Sensible à la casse")
        self.whole_words_check = QCheckBox("Mot entier")

        self.find_next_btn = QPushButton("Suivant")
        self.find_prev_btn = QPushButton("Précédent")
        self.replace_btn = QPushButton("Remplacer")
        self.replace_all_btn = QPushButton("Tout remplacer")
        self.close_btn = QPushButton("Fermer")

        self.find_next_btn.clicked.connect(self.find_next)
        self.find_prev_btn.clicked.connect(self.find_previous)
        self.replace_btn.clicked.connect(self.replace_one)
        self.replace_all_btn.clicked.connect(self.replace_all)
        self.close_btn.clicked.connect(self.close)

        self.find_edit.returnPressed.connect(self.find_next)
        self.replace_edit.returnPressed.connect(self.replace_one)

        layout = QGridLayout(self)
        layout.addWidget(QLabel("Rechercher :"), 0, 0)
        layout.addWidget(self.find_edit, 0, 1)
        layout.addWidget(QLabel("Remplacer par :"), 1, 0)
        layout.addWidget(self.replace_edit, 1, 1)

        options_row = QHBoxLayout()
        options_row.addWidget(self.case_check)
        options_row.addWidget(self.whole_words_check)
        layout.addLayout(options_row, 2, 0, 1, 2)

        buttons_row = QHBoxLayout()
        buttons_row.addWidget(self.find_prev_btn)
        buttons_row.addWidget(self.find_next_btn)
        buttons_row.addWidget(self.replace_btn)
        buttons_row.addWidget(self.replace_all_btn)
        buttons_row.addWidget(self.close_btn)
        layout.addLayout(buttons_row, 3, 0, 1, 2)

        self.setLayout(layout)

    def show_for_find(self):
        self.show()
        self.raise_()
        self.activateWindow()
        self.find_edit.setFocus()
        self.find_edit.selectAll()

    def show_for_replace(self):
        self.show_for_find()
        self.replace_edit.setFocus()

    def _flags(self, backward=False):
        flags = QTextDocument.FindFlag(0)
        if backward:
            flags |= QTextDocument.FindFlag.FindBackward
        if self.case_check.isChecked():
            flags |= QTextDocument.FindFlag.FindCaseSensitively
        if self.whole_words_check.isChecked():
            flags |= QTextDocument.FindFlag.FindWholeWords
        return flags

    def _current_editor(self):
        return self.main_window.current_editor()

    def find_next(self):
        return self._find(backward=False)

    def find_previous(self):
        return self._find(backward=True)

    def _find(self, backward):
        editor = self._current_editor()
        text = self.find_edit.text()
        if editor is None or not text:
            return False
        flags = self._flags(backward=backward)
        found = editor.find(text, flags)
        if not found:
            cursor = editor.textCursor()
            cursor.movePosition(
                QTextCursor.MoveOperation.End if backward else QTextCursor.MoveOperation.Start
            )
            editor.setTextCursor(cursor)
            found = editor.find(text, flags)
        if not found:
            QMessageBox.information(self, "Rechercher", f"« {text} » est introuvable.")
        return found

    def replace_one(self):
        editor = self._current_editor()
        find_text = self.find_edit.text()
        if editor is None or not find_text:
            return
        cursor = editor.textCursor()
        if cursor.hasSelection():
            selected = cursor.selectedText()
            matches = (
                selected == find_text
                if self.case_check.isChecked()
                else selected.lower() == find_text.lower()
            )
            if matches:
                cursor.insertText(self.replace_edit.text())
                editor.setTextCursor(cursor)
        self.find_next()

    def replace_all(self):
        editor = self._current_editor()
        find_text = self.find_edit.text()
        if editor is None or not find_text:
            return
        replace_text = self.replace_edit.text()
        flags = self._flags(backward=False)

        cursor = editor.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.Start)
        editor.setTextCursor(cursor)

        edit_cursor = editor.textCursor()
        edit_cursor.beginEditBlock()
        count = 0
        while editor.find(find_text, flags):
            match_cursor = editor.textCursor()
            match_cursor.insertText(replace_text)
            editor.setTextCursor(match_cursor)
            count += 1
        edit_cursor.endEditBlock()

        QMessageBox.information(self, "Tout remplacer", f"{count} occurrence(s) remplacée(s).")
