#!/usr/bin/env python3
import os
import sys
from datetime import datetime

from PyQt6.QtCore import QEvent, Qt, QPoint, QRect, QSize, QTimer, QUrl
from PyQt6.QtGui import QAction, QDesktopServices, QIcon, QKeySequence, QPainter, QPen, QPixmap, QPolygon
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
    QTextBrowser,
    QToolBar,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

import session
from drafts_browser import REMEMBER_TOOLTIP, DraftsBrowser, path_tooltip, pin_pixmap, unpin_pixmap
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


def _new_note_icon():
    """Dessine une icône « page + » (nouvelle note), dans le même esprit que
    _word_wrap_icon() / _save_icon() : glyphes dessinés, pas de fichier
    externe."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    painter.setPen(pen)
    # feuille de papier, coin supérieur droit corné
    page = QPolygon(
        [
            QPoint(5, 2),
            QPoint(14, 2),
            QPoint(18, 6),
            QPoint(18, 20),
            QPoint(5, 20),
        ]
    )
    painter.drawPolygon(page)
    painter.drawLine(14, 2, 14, 6)
    painter.drawLine(14, 6, 18, 6)
    # « + » au centre bas de la feuille
    painter.drawLine(11, 11, 11, 17)
    painter.drawLine(8, 14, 14, 14)
    painter.end()
    return QIcon(pixmap)


def _open_icon():
    """Dessine une icône « dossier » classique, dans le même esprit que
    _new_note_icon() / _save_icon() : glyphes dessinés, pas de fichier
    externe."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    painter.setPen(pen)
    folder = QPolygon(
        [
            QPoint(2, 6),
            QPoint(9, 6),
            QPoint(11, 8),
            QPoint(20, 8),
            QPoint(20, 18),
            QPoint(2, 18),
        ]
    )
    painter.drawPolygon(folder)
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


def _save_as_icon():
    """Dessine une icône « disquette + flèche » (Enregistrer sous), dans le
    même esprit que _save_icon() : une disquette réduite, avec une petite
    flèche vers un autre emplacement pour la distinguer d'Enregistrer."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    # disquette réduite, en haut à gauche
    body = QPolygon(
        [
            QPoint(2, 3),
            QPoint(12, 3),
            QPoint(15, 6),
            QPoint(15, 15),
            QPoint(2, 15),
        ]
    )
    painter.drawPolygon(body)
    painter.drawRect(6, 3, 5, 4)  # volet métallique en haut
    painter.drawRect(5, 10, 7, 4)  # étiquette en bas
    # petite flèche vers un autre emplacement, en bas à droite
    painter.drawLine(14, 14, 20, 20)
    painter.drawLine(20, 20, 20, 15)
    painter.drawLine(20, 20, 15, 20)
    painter.end()
    return QIcon(pixmap)


def _find_icon():
    """Dessine une icône « loupe » classique, dans le même esprit que
    _save_icon() : glyphes dessinés, pas de fichier externe."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    painter.drawEllipse(3, 3, 11, 11)
    painter.drawLine(13, 13, 19, 19)
    painter.end()
    return QIcon(pixmap)


def _replace_icon():
    """Dessine une icône « loupe + flèche » (Rechercher / Remplacer), dans le
    même esprit que _find_icon() : une loupe réduite, avec une petite flèche
    vers la droite (« remplacer par ») pour la distinguer de Rechercher."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    painter.drawEllipse(2, 2, 9, 9)
    painter.drawLine(10, 10, 13, 13)
    # petite flèche vers la droite, en bas
    painter.drawLine(11, 17, 20, 17)
    painter.drawLine(17, 14, 20, 17)
    painter.drawLine(17, 20, 20, 17)
    painter.end()
    return QIcon(pixmap)


def _close_tab_icon():
    """Dessine une icône « onglet + croix » (Fermer l'onglet), dans le même
    esprit que _save_icon() : un onglet aux coins supérieurs coupés, avec une
    croix à l'intérieur, pour la distinguer des autres icônes de page."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    tab = QPolygon(
        [
            QPoint(3, 19),
            QPoint(3, 6),
            QPoint(6, 3),
            QPoint(16, 3),
            QPoint(19, 6),
            QPoint(19, 19),
        ]
    )
    painter.drawPolygon(tab)
    painter.drawLine(8, 9, 14, 15)
    painter.drawLine(14, 9, 8, 15)
    painter.end()
    return QIcon(pixmap)


def _preview_icon():
    """Dessine une icône « fenêtre coupée en deux » (aperçu Markdown) : du texte
    à gauche, son rendu (un titre et des lignes) à droite. Glyphes dessinés,
    pas de fichier externe."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    painter.drawRect(2, 4, 18, 14)
    painter.drawLine(11, 4, 11, 18)
    painter.drawLine(5, 8, 8, 8)  # texte brut, à gauche
    painter.drawLine(5, 11, 8, 11)
    painter.drawLine(5, 14, 7, 14)
    painter.drawLine(14, 8, 17, 8)  # rendu, à droite
    painter.drawLine(14, 11, 17, 11)
    painter.drawLine(14, 14, 16, 14)
    painter.end()
    return QIcon(pixmap)


def _drafts_panel_icon():
    """Dessine une icône « fenêtre avec panneau latéral » (panneau Brouillons) : la
    colonne de gauche est pleine, l'éditeur à droite. Glyphes dessinés, pas de
    fichier externe."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    painter.drawRect(2, 4, 18, 14)
    painter.fillRect(3, 5, 5, 12, Qt.GlobalColor.darkGray)  # le panneau
    painter.drawLine(8, 4, 8, 18)
    painter.drawLine(11, 8, 17, 8)  # le texte de l'éditeur
    painter.drawLine(11, 11, 17, 11)
    painter.drawLine(11, 14, 15, 14)
    painter.end()
    return QIcon(pixmap)


