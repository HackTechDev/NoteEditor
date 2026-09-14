#!/usr/bin/env python3
import os
import sys
from datetime import datetime

from PyQt6.QtGui import QAction, QKeySequence
from PyQt6.QtWidgets import (
    QApplication,
    QFileDialog,
    QMainWindow,
    QMessageBox,
    QTabWidget,
)

import session
from editor_widget import Editor
from find_replace import FindReplaceDialog


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Éditeur de texte")
        self.resize(900, 650)

        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(True)
        self.tabs.setMovable(True)
        self.tabs.setDocumentMode(True)
        self.tabs.setStyleSheet(
            """
            QTabBar::tab {
                background: #e1e1e1;
                color: #444444;
                padding: 6px 14px;
                border: 1px solid #c4c4c4;
                border-bottom: none;
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
                margin-right: 2px;
            }
            QTabBar::tab:hover:!selected {
                background: #ececec;
            }
            QTabBar::tab:selected {
                background: #ffffff;
                color: #000000;
                font-weight: bold;
                border-top: 2px solid #2f6fdb;
                margin-top: -1px;
            }
            """
        )
        self.tabs.tabCloseRequested.connect(self.close_tab)
        self.tabs.currentChanged.connect(self.update_title)
        self.setCentralWidget(self.tabs)

        self.find_dialog = FindReplaceDialog(self)

        self._create_actions()
        self._create_menu()

        self.statusBar()
        self._restore_session()

    def _create_actions(self):
        self.new_action = QAction("&Nouveau", self)
        self.new_action.setShortcut(QKeySequence.StandardKey.New)
        self.new_action.triggered.connect(lambda: self.new_tab())

        self.open_action = QAction("&Ouvrir...", self)
        self.open_action.setShortcut(QKeySequence.StandardKey.Open)
        self.open_action.triggered.connect(self.open_file)

        self.save_action = QAction("&Enregistrer", self)
        self.save_action.setShortcut(QKeySequence.StandardKey.Save)
        self.save_action.triggered.connect(self.save_file)

        self.save_as_action = QAction("Enregistrer &sous...", self)
        self.save_as_action.setShortcut(QKeySequence.StandardKey.SaveAs)
        self.save_as_action.triggered.connect(self.save_file_as)

        self.close_tab_action = QAction("&Fermer l'onglet", self)
        self.close_tab_action.setShortcut(QKeySequence.StandardKey.Close)
        self.close_tab_action.triggered.connect(lambda: self.close_tab(self.tabs.currentIndex()))

        self.quit_action = QAction("&Quitter", self)
        self.quit_action.setShortcut(QKeySequence.StandardKey.Quit)
        self.quit_action.triggered.connect(self.close)

        self.undo_action = QAction("Annuler", self)
        self.undo_action.setShortcut(QKeySequence.StandardKey.Undo)
        self.undo_action.triggered.connect(lambda: self.current_editor().undo())

        self.redo_action = QAction("Rétablir", self)
        self.redo_action.setShortcut(QKeySequence.StandardKey.Redo)
        self.redo_action.triggered.connect(lambda: self.current_editor().redo())

        self.cut_action = QAction("Couper", self)
        self.cut_action.setShortcut(QKeySequence.StandardKey.Cut)
        self.cut_action.triggered.connect(lambda: self.current_editor().cut())

        self.copy_action = QAction("Copier", self)
        self.copy_action.setShortcut(QKeySequence.StandardKey.Copy)
        self.copy_action.triggered.connect(lambda: self.current_editor().copy())

        self.paste_action = QAction("Coller", self)
        self.paste_action.setShortcut(QKeySequence.StandardKey.Paste)
        self.paste_action.triggered.connect(lambda: self.current_editor().paste())

        self.select_all_action = QAction("Tout sélectionner", self)
        self.select_all_action.setShortcut(QKeySequence.StandardKey.SelectAll)
        self.select_all_action.triggered.connect(lambda: self.current_editor().selectAll())

        self.find_action = QAction("&Rechercher...", self)
        self.find_action.setShortcut(QKeySequence.StandardKey.Find)
        self.find_action.triggered.connect(self.find_dialog.show_for_find)

        self.replace_action = QAction("Rechercher / &Remplacer...", self)
        self.replace_action.setShortcut(QKeySequence("Ctrl+H"))
        self.replace_action.triggered.connect(self.find_dialog.show_for_replace)

        self.find_next_action = QAction("Suivant", self)
        self.find_next_action.setShortcut(QKeySequence.StandardKey.FindNext)
        self.find_next_action.triggered.connect(self.find_dialog.find_next)

    def _create_menu(self):
        menu = self.menuBar()

        file_menu = menu.addMenu("&Fichier")
        file_menu.addAction(self.new_action)
        file_menu.addAction(self.open_action)
        file_menu.addAction(self.save_action)
        file_menu.addAction(self.save_as_action)
        file_menu.addSeparator()
        file_menu.addAction(self.close_tab_action)
        file_menu.addAction(self.quit_action)

        edit_menu = menu.addMenu("&Édition")
        edit_menu.addAction(self.undo_action)
        edit_menu.addAction(self.redo_action)
        edit_menu.addSeparator()
        edit_menu.addAction(self.cut_action)
        edit_menu.addAction(self.copy_action)
        edit_menu.addAction(self.paste_action)
        edit_menu.addSeparator()
        edit_menu.addAction(self.select_all_action)

        search_menu = menu.addMenu("&Rechercher")
        search_menu.addAction(self.find_action)
        search_menu.addAction(self.replace_action)
        search_menu.addAction(self.find_next_action)

    def current_editor(self):
        return self.tabs.currentWidget()

    def new_tab(self, file_path=None, content="", default_name=None, session_id=None, modified=False):
        editor = Editor()
        if session_id:
            editor.session_id = session_id
        editor.setPlainText(content)
        editor.set_file_path(file_path)
        editor.default_name = None if file_path else (default_name or self._timestamp_name())
        editor.document().setModified(modified)
        editor.document().modificationChanged.connect(lambda _: self.update_title())

        label = os.path.basename(file_path) if file_path else editor.default_name
        index = self.tabs.addTab(editor, label)
        self.tabs.setCurrentIndex(index)
        editor.setFocus()
        return editor

    @staticmethod
    def _timestamp_name():
        now = datetime.now()
        return f"{now:%y%m%d_%H%M%S}{now.microsecond // 10000:02d}"

    def _restore_session(self):
        entries, active_id = session.load_session()
        if not entries:
            self.new_tab()
            return
        active_index = 0
        for i, entry in enumerate(entries):
            self.new_tab(
                file_path=entry.get("file_path"),
                content=entry.get("content", ""),
                default_name=entry.get("default_name"),
                session_id=entry.get("id"),
                modified=entry.get("modified", False),
            )
            if entry.get("id") == active_id:
                active_index = i
        self.tabs.setCurrentIndex(active_index)

    def tab_label(self, editor):
        name = os.path.basename(editor.file_path) if editor.file_path else editor.default_name
        return "*" + name if editor.document().isModified() else name

    def update_title(self):
        editor = self.current_editor()
        if editor is None:
            self.setWindowTitle("Éditeur de texte")
            return
        index = self.tabs.currentIndex()
        self.tabs.setTabText(index, self.tab_label(editor))
        self.setWindowTitle(f"{self.tab_label(editor)} — Éditeur de texte")

    def open_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Ouvrir un fichier", "", "Fichiers texte (*.txt);;Tous les fichiers (*)")
        if not path:
            return
        for i in range(self.tabs.count()):
            w = self.tabs.widget(i)
            if w.file_path == path:
                self.tabs.setCurrentIndex(i)
                return
        try:
            with open(path, "r", encoding="utf-8") as f:
                content = f.read()
        except OSError as e:
            QMessageBox.critical(self, "Erreur", f"Impossible d'ouvrir le fichier :\n{e}")
            return
        self.new_tab(file_path=path, content=content)

    def save_file(self):
        editor = self.current_editor()
        if editor is None:
            return False
        if editor.file_path is None:
            return self.save_file_as()
        return self._write_file(editor, editor.file_path)

    def save_file_as(self):
        editor = self.current_editor()
        if editor is None:
            return False
        start = editor.file_path or f"{editor.default_name}.txt"
        path, _ = QFileDialog.getSaveFileName(self, "Enregistrer sous", start, "Fichiers texte (*.txt);;Tous les fichiers (*)")
        if not path:
            return False
        return self._write_file(editor, path)

    def _write_file(self, editor, path):
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(editor.toPlainText())
        except OSError as e:
            QMessageBox.critical(self, "Erreur", f"Impossible d'enregistrer le fichier :\n{e}")
            return False
        editor.set_file_path(path)
        editor.document().setModified(False)
        self.update_title()
        return True

    def _try_close_tab(self, index):
        editor = self.tabs.widget(index)
        if editor is None:
            return True
        if editor.document().isModified():
            name = os.path.basename(editor.file_path) if editor.file_path else editor.default_name
            result = QMessageBox.question(
                self,
                "Modifications non enregistrées",
                f"« {name} » contient des modifications non enregistrées.\nVoulez-vous les enregistrer ?",
                QMessageBox.StandardButton.Save
                | QMessageBox.StandardButton.Discard
                | QMessageBox.StandardButton.Cancel,
            )
            if result == QMessageBox.StandardButton.Cancel:
                return False
            if result == QMessageBox.StandardButton.Save:
                self.tabs.setCurrentIndex(index)
                if not self.save_file():
                    return False

        self.tabs.removeTab(index)
        return True

    def close_tab(self, index):
        if self._try_close_tab(index) and self.tabs.count() == 0:
            self.new_tab()

    def _save_session(self):
        tabs_info = [
            {
                "id": self.tabs.widget(i).session_id,
                "file_path": self.tabs.widget(i).file_path,
                "default_name": self.tabs.widget(i).default_name,
                "modified": self.tabs.widget(i).document().isModified(),
                "content": self.tabs.widget(i).toPlainText(),
            }
            for i in range(self.tabs.count())
        ]
        active_editor = self.current_editor()
        active_id = active_editor.session_id if active_editor is not None else None
        session.save_session(tabs_info, active_id)

    def closeEvent(self, event):
        self._save_session()
        event.accept()


def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
