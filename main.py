#!/usr/bin/env python3
import os
import sys
from datetime import datetime

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QAction, QKeySequence
from PyQt6.QtWidgets import (
    QApplication,
    QFileDialog,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QSplitter,
    QTabBar,
    QTabWidget,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

import session
from drafts_browser import DraftsBrowser
from editor_widget import Editor
from find_replace import FindReplaceDialog


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Éditeur de texte")
        self.resize(900, 650)

        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(False)
        self.tabs.setMovable(True)
        self.tabs.setDocumentMode(True)
        self.tabs.setStyleSheet(
            """
            QTabBar::tab {
                background: #e1e1e1;
                color: #444444;
                padding: 6px 8px 6px 14px;
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
        self.tabs.currentChanged.connect(self.update_title)

        self.drafts_browser = DraftsBrowser()
        self.drafts_browser.open_requested.connect(self._open_draft)
        self.drafts_browser.delete_requested.connect(self._delete_draft)

        sidebar = QWidget()
        sidebar_layout = QVBoxLayout(sidebar)
        sidebar_layout.setContentsMargins(0, 0, 0, 0)
        sidebar_layout.setSpacing(0)
        sidebar_layout.addWidget(QLabel("Brouillons"))
        sidebar_layout.addWidget(self.drafts_browser)

        self.splitter = QSplitter()
        self.splitter.addWidget(sidebar)
        self.splitter.addWidget(self.tabs)
        self.splitter.setStretchFactor(0, 0)
        self.splitter.setStretchFactor(1, 1)
        self.splitter.setSizes([180, 720])
        self.setCentralWidget(self.splitter)

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

        self.about_action = QAction("À &propos...", self)
        self.about_action.triggered.connect(self.show_about)

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

        help_menu = menu.addMenu("&Aide")
        help_menu.addAction(self.about_action)

    def show_about(self):
        QMessageBox.about(
            self,
            "À propos",
            "<h3>Éditeur de texte</h3>"
            "<p>Éditeur de texte à onglets écrit en Python avec PyQt6.</p>"
            "<p>Numéros de ligne, coloration syntaxique, recherche/remplacement "
            "et restauration automatique de session.</p>",
        )

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
        self.tabs.tabBar().setTabButton(index, QTabBar.ButtonPosition.RightSide, self._make_close_button())
        self.tabs.setCurrentIndex(index)
        editor.setFocus()
        self._refresh_drafts_browser()
        return editor

    def _refresh_drafts_browser(self):
        open_ids = {self.tabs.widget(i).session_id for i in range(self.tabs.count())}
        self.drafts_browser.refresh(exclude_ids=open_ids)

    def _open_draft(self, entry):
        for i in range(self.tabs.count()):
            if self.tabs.widget(i).session_id == entry["id"]:
                self.tabs.setCurrentIndex(i)
                return
        content = session.read_draft(entry["id"])
        self.new_tab(
            file_path=entry.get("file_path"),
            content=content,
            default_name=entry.get("default_name"),
            session_id=entry["id"],
            modified=entry.get("modified", True),
        )

    def _delete_draft(self, entry):
        label = os.path.basename(entry["file_path"]) if entry.get("file_path") else entry.get("default_name") or entry["id"][:8]
        result = QMessageBox.question(
            self,
            "Supprimer le brouillon",
            f"Supprimer définitivement « {label} » ? Cette action est irréversible.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if result == QMessageBox.StandardButton.Yes:
            session.delete_draft(entry["id"])
            self._refresh_drafts_browser()

    def _make_close_button(self):
        button = QToolButton()
        button.setText("✕")
        button.setAutoRaise(True)
        button.setCursor(Qt.CursorShape.PointingHandCursor)
        button.setFixedSize(18, 18)
        button.setStyleSheet(
            """
            QToolButton {
                border: none;
                color: #777777;
                font-size: 11px;
                border-radius: 3px;
            }
            QToolButton:hover {
                color: #d32f2f;
                background: #dddddd;
            }
            """
        )
        button.clicked.connect(lambda: self._close_tab_by_button(button))

        wrapper = QWidget()
        layout = QHBoxLayout(wrapper)
        layout.setContentsMargins(0, 0, 8, 0)
        layout.addWidget(button)
        return wrapper

    def _close_tab_by_button(self, button):
        bar = self.tabs.tabBar()
        for i in range(bar.count()):
            widget = bar.tabButton(i, QTabBar.ButtonPosition.RightSide)
            if widget is not None and widget.findChild(QToolButton) is button:
                self.close_tab(i)
                return

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

    def close_tab(self, index):
        editor = self.tabs.widget(index)
        if editor is None:
            return
        if editor.document().isModified():
            session.save_draft(
                {
                    "id": editor.session_id,
                    "file_path": editor.file_path,
                    "default_name": editor.default_name,
                    "modified": True,
                    "content": editor.toPlainText(),
                }
            )

        self.tabs.removeTab(index)
        if self.tabs.count() == 0:
            self.new_tab()
        else:
            self._refresh_drafts_browser()

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
    app.setStyle("Fusion")
    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
