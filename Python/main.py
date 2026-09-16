#!/usr/bin/env python3
import os
import sys
from datetime import datetime

from PyQt6.QtCore import Qt, QPoint, QSize, QTimer
from PyQt6.QtGui import QAction, QIcon, QKeySequence, QPainter, QPen, QPixmap, QPolygon
from PyQt6.QtWidgets import (
    QApplication,
    QComboBox,
    QFileDialog,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMenu,
    QMessageBox,
    QPlainTextEdit,
    QSplitter,
    QTabBar,
    QTabWidget,
    QToolBar,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

import session
from drafts_browser import DraftsBrowser
from editor_widget import Editor
from find_replace import FindReplaceDialog
from trash_dialog import TrashDialog
from version_history_dialog import VersionHistoryDialog


def _word_wrap_icon():
    """Dessine une icône « lignes qui reviennent à la ligne » : pas de fichier
    externe à embarquer, cohérent avec les autres icônes de l'appli (+, ✕),
    de simples glyphes dessinés/textuels plutôt que des assets."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    painter.drawLine(3, 5, 19, 5)
    painter.drawLine(3, 11, 19, 11)
    painter.drawLine(3, 17, 13, 17)
    # petite flèche de retour à la ligne, à la fin de la 2e ligne
    painter.drawLine(19, 11, 19, 16)
    painter.drawLine(19, 16, 15, 16)
    painter.drawLine(15, 16, 17, 14)
    painter.drawLine(15, 16, 17, 18)
    painter.end()
    return QIcon(pixmap)


def _save_icon():
    """Dessine une icône « disquette » classique, dans le même esprit que
    _word_wrap_icon() : glyphes dessinés, pas de fichier externe."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    painter.setPen(pen)
    # corps de la disquette, coin supérieur droit coupé
    body = QPolygon(
        [
            QPoint(3, 3),
            QPoint(16, 3),
            QPoint(19, 6),
            QPoint(19, 19),
            QPoint(3, 19),
        ]
    )
    painter.drawPolygon(body)
    painter.drawRect(7, 3, 6, 6)  # volet métallique en haut
    painter.drawRect(6, 12, 10, 6)  # étiquette en bas
    painter.end()
    return QIcon(pixmap)


class _CornerToolButton(QToolButton):
    """QToolButton dont le sizeHint force sa taille, pour que QTabWidget le
    dimensionne correctement en widget de coin (il se base sur sizeHint(), pas
    sur une taille imposée après coup via setFixedSize). QTabWidget cale un
    widget de coin sur le BAS de la ligne d'onglets (jamais centré) : le
    rendre presque aussi haut que la barre (hauteur - 1, le maximum que Qt
    accepte sans écraser la hauteur à 0) est la seule façon de le faire
    paraître à peu près centré."""

    def __init__(self, height, parent=None):
        super().__init__(parent)
        self._forced_height = height

    def sizeHint(self):
        return QSize(super().sizeHint().width(), self._forced_height)


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
        self.tabs.currentChanged.connect(self._check_current_external_change)
        self.tabs.currentChanged.connect(self._update_status_bar)
        self.tabs.tabBar().setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.tabs.tabBar().customContextMenuRequested.connect(self._show_tab_context_menu)

        self.setAcceptDrops(True)
        QApplication.instance().applicationStateChanged.connect(self._on_app_state_changed)

        # Deux boutons "+" pour le même bouton logique :
        # - new_tab_button : enfant de la barre d'onglets, collé juste après le
        #   dernier onglet (comme Gedit) tant que les onglets tiennent dans la largeur.
        # - new_tab_corner_button : widget de coin du QTabWidget, dont Qt réserve
        #   automatiquement la place. Utilisé uniquement quand les onglets débordent
        #   (flèches de défilement visibles), cas où il n'y a plus aucun espace
        #   libre juste après le dernier onglet. QTabWidget se base sur son
        #   sizeHint() pour le centrer/dimensionner : _CornerToolButton force ce
        #   sizeHint() à la hauteur de la barre d'onglets pour un centrage correct
        #   (setFixedSize après coup ne fonctionne pas, Qt le réinitialise à 0).
        self.new_tab_button = self._build_new_tab_button(self.tabs.tabBar())
        bar_height = self.tabs.tabBar().sizeHint().height()
        self.new_tab_corner_button = self._build_new_tab_button(
            None, cls=lambda parent: _CornerToolButton(bar_height - 1, parent)
        )
        self.new_tab_corner_button.hide()
        self.tabs.setCornerWidget(self.new_tab_corner_button, Qt.Corner.TopRightCorner)
        self._reposition_new_tab_button()

        self.drafts_browser = DraftsBrowser()
        self.drafts_browser.open_requested.connect(self._open_draft)
        self.drafts_browser.delete_requested.connect(self._trash_draft)
        self.drafts_browser.rename_requested.connect(self._rename_draft_entry)

        self.drafts_search = QLineEdit()
        self.drafts_search.setPlaceholderText("Rechercher...")
        self.drafts_search.textChanged.connect(self.drafts_browser.set_filter_text)

        self.drafts_sort = QComboBox()
        self.drafts_sort.addItem("Date", "date")
        self.drafts_sort.addItem("Nom", "name")
        self.drafts_sort.currentIndexChanged.connect(
            lambda i: self.drafts_browser.set_sort_mode(self.drafts_sort.itemData(i))
        )

        drafts_toolbar = QHBoxLayout()
        drafts_toolbar.addWidget(self.drafts_search)
        drafts_toolbar.addWidget(self.drafts_sort)

        self.trash_button = QToolButton()
        self.trash_button.setText("Corbeille...")
        self.trash_button.clicked.connect(self._show_trash)

        sidebar = QWidget()
        sidebar_layout = QVBoxLayout(sidebar)
        sidebar_layout.setContentsMargins(0, 0, 0, 0)
        sidebar_layout.setSpacing(4)
        sidebar_layout.addWidget(QLabel("Brouillons"))
        sidebar_layout.addLayout(drafts_toolbar)
        sidebar_layout.addWidget(self.drafts_browser)
        sidebar_layout.addWidget(self.trash_button)

        self.splitter = QSplitter()
        self.splitter.addWidget(sidebar)
        self.splitter.addWidget(self.tabs)
        self.splitter.setStretchFactor(0, 0)
        self.splitter.setStretchFactor(1, 1)
        self.splitter.setSizes([180, 720])
        self.setCentralWidget(self.splitter)

        window_state = session.load_window_state()
        if window_state:
            width = window_state.get("width")
            height = window_state.get("height")
            if width and height:
                self.resize(width, height)
            sizes = window_state.get("splitter_sizes")
            if sizes:
                self.splitter.setSizes(sizes)

        self.find_dialog = FindReplaceDialog(self)
        self.word_wrap_enabled = True

        self._create_actions()
        self._create_menu()
        self._create_toolbar()
        self._create_status_bar()

        self._restore_session()

    def _build_new_tab_button(self, parent, cls=None):
        button = cls(parent) if cls else QToolButton(parent)
        button.setText("+")
        button.setAutoRaise(True)
        button.setCursor(Qt.CursorShape.PointingHandCursor)
        button.setToolTip("Nouvel onglet")
        if cls is None:
            button.setFixedSize(24, 24)
        button.setStyleSheet(
            """
            QToolButton {
                border: none;
                color: #444444;
                font-size: 15px;
                font-weight: bold;
                border-radius: 3px;
            }
            QToolButton:hover {
                color: #2f6fdb;
                background: #dddddd;
            }
            """
        )
        button.clicked.connect(lambda: self.new_tab())
        return button

    def _create_actions(self):
        self.new_action = QAction("&Nouveau", self)
        self.new_action.setShortcut(QKeySequence.StandardKey.New)
        self.new_action.triggered.connect(lambda: self.new_tab())

        self.open_action = QAction("&Ouvrir...", self)
        self.open_action.setShortcut(QKeySequence.StandardKey.Open)
        self.open_action.triggered.connect(self.open_file)

        self.save_action = QAction(_save_icon(), "&Enregistrer", self)
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

        self.trash_action = QAction("&Corbeille...", self)
        self.trash_action.triggered.connect(self._show_trash)

        self.word_wrap_action = QAction(_word_wrap_icon(), "Retour automatique à la ligne", self)
        self.word_wrap_action.setCheckable(True)
        self.word_wrap_action.setChecked(True)
        self.word_wrap_action.toggled.connect(self._set_word_wrap)

    def _create_toolbar(self):
        toolbar = QToolBar("Barre d'outils", self)
        toolbar.setMovable(False)
        toolbar.addAction(self.save_action)
        toolbar.addSeparator()
        toolbar.addAction(self.word_wrap_action)
        self.addToolBar(toolbar)

    def _set_word_wrap(self, enabled):
        self.word_wrap_enabled = enabled
        mode = QPlainTextEdit.LineWrapMode.WidgetWidth if enabled else QPlainTextEdit.LineWrapMode.NoWrap
        for i in range(self.tabs.count()):
            self.tabs.widget(i).setLineWrapMode(mode)

    def _create_status_bar(self):
        self.status_position = QLabel()
        self.status_counts = QLabel()
        self.status_encoding = QLabel("UTF-8")
        bar = self.statusBar()
        bar.addPermanentWidget(self.status_position)
        bar.addPermanentWidget(self.status_counts)
        bar.addPermanentWidget(self.status_encoding)
        self._update_status_bar()

    def _update_status_bar(self):
        editor = self.current_editor()
        if editor is None:
            self.status_position.setText("")
            self.status_counts.setText("")
            self.status_encoding.setText("")
            return
        cursor = editor.textCursor()
        line = cursor.blockNumber() + 1
        col = cursor.columnNumber() + 1
        self.status_position.setText(f"Ligne {line}, Colonne {col}")

        text = editor.toPlainText()
        words = len(text.split())
        chars = len(text)
        word_label = "mot" if words <= 1 else "mots"
        char_label = "caractère" if chars <= 1 else "caractères"
        self.status_counts.setText(f"{words} {word_label}, {chars} {char_label}")

        self.status_encoding.setText("UTF-8")

    def _create_menu(self):
        menu = self.menuBar()

        file_menu = menu.addMenu("&Fichier")
        file_menu.addAction(self.new_action)
        file_menu.addAction(self.open_action)
        file_menu.addAction(self.save_action)
        file_menu.addAction(self.save_as_action)
        file_menu.addSeparator()
        file_menu.addAction(self.trash_action)
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
        editor.autosave_requested.connect(lambda: self._autosave_tab(editor))
        editor.cursorPositionChanged.connect(self._update_status_bar)
        editor.textChanged.connect(self._update_status_bar)
        editor.setLineWrapMode(
            QPlainTextEdit.LineWrapMode.WidgetWidth if self.word_wrap_enabled else QPlainTextEdit.LineWrapMode.NoWrap
        )

        label = os.path.basename(file_path) if file_path else editor.default_name
        index = self.tabs.addTab(editor, label)
        self.tabs.tabBar().setTabButton(index, QTabBar.ButtonPosition.RightSide, self._make_close_button())
        self.tabs.setCurrentIndex(index)
        editor.setFocus()

        session.save_draft(
            {
                "id": editor.session_id,
                "file_path": editor.file_path,
                "default_name": editor.default_name,
                "modified": editor.document().isModified(),
                "content": editor.toPlainText(),
            }
        )
        self._refresh_drafts_browser()
        self._reposition_new_tab_button()
        return editor

    def _reposition_new_tab_button(self):
        QTimer.singleShot(0, self._toggle_new_tab_button_mode)

    def _native_scroll_buttons(self):
        # bar.children() (non-recursive) holds the native scroll-arrow buttons
        # (only present when tabs overflow) as direct QToolButton children;
        # per-tab close buttons live one level deeper, inside their wrapper.
        bar = self.tabs.tabBar()
        return [c for c in bar.children() if isinstance(c, QToolButton) and c is not self.new_tab_button and c.isVisible()]

    def _toggle_new_tab_button_mode(self):
        overflowing = bool(self._native_scroll_buttons())
        self.new_tab_button.setVisible(not overflowing)
        if overflowing:
            # la hauteur de la barre n'est fiable qu'une fois des onglets
            # présents (0 à la construction) : on la recalcule ici et on force
            # Qt à requêter le sizeHint mis à jour avant d'afficher le bouton.
            bar_height = self.tabs.tabBar().height()
            self.new_tab_corner_button._forced_height = max(1, bar_height - 1)
            self.new_tab_corner_button.updateGeometry()
        self.new_tab_corner_button.setVisible(overflowing)
        if overflowing:
            return
        QTimer.singleShot(0, self._position_inline_new_tab_button)

    def _position_inline_new_tab_button(self):
        if self._native_scroll_buttons():
            return
        bar = self.tabs.tabBar()
        button_width = self.new_tab_button.width()
        after_tabs_x = bar.tabRect(bar.count() - 1).right() + 4 if bar.count() > 0 else 4
        max_x = bar.width() - button_width - 4
        x = max(4, min(after_tabs_x, max_x))

        y = max(0, (bar.height() - self.new_tab_button.height()) // 2)
        self.new_tab_button.move(x, y)

    def _refresh_drafts_browser(self):
        open_ids = {self.tabs.widget(i).session_id for i in range(self.tabs.count())}
        self.drafts_browser.refresh(open_ids=open_ids)
        self._highlight_active_draft()

    def _highlight_active_draft(self):
        editor = self.current_editor()
        if editor is not None:
            self.drafts_browser.select_draft(editor.session_id)
        else:
            self.drafts_browser.setCurrentItem(None)

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

    def _trash_draft(self, entry):
        label = os.path.basename(entry["file_path"]) if entry.get("file_path") else entry.get("default_name") or entry["id"][:8]
        result = QMessageBox.question(
            self,
            "Mettre à la corbeille",
            f"Mettre « {label} » à la corbeille ?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if result == QMessageBox.StandardButton.Yes:
            session.trash_draft(entry["id"])
            self._refresh_drafts_browser()

    def _show_trash(self):
        dialog = TrashDialog(self)
        dialog.restored.connect(self._refresh_drafts_browser)
        dialog.exec()

    def _autosave_tab(self, editor):
        if self.tabs.indexOf(editor) == -1:
            return  # l'onglet a été fermé avant que le délai ne s'écoule
        session.save_draft(
            {
                "id": editor.session_id,
                "file_path": editor.file_path,
                "default_name": editor.default_name,
                "modified": editor.document().isModified(),
                "content": editor.toPlainText(),
            }
        )

    def _rename_tab(self, index):
        editor = self.tabs.widget(index)
        if editor is None or editor.file_path is not None:
            return
        new_name, ok = QInputDialog.getText(self, "Renommer", "Nouveau nom :", text=editor.default_name)
        new_name = new_name.strip()
        if not ok or not new_name:
            return
        editor.default_name = new_name
        session.save_draft(
            {
                "id": editor.session_id,
                "file_path": editor.file_path,
                "default_name": editor.default_name,
                "modified": editor.document().isModified(),
                "content": editor.toPlainText(),
            }
        )
        self.update_title()
        self._refresh_drafts_browser()

    def _rename_draft_entry(self, entry):
        for i in range(self.tabs.count()):
            if self.tabs.widget(i).session_id == entry["id"]:
                self._rename_tab(i)
                return
        new_name, ok = QInputDialog.getText(self, "Renommer", "Nouveau nom :", text=entry.get("default_name") or "")
        new_name = new_name.strip()
        if not ok or not new_name:
            return
        session.rename_draft(entry["id"], new_name)
        self._refresh_drafts_browser()

    def _duplicate_tab(self, index):
        editor = self.tabs.widget(index)
        if editor is None:
            return
        self.new_tab(content=editor.toPlainText())

    def _close_other_tabs(self, index):
        keep = self.tabs.widget(index)
        for i in reversed(range(self.tabs.count())):
            if self.tabs.widget(i) is not keep:
                self.close_tab(i)

    def _close_all_tabs(self):
        for i in reversed(range(self.tabs.count())):
            self.close_tab(i)

    def _close_tabs_to_the_right(self, index):
        for i in reversed(range(index + 1, self.tabs.count())):
            self.close_tab(i)

    def _show_version_history(self, editor):
        dialog = VersionHistoryDialog(editor.session_id, self)
        dialog.restore_requested.connect(lambda content: self._apply_restored_version(editor, content))
        dialog.exec()

    def _apply_restored_version(self, editor, content):
        editor.setPlainText(content)
        editor.document().setModified(True)
        self.update_title()

    def _show_tab_context_menu(self, pos):
        bar = self.tabs.tabBar()
        index = bar.tabAt(pos)
        if index == -1:
            return
        editor = self.tabs.widget(index)

        menu = QMenu(self)
        close_action = menu.addAction("Fermer")
        close_others_action = menu.addAction("Fermer les autres")
        close_right_action = menu.addAction("Fermer à droite")
        close_right_action.setEnabled(index < self.tabs.count() - 1)
        close_all_action = menu.addAction("Fermer tout")
        menu.addSeparator()
        duplicate_action = menu.addAction("Dupliquer")
        rename_action = menu.addAction("Renommer...") if editor.file_path is None else None
        history_action = menu.addAction("Historique des versions...") if session.list_versions(editor.session_id) else None

        chosen = menu.exec(bar.mapToGlobal(pos))
        if chosen == close_action:
            self.close_tab(index)
        elif chosen == close_others_action:
            self._close_other_tabs(index)
        elif chosen == close_right_action:
            self._close_tabs_to_the_right(index)
        elif chosen == close_all_action:
            self._close_all_tabs()
        elif chosen == duplicate_action:
            self._duplicate_tab(index)
        elif rename_action is not None and chosen == rename_action:
            self._rename_tab(index)
        elif history_action is not None and chosen == history_action:
            self._show_version_history(editor)

    def _check_current_external_change(self):
        self._check_external_change(self.current_editor())

    def _on_app_state_changed(self, state):
        if state == Qt.ApplicationState.ApplicationActive:
            self._check_current_external_change()

    def _check_external_change(self, editor):
        if editor is None or not editor.file_path:
            return
        try:
            mtime = os.path.getmtime(editor.file_path)
        except OSError:
            return
        if editor.disk_mtime is None:
            editor.disk_mtime = mtime
            return
        if mtime <= editor.disk_mtime:
            return
        editor.disk_mtime = mtime
        result = QMessageBox.warning(
            self,
            "Fichier modifié en dehors de l'éditeur",
            f"« {os.path.basename(editor.file_path)} » a été modifié par un autre programme.\n"
            "Voulez-vous recharger son contenu depuis le disque ? "
            "Les modifications non enregistrées dans cet onglet seront perdues.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if result != QMessageBox.StandardButton.Yes:
            return
        try:
            with open(editor.file_path, "r", encoding="utf-8") as f:
                content = f.read()
        except OSError as e:
            QMessageBox.critical(self, "Erreur", f"Impossible de recharger le fichier :\n{e}")
            return
        editor.setPlainText(content)
        editor.document().setModified(False)
        self.update_title()

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event):
        for url in event.mimeData().urls():
            path = url.toLocalFile()
            if path:
                self._open_path(path)
        event.acceptProposedAction()

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
            editor = self.new_tab(
                file_path=entry.get("file_path"),
                content=entry.get("content", ""),
                default_name=entry.get("default_name"),
                session_id=entry.get("id"),
                modified=entry.get("modified", False),
            )
            if entry.get("file_path") and os.path.isfile(entry["file_path"]):
                try:
                    editor.disk_mtime = os.path.getmtime(entry["file_path"])
                except OSError:
                    pass
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
            self._highlight_active_draft()
            return
        index = self.tabs.currentIndex()
        self.tabs.setTabText(index, self.tab_label(editor))
        self.setWindowTitle(f"{self.tab_label(editor)} — Éditeur de texte")
        self._highlight_active_draft()

    def open_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Ouvrir un fichier", "", "Fichiers texte (*.txt);;Tous les fichiers (*)")
        if path:
            self._open_path(path)

    def _open_path(self, path):
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
        editor = self.new_tab(file_path=path, content=content)
        try:
            editor.disk_mtime = os.path.getmtime(path)
        except OSError:
            pass

    def save_file(self):
        editor = self.current_editor()
        if editor is None:
            return False
        if editor.file_path is None:
            os.makedirs(session.DOCS_DIR, exist_ok=True)
            path = os.path.join(session.DOCS_DIR, f"{editor.default_name}.txt")
            return self._write_file(editor, path)
        return self._write_file(editor, editor.file_path)

    def save_file_as(self):
        editor = self.current_editor()
        if editor is None:
            return False
        start = editor.file_path or os.path.join(session.DOCS_DIR, f"{editor.default_name}.txt")
        path, _ = QFileDialog.getSaveFileName(self, "Enregistrer sous", start, "Fichiers texte (*.txt);;Tous les fichiers (*)")
        if not path:
            return False
        return self._write_file(editor, path)

    def _write_file(self, editor, path):
        if os.path.isfile(path):
            try:
                with open(path, "r", encoding="utf-8") as f:
                    previous_content = f.read()
            except OSError:
                previous_content = None
            if previous_content is not None:
                session.save_version(editor.session_id, previous_content)
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(editor.toPlainText())
        except OSError as e:
            QMessageBox.critical(self, "Erreur", f"Impossible d'enregistrer le fichier :\n{e}")
            return False
        editor.set_file_path(path)
        editor.document().setModified(False)
        try:
            editor.disk_mtime = os.path.getmtime(path)
        except OSError:
            pass
        session.save_draft(
            {
                "id": editor.session_id,
                "file_path": editor.file_path,
                "default_name": editor.default_name,
                "modified": False,
                "content": editor.toPlainText(),
            }
        )
        self.update_title()
        self._refresh_drafts_browser()
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
        self._refresh_drafts_browser()
        self._reposition_new_tab_button()
        self.update_title()

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
        session.save_window_state(self.width(), self.height(), self.splitter.sizes())
        event.accept()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._reposition_new_tab_button()


def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