def _trash_icon():
    """Dessine une icône « poubelle » classique, dans le même esprit que
    _save_icon() : glyphes dessinés, pas de fichier externe."""
    pixmap = QPixmap(22, 22)
    pixmap.fill(Qt.GlobalColor.transparent)
    painter = QPainter(pixmap)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing)
    pen = QPen(Qt.GlobalColor.darkGray)
    pen.setWidth(2)
    pen.setJoinStyle(Qt.PenJoinStyle.RoundJoin)
    pen.setCapStyle(Qt.PenCapStyle.RoundCap)
    painter.setPen(pen)
    painter.drawLine(3, 6, 19, 6)  # couvercle
    painter.drawPolyline(QPolygon([QPoint(8, 6), QPoint(8, 3), QPoint(14, 3), QPoint(14, 6)]))  # poignée
    painter.drawPolygon(QPolygon([QPoint(5, 6), QPoint(6, 19), QPoint(16, 19), QPoint(17, 6)]))  # cuve
    painter.drawLine(9, 10, 9, 15)
    painter.drawLine(13, 10, 13, 15)
    painter.end()
    return QIcon(pixmap)


FILE_FILTERS = (
    "Fichiers texte et Markdown (*.txt *.md);;Fichiers texte (*.txt);;"
    "Fichiers Markdown (*.md);;Tous les fichiers (*)"
)


def _save_filter_for(path):
    """Filtre initial de « Enregistrer sous » : celui qui montre le fichier courant."""
    ext = os.path.splitext(path)[1].lower()
    if ext == ".md":
        return "Fichiers Markdown (*.md)"
    if ext in ("", ".txt"):
        return "Fichiers texte (*.txt)"
    return "Tous les fichiers (*)"


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
        self.setWindowTitle("Éditeur de note")
        self.resize(900, 650)
        # Qt n'affiche pas les infobulles d'une fenêtre inactive (focus ailleurs,
        # lancement automatique au démarrage...) : on les veut toujours.
        self.setAttribute(Qt.WidgetAttribute.WA_AlwaysShowToolTips, True)

        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(False)
        self.tabs.setMovable(True)
        self.tabs.setDocumentMode(True)
        self.tabs.tabBar().setIconSize(QSize(14, 14))
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
        self._normalizing_tabs = False
        # les onglets épinglés restent groupés à gauche : après un glisser-déposer
        # on remet l'ordre en place au relâchement de la souris (pas pendant le geste)
        self.tabs.tabBar().installEventFilter(self)
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
        self.drafts_browser.action_requested.connect(self._handle_drafts_context_action)
        self.drafts_browser.bulk_action_requested.connect(self._handle_bulk_action)

        self.drafts_search = QLineEdit()
        self.drafts_search.setPlaceholderText("Rechercher...")
        self.drafts_search.setToolTip("Filtre les notes par nom ou par contenu")
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
        self.trash_button.setIcon(_trash_icon())
        self.trash_button.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonTextBesideIcon)
        self.trash_button.clicked.connect(self._show_trash)

        sidebar = QWidget()
        sidebar_layout = QVBoxLayout(sidebar)
        sidebar_layout.setContentsMargins(0, 0, 0, 0)
        sidebar_layout.setSpacing(4)
        sidebar_layout.addWidget(QLabel("Brouillons"))
        sidebar_layout.addLayout(drafts_toolbar)
        sidebar_layout.addWidget(self.drafts_browser)
        sidebar_layout.addWidget(self.trash_button)

        # Le volet d'aperçu est unique et partagé (il suit l'onglet actif), dans un
        # splitter interne : le splitter principal garde deux entrées, ce qui
        # préserve le format de window.json.
        self.preview = QTextBrowser()
        self.preview.setOpenExternalLinks(True)
        self.preview.hide()
        self._preview_key = None
        self._preview_timer = QTimer(self)
        self._preview_timer.setSingleShot(True)
        self._preview_timer.setInterval(250)
        self._preview_timer.timeout.connect(self._render_preview)
        self.editor_splitter = QSplitter()
        self.editor_splitter.addWidget(self.tabs)
        self.editor_splitter.addWidget(self.preview)
        self.editor_splitter.setStretchFactor(0, 1)
        self.editor_splitter.setStretchFactor(1, 1)
        self.editor_splitter.setSizes([1, 1])

        self.splitter = QSplitter()
        self.splitter.addWidget(sidebar)
        self.splitter.addWidget(self.editor_splitter)
        self.splitter.setStretchFactor(0, 0)
        self.splitter.setStretchFactor(1, 1)
        self.splitter.setSizes([180, 720])
        self.setCentralWidget(self.splitter)

        self._drafts_panel_width = 180  # largeur du panneau avant qu'on le masque
        window_state = session.load_window_state()
        if window_state:
            width = window_state.get("width")
            height = window_state.get("height")
            if width and height:
                self.resize(width, height)
                x = window_state.get("x")
                y = window_state.get("y")
                if x is not None and y is not None and self._is_visible_on_a_screen(x, y, width, height):
                    self.move(x, y)
            sizes = window_state.get("splitter_sizes")
            if sizes:
                self.splitter.setSizes(sizes)
                self._drafts_panel_width = sizes[0] or self._drafts_panel_width

        self.find_dialog = FindReplaceDialog(self)
        self.word_wrap_enabled = True

        self._create_actions()
        self._create_menu()
        self._create_toolbar()
        self._create_status_bar()

        # réglages d'affichage mémorisés (les valeurs par défaut sont : retour à la
        # ligne activé, aperçu Markdown masqué)
        prefs = window_state or {}
        if prefs.get("sidebar_visible") is False:
            self.drafts_panel_action.setChecked(False)
        if prefs.get("word_wrap") is False:
            self.word_wrap_action.setChecked(False)
        if prefs.get("markdown_preview"):
            self.preview_action.setChecked(True)

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
        self.new_action = QAction(_new_note_icon(), "&Nouveau", self)
        self.new_action.setShortcut(QKeySequence.StandardKey.New)
        self.new_action.triggered.connect(lambda: self.new_tab())

        self.open_action = QAction(_open_icon(), "&Ouvrir...", self)
        self.open_action.setShortcut(QKeySequence.StandardKey.Open)
        self.open_action.triggered.connect(self.open_file)

        self.save_action = QAction(_save_icon(), "&Enregistrer", self)
        self.save_action.setShortcut(QKeySequence.StandardKey.Save)
        self.save_action.triggered.connect(self.save_file)

        self.save_as_action = QAction(_save_as_icon(), "Enregistrer &sous...", self)
        self.save_as_action.setShortcut(QKeySequence.StandardKey.SaveAs)
        self.save_as_action.triggered.connect(self.save_file_as)

        self.close_tab_action = QAction(_close_tab_icon(), "&Fermer l'onglet", self)
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

        self.find_action = QAction(_find_icon(), "&Rechercher...", self)
        self.find_action.setShortcut(QKeySequence.StandardKey.Find)
        self.find_action.triggered.connect(self.find_dialog.show_for_find)

        self.replace_action = QAction(_replace_icon(), "Rechercher / &Remplacer...", self)
        self.replace_action.setShortcut(QKeySequence("Ctrl+H"))
        self.replace_action.triggered.connect(self.find_dialog.show_for_replace)

        self.find_next_action = QAction("Suivant", self)
        self.find_next_action.setShortcut(QKeySequence.StandardKey.FindNext)
        self.find_next_action.triggered.connect(self.find_dialog.find_next)

        self.about_action = QAction("À &propos...", self)
        self.about_action.triggered.connect(self.show_about)

        self.pin_action = QAction(QIcon(pin_pixmap(22)), "Épingler l'onglet", self)
        self.pin_action.triggered.connect(lambda: self._set_current_pinned(True))

        self.unpin_action = QAction(QIcon(unpin_pixmap(22)), "Détacher l'onglet", self)
        self.unpin_action.triggered.connect(lambda: self._set_current_pinned(False))

        self.trash_action = QAction(_trash_icon(), "&Corbeille...", self)
        self.trash_action.triggered.connect(self._show_trash)

        self.drafts_panel_action = QAction(_drafts_panel_icon(), "Afficher / masquer le panneau Brouillons", self)
        self.drafts_panel_action.setCheckable(True)
        self.drafts_panel_action.setChecked(True)
        self.drafts_panel_action.toggled.connect(self._set_drafts_panel_visible)

        self.word_wrap_action = QAction(_word_wrap_icon(), "Retour automatique à la ligne", self)
        self.word_wrap_action.setCheckable(True)
        self.word_wrap_action.setChecked(True)
        self.word_wrap_action.toggled.connect(self._set_word_wrap)

        self.preview_action = QAction(_preview_icon(), "Aperçu Markdown", self)
        self.preview_action.setCheckable(True)
        self.preview_action.setEnabled(False)
        self.preview_action.toggled.connect(lambda _: self._update_preview_state())

    def _create_toolbar(self):
        toolbar = QToolBar("Barre d'outils", self)
        toolbar.setMovable(False)
        toolbar.addAction(self.new_action)
        toolbar.addAction(self.open_action)
        toolbar.addAction(self.save_action)
        toolbar.addAction(self.save_as_action)
        toolbar.addAction(self.close_tab_action)
        toolbar.addAction(self.pin_action)
        toolbar.addAction(self.unpin_action)
        toolbar.addAction(self.trash_action)
        toolbar.addSeparator()
        toolbar.addAction(self.find_action)
        toolbar.addAction(self.replace_action)
        toolbar.addSeparator()
        toolbar.addAction(self.drafts_panel_action)
        toolbar.addAction(self.word_wrap_action)
        toolbar.addAction(self.preview_action)
        self.addToolBar(toolbar)

    @staticmethod
    def _is_markdown(editor):
        return (
            editor is not None
            and editor.file_path is not None
            and os.path.splitext(editor.file_path)[1].lower() in (".md", ".markdown")
        )

    def _update_preview_state(self):
        """Le volet n'est visible que si l'aperçu est activé ET que l'onglet actif
        est un fichier Markdown ; l'icône n'est active que pour un tel onglet."""
        editor = self.current_editor()
        markdown = self._is_markdown(editor)
        self.preview_action.setEnabled(markdown)
        show = markdown and self.preview_action.isChecked()
        was_visible = self.preview.isVisible()
        same_note = markdown and self._preview_key is not None and self._preview_key[0] == editor.session_id
        self.preview.setVisible(show)
        if show:
            if was_visible and same_note:
                # simple changement d'état (ex. « modifié ») : rendu différé comme à la frappe
                self._preview_timer.start()
            else:
                self._render_preview()

    def _schedule_preview(self):
        if self.preview.isVisible():
            self._preview_timer.start()

    def _render_preview(self):
        editor = self.current_editor()
        if not self._is_markdown(editor) or not self.preview.isVisible():
            return
        text = editor.toPlainText()
        key = (editor.session_id, editor.file_path, text)
        if key == self._preview_key:
            return
        same_note = self._preview_key is not None and self._preview_key[0] == editor.session_id
        bar = self.preview.verticalScrollBar()
        scroll = bar.value() if same_note else 0
        self._preview_key = key
        # les images et liens relatifs se résolvent depuis le dossier du fichier
        self.preview.setSearchPaths([os.path.dirname(editor.file_path)])
        self.preview.setMarkdown(text)
        bar.setValue(scroll)

    def _set_drafts_panel_visible(self, visible):
        """Affiche ou masque le panneau Brouillons ; il retrouve sa largeur en revenant."""
        panel = self.splitter.widget(0)
        if not visible:
            # (fenêtre pas encore affichée, au lancement : les tailles ne sont pas fiables)
            if self.isVisible() and not panel.isHidden() and self.splitter.sizes()[0] > 0:
                self._drafts_panel_width = self.splitter.sizes()[0]
            panel.hide()
        else:
            panel.show()
            total = self.splitter.width() - self.splitter.handleWidth()
            self.splitter.setSizes([self._drafts_panel_width, max(1, total - self._drafts_panel_width)])
        editor = self.current_editor()
        if editor is not None:
            editor.setFocus()

    def _splitter_sizes_to_save(self):
        sizes = self.splitter.sizes()
        if self.splitter.widget(0).isHidden():
            # la largeur d'avant le masquage, prise sur celle de l'éditeur
            sizes = [self._drafts_panel_width, max(1, sizes[1] - self._drafts_panel_width - self.splitter.handleWidth())]
        return sizes

    def _set_word_wrap(self, enabled):
        self.word_wrap_enabled = enabled
        mode = QPlainTextEdit.LineWrapMode.WidgetWidth if enabled else QPlainTextEdit.LineWrapMode.NoWrap
        for i in range(self.tabs.count()):
            self.tabs.widget(i).setLineWrapMode(mode)

    def _create_status_bar(self):
        self.status_mode = QLabel()
        self.status_position = QLabel()
        self.status_counts = QLabel()
        self.status_encoding = QLabel("UTF-8")
        bar = self.statusBar()
        bar.addPermanentWidget(self.status_mode)
        bar.addPermanentWidget(self.status_position)
        bar.addPermanentWidget(self.status_counts)
        bar.addPermanentWidget(self.status_encoding)
        self._update_status_bar()

    def _update_status_bar(self):
        editor = self.current_editor()
        if editor is None:
            self.status_mode.setText("")
            self.status_position.setText("")
            self.status_counts.setText("")
            self.status_encoding.setText("")
            return
        self.status_mode.setText("-- COMMANDE --" if editor.command_mode else "-- INSERTION --")
        self.status_mode.setStyleSheet("font-weight: bold;" if editor.command_mode else "")
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

    def _open_folder_of(self, file_path):
        """Ouvre le dossier du fichier dans le gestionnaire de fichiers du bureau."""
        folder = os.path.dirname(file_path) if file_path else ""
        if not folder or not os.path.isdir(folder):
            self.statusBar().showMessage("Dossier introuvable : " + (folder or "(aucun fichier)"), 4000)
            return
        if not QDesktopServices.openUrl(QUrl.fromLocalFile(folder)):
            self.statusBar().showMessage("Impossible d'ouvrir le dossier : " + folder, 4000)

    def _refresh_recent_menu(self):
        """Remplit le sous-menu des notes fermées récemment (le plus récent d'abord)."""
        self.recent_menu.clear()
        entries = session.list_recent()
        if not entries:
            self.recent_menu.addAction("Aucune note fermée récemment").setEnabled(False)
        for entry in entries:
            path = entry.get("file_path")
            label = os.path.basename(path) if path else entry.get("default_name") or entry["id"][:8]
            action = self.recent_menu.addAction(label)
            action.setToolTip(path_tooltip(path, label, entry["id"]))
            action.triggered.connect(lambda _checked=False, e=entry: self._reopen_recent(e))
        self.recent_menu.addSeparator()
        if entries:
            self.recent_menu.addAction("Effacer la liste").triggered.connect(session.clear_recent)
        self.recent_menu.addAction("Nombre de notes mémorisées...").triggered.connect(self._ask_recent_limit)

    def _ask_recent_limit(self):
        value, ok = QInputDialog.getInt(
            self,
            "Notes fermées récemment",
            f"Nombre de notes à mémoriser ({session.MIN_RECENT} à {session.MAX_RECENT}) :",
            session.get_recent_limit(),
            session.MIN_RECENT,
            session.MAX_RECENT,
        )
        if ok:
            session.set_recent_limit(value)

    def _reopen_recent(self, entry):
        path = entry.get("file_path")
        if path and session.is_external_file(path):
            if not os.path.isfile(path):
                session.remove_recent(entry["id"])
                self.statusBar().showMessage("Fichier introuvable : " + path, 4000)
                return
            self._open_path(path)  # relit le fichier sur le disque
            return
        draft = next((d for d in session.list_drafts() if d["id"] == entry["id"]), None)
        if draft is None:
            session.remove_recent(entry["id"])
            return
        self._open_draft(draft)

    def _create_menu(self):
        menu = self.menuBar()

        file_menu = menu.addMenu("&Fichier")
        file_menu.addAction(self.new_action)
        file_menu.addAction(self.open_action)
        self.recent_menu = file_menu.addMenu("Notes fermées récemment")
        self.recent_menu.setToolTipsVisible(True)
        file_menu.aboutToShow.connect(self._refresh_recent_menu)
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
        editor.pinned = session.is_pinned(editor.session_id)
        session.remove_recent(editor.session_id)
        editor.document().modificationChanged.connect(lambda _: self.update_title())
        editor.autosave_requested.connect(lambda: self._autosave_tab(editor))
        editor.cursorPositionChanged.connect(self._update_status_bar)
        editor.mode_changed.connect(self._update_status_bar)
        editor.textChanged.connect(self._update_status_bar)
        editor.textChanged.connect(self._schedule_preview)
        editor.setLineWrapMode(
            QPlainTextEdit.LineWrapMode.WidgetWidth if self.word_wrap_enabled else QPlainTextEdit.LineWrapMode.NoWrap
        )

        label = os.path.basename(file_path) if file_path else editor.default_name
        index = self.tabs.addTab(editor, label)
        self._refresh_tab_button(editor)
        self.tabs.setCurrentIndex(index)
        if editor.pinned:
            self._normalize_tab_order()
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

    def eventFilter(self, obj, event):
        if obj is self.tabs.tabBar() and event.type() == QEvent.Type.MouseButtonRelease:
            QTimer.singleShot(0, self._normalize_tab_order)
        return super().eventFilter(obj, event)

    def _normalize_tab_order(self):
        """Regroupe les onglets épinglés à gauche, sans changer l'ordre relatif des
        épinglés entre eux ni des autres (partition stable)."""
        if self._normalizing_tabs:
            return
        self._normalizing_tabs = True
        try:
            bar = self.tabs.tabBar()
            target = 0
            moved = False
            for i in range(self.tabs.count()):
                if self.tabs.widget(i).pinned:
                    if i != target:
                        bar.moveTab(i, target)
                        moved = True
                    target += 1
            if moved:
                self._reposition_new_tab_button()
        finally:
            self._normalizing_tabs = False

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
        if entry.get("file_path"):
            # le même fichier est déjà ouvert dans un autre onglet : on y bascule
            for i in range(self.tabs.count()):
                if self.tabs.widget(i).file_path == entry["file_path"]:
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
        if session.is_pinned(entry["id"]):
            self.statusBar().showMessage("Note épinglée : détachez-la pour la mettre à la corbeille.", 3000)
            return
        label = os.path.basename(entry["file_path"]) if entry.get("file_path") else entry.get("default_name") or entry["id"][:8]
        result = QMessageBox.question(
            self,
            "Mettre à la corbeille",
            f"Mettre « {label} » à la corbeille ?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if result == QMessageBox.StandardButton.Yes:
            self._trash_now(entry)
            self._after_trash()

    def _trash_now(self, entry):
        index = self._tab_index_for_id(entry["id"])
        if index is not None:
            # La corbeille doit recevoir le texte à jour (l'autosave est
            # différé), et l'onglet doit disparaître sans passer par
            # close_tab(), qui réarchiverait le brouillon qu'on met à la corbeille.
            self._autosave_tab(self.tabs.widget(index))
            self.tabs.removeTab(index)
        session.trash_draft(entry["id"])

    def _after_trash(self):
        self._refresh_drafts_browser()
        self._reposition_new_tab_button()
        self.update_title()

    def _trash_entries(self, entries):
        """Mise à la corbeille de plusieurs notes : une seule confirmation, les
        notes épinglées sont ignorées."""
        targets = [e for e in entries if not session.is_pinned(e["id"])]
        skipped = len(entries) - len(targets)
        if not targets:
            self.statusBar().showMessage("Notes épinglées : détachez-les pour les mettre à la corbeille.", 3000)
            return
        if len(targets) == 1 and skipped == 0:
            self._trash_draft(targets[0])
            return
        text = f"Mettre {len(targets)} note{'s' if len(targets) > 1 else ''} à la corbeille ?"
        if skipped:
            s = "s" if skipped > 1 else ""
            text += f"\n({skipped} note{s} épinglée{s} ignorée{s}.)"
        result = QMessageBox.question(
            self,
            "Mettre à la corbeille",
            text,
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if result == QMessageBox.StandardButton.Yes:
            for entry in targets:
                self._trash_now(entry)
            self._after_trash()

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
                if self.close_tab(i) is False:
                    break

    def _close_all_tabs(self):
        for i in reversed(range(self.tabs.count())):
            if self.close_tab(i) is False:
                break

    def _close_tabs_to_the_right(self, index):
        for i in reversed(range(index + 1, self.tabs.count())):
            if self.close_tab(i) is False:
                break

    def _close_entries(self, entries):
        """Ferme les onglets des notes sélectionnées dans le panneau Brouillons."""
        for entry in entries:
            index = self._tab_index_for_id(entry["id"])
            if index is not None and self.close_tab(index) is False:
                break

    def _show_version_history(self, editor):
        dialog = VersionHistoryDialog(editor.session_id, self)
        dialog.restore_requested.connect(lambda content: self._apply_restored_version(editor, content))
        dialog.exec()

    def _apply_restored_version(self, editor, content):
        editor.setPlainText(content)
        editor.document().setModified(True)
        self.update_title()

    def _tab_index_for_id(self, draft_id):
        for i in range(self.tabs.count()):
            if self.tabs.widget(i).session_id == draft_id:
                return i
        return None

    def _duplicate_draft_entry(self, entry):
        index = self._tab_index_for_id(entry.get("id"))
        if index is not None:
            self._duplicate_tab(index)
            return
        self.new_tab(content=session.read_draft(entry["id"]))

    def _show_version_history_for_entry(self, entry):
        self._open_draft(entry)
        index = self._tab_index_for_id(entry.get("id"))
        if index is not None:
            self._show_version_history(self.tabs.widget(index))

    def _handle_drafts_context_action(self, action, entry):
        index = self._tab_index_for_id(entry.get("id"))
        if action == "close":
            if index is not None:
                self.close_tab(index)
        elif action == "close_others":
            if index is not None:
                self._close_other_tabs(index)
        elif action == "close_right":
            if index is not None:
                self._close_tabs_to_the_right(index)
        elif action == "close_all":
            self._close_all_tabs()
        elif action == "duplicate":
            self._duplicate_draft_entry(entry)
        elif action == "history":
            self._show_version_history_for_entry(entry)
        elif action == "toggle_pin":
            self._set_pinned(entry["id"], not session.is_pinned(entry["id"]))
        elif action == "open_folder":
            self._open_folder_of(entry.get("file_path"))

    def _show_tab_context_menu(self, pos):
        bar = self.tabs.tabBar()
        index = bar.tabAt(pos)
        if index == -1:
            return
        editor = self.tabs.widget(index)

        menu = QMenu(self)
        close_action = menu.addAction("Fermer")
        close_action.setEnabled(not editor.pinned)
        close_others_action = menu.addAction("Fermer les autres")
        close_right_action = menu.addAction("Fermer à droite")
        close_right_action.setEnabled(index < self.tabs.count() - 1)
        close_all_action = menu.addAction("Fermer tout")
        menu.addSeparator()
        pin_action = menu.addAction("Détacher" if editor.pinned else "Épingler")
        remember_action = menu.addAction("Mémoriser à la fermeture")
        remember_action.setCheckable(True)
        remember_action.setChecked(session.is_remembered(editor.session_id))
        remember_action.setToolTip(REMEMBER_TOOLTIP)
        menu.setToolTipsVisible(True)
        duplicate_action = menu.addAction("Dupliquer")
        rename_action = menu.addAction("Renommer...") if editor.file_path is None else None
        history_action = menu.addAction("Historique des versions...") if session.list_versions(editor.session_id) else None
        menu.addSeparator()
        # le nom marche pour toute note (fichier ou nom de la note) ; le chemin
        # n'existe que pour une note liée à un fichier
        copy_name_action = menu.addAction("Copier le nom du fichier")
        copy_path_action = menu.addAction("Copier le chemin complet du fichier")
        copy_path_action.setEnabled(bool(editor.file_path))
        open_folder_action = menu.addAction("Ouvrir le dossier du fichier")
        open_folder_action.setEnabled(bool(editor.file_path))
        menu.addSeparator()
        trash_action = menu.addAction("Mettre à la corbeille")
        trash_action.setEnabled(not editor.pinned)

        chosen = menu.exec(bar.mapToGlobal(pos))
        if chosen == close_action:
            self.close_tab(index)
        elif chosen == close_others_action:
            self._close_other_tabs(index)
        elif chosen == close_right_action:
            self._close_tabs_to_the_right(index)
        elif chosen == close_all_action:
            self._close_all_tabs()
        elif chosen == pin_action:
            self._set_pinned(editor.session_id, not editor.pinned)
        elif chosen == remember_action:
            session.set_remembered(editor.session_id, not session.is_remembered(editor.session_id))
        elif chosen == duplicate_action:
            self._duplicate_tab(index)
        elif rename_action is not None and chosen == rename_action:
            self._rename_tab(index)
        elif history_action is not None and chosen == history_action:
            self._show_version_history(editor)
        elif chosen == copy_name_action:
            name = os.path.basename(editor.file_path) if editor.file_path else editor.default_name
            QApplication.clipboard().setText(name)
        elif chosen == copy_path_action:
            QApplication.clipboard().setText(editor.file_path)
        elif chosen == open_folder_action:
            self._open_folder_of(editor.file_path)
        elif chosen == trash_action:
            self._trash_draft(
                {
                    "id": editor.session_id,
                    "file_path": editor.file_path,
                    "default_name": editor.default_name,
                    "modified": editor.document().isModified(),
                }
            )

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

    def _refresh_tab_button(self, editor):
        """Décor de l'onglet : la croix de fermeture à droite du nom ou, quand la
        note est épinglée (donc non fermable), une punaise à gauche du nom (icône
        d'onglet native) et plus de croix."""
        index = self.tabs.indexOf(editor)
        if index == -1:
            return
        bar = self.tabs.tabBar()
        if editor.pinned:
            bar.setTabButton(index, QTabBar.ButtonPosition.RightSide, None)
            self.tabs.setTabIcon(index, QIcon(pin_pixmap(14)))
        else:
            self.tabs.setTabIcon(index, QIcon())
            bar.setTabButton(index, QTabBar.ButtonPosition.RightSide, self._make_close_button())
        self._refresh_tab_tooltips()

    def _tab_tooltip(self, editor):
        """Même infobulle que dans le panneau Brouillons : le chemin du fichier."""
        name = os.path.basename(editor.file_path) if editor.file_path else editor.default_name
        tip = path_tooltip(editor.file_path, name, editor.session_id)
        if editor.pinned:
            tip += "\nNote épinglée"
        return tip

    def _refresh_tab_tooltips(self):
        for i in range(self.tabs.count()):
            self.tabs.setTabToolTip(i, self._tab_tooltip(self.tabs.widget(i)))

    def _apply_pinned(self, draft_id, pinned):
        session.set_pinned(draft_id, pinned)
        index = self._tab_index_for_id(draft_id)
        if index is not None:
            editor = self.tabs.widget(index)
            editor.pinned = pinned
            self._refresh_tab_button(editor)

    def _finish_pin_change(self):
        self._normalize_tab_order()
        self._reposition_new_tab_button()
        self._refresh_drafts_browser()
        self._update_pin_action()

    def _set_pinned(self, draft_id, pinned):
        self._apply_pinned(draft_id, pinned)
        self._finish_pin_change()

    def _handle_bulk_action(self, action, entries):
        """Action du menu de la sélection multiple du panneau Brouillons."""
        ids = [entry["id"] for entry in entries]
        if action == "close":
            self._close_entries(entries)
        elif action in ("pin", "unpin"):
            pinned = action == "pin"
            for draft_id in ids:
                if session.is_pinned(draft_id) != pinned:
                    self._apply_pinned(draft_id, pinned)
            self._finish_pin_change()
            self.drafts_browser.select_ids(ids)  # la sélection survit à l'action
        elif action == "trash":
            self._trash_entries(entries)

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
        for entry in entries:
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
            history = session.load_history(entry["id"]) if entry.get("id") else None
            if history:
                editor.restore_history(history)
            if "cursor" in entry:
                editor.restore_view(entry["cursor"], entry.get("scroll", 0), entry.get("anchor"))
        self._normalize_tab_order()
        active = self._tab_index_for_id(active_id) if active_id else None
        self.tabs.setCurrentIndex(active if active is not None else 0)

    def tab_label(self, editor):
        name = os.path.basename(editor.file_path) if editor.file_path else editor.default_name
        return "*" + name if editor.document().isModified() else name

    def _update_pin_action(self):
        editor = self.current_editor()
        pinned = editor is not None and editor.pinned
        self.pin_action.setEnabled(editor is not None and not pinned)
        self.unpin_action.setEnabled(pinned)

    def _set_current_pinned(self, pinned):
        editor = self.current_editor()
        if editor is not None:
            self._set_pinned(editor.session_id, pinned)

    def update_title(self):
        self._refresh_tab_tooltips()
        self._update_pin_action()
        self._update_preview_state()
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
        path, _ = QFileDialog.getOpenFileName(self, "Ouvrir un fichier", "", FILE_FILTERS)
        if path:
            self._open_path(path)

    def _open_path(self, path):
        for i in range(self.tabs.count()):
            w = self.tabs.widget(i)
            if w.file_path == path:
                self.tabs.setCurrentIndex(i)
                return
        existing = session.find_draft_for_path(path)
        if existing is not None and existing["modified"]:
            # une session précédente de ce fichier a laissé des modifications non
            # enregistrées : on les retrouve plutôt que de les écraser
            self._open_draft(existing)
            return
        try:
            with open(path, "r", encoding="utf-8") as f:
                content = f.read()
        except OSError as e:
            QMessageBox.critical(self, "Erreur", f"Impossible d'ouvrir le fichier :\n{e}")
            return
        # même identifiant que le brouillon existant du fichier : une seule entrée
        # par fichier dans le panneau, et l'historique des versions suit
        editor = self.new_tab(file_path=path, content=content, session_id=existing["id"] if existing else None)
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
            path = os.path.join(session.DOCS_DIR, f"{editor.default_name}.md")
            return self._write_file(editor, path)
        return self._write_file(editor, editor.file_path)

    def save_file_as(self):
        editor = self.current_editor()
        if editor is None:
            return False
        start = editor.file_path or os.path.join(session.DOCS_DIR, f"{editor.default_name}.md")
        path, _ = QFileDialog.getSaveFileName(self, "Enregistrer sous", start, FILE_FILTERS, _save_filter_for(start))
        if not path:
            return False
        return self._write_file(editor, path)

    def _adopt_existing_draft(self, editor, path):
        """Enregistrer (sous) vers un fichier qui a déjà une note fermée : on reprend
        son identifiant au lieu de créer une seconde entrée pour le même fichier
        dans le panneau Brouillons (son historique des versions et son état épinglé
        sont conservés). Si cette note est ouverte dans un autre onglet, on laisse
        les choses telles quelles."""
        if path == editor.file_path:
            return
        existing = session.find_draft_for_path(path)
        if existing is None or existing["id"] == editor.session_id:
            return
        if self._tab_index_for_id(existing["id"]) is not None:
            return
        old_id, new_id = editor.session_id, existing["id"]
        pinned = editor.pinned or existing["pinned"]
        session.merge_versions(old_id, new_id)
        session.delete_draft(old_id)
        editor.session_id = new_id
        if pinned != existing["pinned"]:
            session.set_pinned(new_id, pinned)
        if pinned != editor.pinned:
            editor.pinned = pinned
            self._refresh_tab_button(editor)

    def _write_file(self, editor, path):
        self._adopt_existing_draft(editor, path)
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

    def _ask_save_before_close(self, editor):
        """Alerte avant de fermer un fichier extérieur modifié : renvoie "save",
        "discard" ou "cancel"."""
        box = QMessageBox(self)
        box.setIcon(QMessageBox.Icon.Warning)
        box.setWindowTitle("Modifications non enregistrées")
        box.setText(f"« {os.path.basename(editor.file_path)} » a été modifié et n'est pas enregistré.")
        box.setInformativeText(
            "Enregistrez-le pour ne pas perdre vos modifications : une fois fermé, il ne sera plus "
            "listé dans les brouillons."
        )
        save = box.addButton("Enregistrer", QMessageBox.ButtonRole.AcceptRole)
        discard = box.addButton("Ne pas enregistrer", QMessageBox.ButtonRole.DestructiveRole)
        box.addButton("Annuler", QMessageBox.ButtonRole.RejectRole)
        box.setDefaultButton(save)
        box.exec()
        if box.clickedButton() is save:
            return "save"
        if box.clickedButton() is discard:
            return "discard"
        return "cancel"

    def _archive_unmodified(self, editor):
        """Après « Ne pas enregistrer » : le brouillon (masqué) ne doit pas garder
        les modifications abandonnées, sinon elles ressusciteraient à la réouverture."""
        try:
            with open(editor.file_path, "r", encoding="utf-8") as f:
                content = f.read()
        except OSError:
            content = editor.toPlainText()
        session.save_draft(
            {
                "id": editor.session_id,
                "file_path": editor.file_path,
                "default_name": editor.default_name,
                "modified": False,
                "content": content,
            }
        )

    def close_tab(self, index):
        """Ferme l'onglet. Renvoie False seulement si l'utilisateur a annulé (les
        fermetures en lot s'arrêtent alors) ; True sinon, y compris pour une note
        épinglée simplement ignorée."""
        editor = self.tabs.widget(index)
        if editor is None:
            return True
        if editor.pinned:
            # Seul point de passage de toutes les fermetures (croix, Ctrl+W,
            # menus, « fermer les autres/à droite/tout ») : les notes épinglées
            # sont simplement ignorées par les fermetures en lot.
            self.statusBar().showMessage("Note épinglée : détachez-la pour la fermer.", 3000)
            return True
        if session.is_external_file(editor.file_path) and editor.document().isModified():
            choice = self._ask_save_before_close(editor)
            if choice == "cancel":
                return False
            if choice == "save":
                if not self._write_file(editor, editor.file_path):
                    return False
            else:
                self._archive_unmodified(editor)
        if editor.document().isModified() and not session.is_external_file(editor.file_path):
            session.save_draft(
                {
                    "id": editor.session_id,
                    "file_path": editor.file_path,
                    "default_name": editor.default_name,
                    "modified": True,
                    "content": editor.toPlainText(),
                }
            )

        # une note vide sans fichier n'a rien à rouvrir, sauf si elle a demandé à être mémorisée
        if editor.file_path or editor.toPlainText().strip() or session.is_remembered(editor.session_id):
            session.add_recent(
                {"id": editor.session_id, "file_path": editor.file_path, "default_name": editor.default_name}
            )
        self.tabs.removeTab(index)
        self._refresh_drafts_browser()
        self._reposition_new_tab_button()
        self.update_title()
        return True

    def _save_session(self):
        tabs_info = []
        for i in range(self.tabs.count()):
            editor = self.tabs.widget(i)
            cursor, scroll, anchor = editor.view_state()
            tabs_info.append({
                "id": editor.session_id,
                "file_path": editor.file_path,
                "default_name": editor.default_name,
                "modified": editor.document().isModified(),
                "content": editor.toPlainText(),
                "cursor": cursor,
                "scroll": scroll,
                "anchor": anchor,
                "history": editor.history_state(),
            })
        active_editor = self.current_editor()
        active_id = active_editor.session_id if active_editor is not None else None
        session.save_session(tabs_info, active_id)

    @staticmethod
    def _is_visible_on_a_screen(x, y, width, height):
        # Ignore a saved position that no longer lands on any screen (e.g. an
        # external monitor that has since been unplugged).
        rect = QRect(x, y, width, height)
        return any(screen.availableGeometry().intersects(rect) for screen in QApplication.screens())

    def closeEvent(self, event):
        self._save_session()
        session.save_window_state(
            self.width(),
            self.height(),
            self._splitter_sizes_to_save(),
            self.x(),
            self.y(),
            sidebar_visible=self.drafts_panel_action.isChecked(),
            word_wrap=self.word_wrap_action.isChecked(),
            markdown_preview=self.preview_action.isChecked(),
        )
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
