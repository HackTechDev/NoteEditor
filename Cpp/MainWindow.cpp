#include "MainWindow.h"
#include "DraftsBrowser.h"
#include "Editor.h"
#include "FindReplaceDialog.h"
#include "TrashDialog.h"
#include "VersionHistoryDialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPolygon>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStringConverter>
#include <QTabBar>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace {

const char *const kFileFilters =
    "Fichiers texte et Markdown (*.txt *.md);;Fichiers texte (*.txt);;Fichiers Markdown (*.md);;Tous les fichiers (*)";

// Filtre initial de « Enregistrer sous » : celui qui montre le fichier courant.
QString saveFilterFor(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "md")
        return "Fichiers Markdown (*.md)";
    if (ext.isEmpty() || ext == "txt")
        return "Fichiers texte (*.txt)";
    return "Tous les fichiers (*)";
}

void styleNewTabButton(QToolButton *button)
{
    button->setText("+");
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip("Nouvel onglet");
    button->setStyleSheet(R"(
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
    )");
}

// Dessine une icône « lignes qui reviennent à la ligne » : pas de fichier
// externe à embarquer, cohérent avec les autres icônes de l'appli (+, ✕),
// de simples glyphes dessinés plutôt que des assets.
QIcon wordWrapIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(3, 5, 19, 5);
    painter.drawLine(3, 11, 19, 11);
    painter.drawLine(3, 17, 13, 17);
    // petite flèche de retour à la ligne, à la fin de la 2e ligne
    painter.drawLine(19, 11, 19, 16);
    painter.drawLine(19, 16, 15, 16);
    painter.drawLine(15, 16, 17, 14);
    painter.drawLine(15, 16, 17, 18);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « page + » (nouvelle note), dans le même esprit que
// wordWrapIcon() / saveIcon() : glyphes dessinés, pas de fichier externe.
QIcon newNoteIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    // feuille de papier, coin supérieur droit corné
    const QPolygon page({
        QPoint(5, 2),
        QPoint(14, 2),
        QPoint(18, 6),
        QPoint(18, 20),
        QPoint(5, 20),
    });
    painter.drawPolygon(page);
    painter.drawLine(14, 2, 14, 6);
    painter.drawLine(14, 6, 18, 6);
    // « + » au centre bas de la feuille
    painter.drawLine(11, 11, 11, 17);
    painter.drawLine(8, 14, 14, 14);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « dossier » classique, dans le même esprit que
// newNoteIcon() / saveIcon() : glyphes dessinés, pas de fichier externe.
QIcon openIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    const QPolygon folder({
        QPoint(2, 6),
        QPoint(9, 6),
        QPoint(11, 8),
        QPoint(20, 8),
        QPoint(20, 18),
        QPoint(2, 18),
    });
    painter.drawPolygon(folder);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « disquette » classique, dans le même esprit que
// wordWrapIcon() : glyphes dessinés, pas de fichier externe.
QIcon saveIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    // corps de la disquette, coin supérieur droit coupé
    const QPolygon body({
        QPoint(3, 3),
        QPoint(16, 3),
        QPoint(19, 6),
        QPoint(19, 19),
        QPoint(3, 19),
    });
    painter.drawPolygon(body);
    painter.drawRect(7, 3, 6, 6);   // volet métallique en haut
    painter.drawRect(6, 12, 10, 6); // étiquette en bas
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « disquette + flèche » (Enregistrer sous), dans le même
// esprit que saveIcon() : une disquette réduite, avec une petite flèche vers
// un autre emplacement pour la distinguer d'Enregistrer.
QIcon saveAsIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    // disquette réduite, en haut à gauche
    const QPolygon body({
        QPoint(2, 3),
        QPoint(12, 3),
        QPoint(15, 6),
        QPoint(15, 15),
        QPoint(2, 15),
    });
    painter.drawPolygon(body);
    painter.drawRect(6, 3, 5, 4);  // volet métallique en haut
    painter.drawRect(5, 10, 7, 4); // étiquette en bas
    // petite flèche vers un autre emplacement, en bas à droite
    painter.drawLine(14, 14, 20, 20);
    painter.drawLine(20, 20, 20, 15);
    painter.drawLine(20, 20, 15, 20);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « loupe » classique, dans le même esprit que
// saveIcon() : glyphes dessinés, pas de fichier externe.
QIcon findIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawEllipse(3, 3, 11, 11);
    painter.drawLine(13, 13, 19, 19);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « loupe + flèche » (Rechercher / Remplacer), dans le même
// esprit que findIcon() : une loupe réduite, avec une petite flèche vers la
// droite (« remplacer par ») pour la distinguer de Rechercher.
QIcon replaceIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawEllipse(2, 2, 9, 9);
    painter.drawLine(10, 10, 13, 13);
    // petite flèche vers la droite, en bas
    painter.drawLine(11, 17, 20, 17);
    painter.drawLine(17, 14, 20, 17);
    painter.drawLine(17, 20, 20, 17);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « onglet + croix » (Fermer l'onglet), dans le même esprit
// que saveIcon() : un onglet aux coins supérieurs coupés, avec une croix à
// l'intérieur, pour la distinguer des autres icônes de page.
QIcon closeTabIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    const QPolygon tab({
        QPoint(3, 19),
        QPoint(3, 6),
        QPoint(6, 3),
        QPoint(16, 3),
        QPoint(19, 6),
        QPoint(19, 19),
    });
    painter.drawPolygon(tab);
    painter.drawLine(8, 9, 14, 15);
    painter.drawLine(14, 9, 8, 15);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « fenêtre coupée en deux » (aperçu Markdown) : du texte à
// gauche, son rendu (un titre et des lignes) à droite. Glyphes dessinés, pas
// de fichier externe.
QIcon previewIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawRect(2, 4, 18, 14);
    painter.drawLine(11, 4, 11, 18);
    painter.drawLine(5, 8, 8, 8); // texte brut, à gauche
    painter.drawLine(5, 11, 8, 11);
    painter.drawLine(5, 14, 7, 14);
    painter.drawLine(14, 8, 17, 8); // rendu, à droite
    painter.drawLine(14, 11, 17, 11);
    painter.drawLine(14, 14, 16, 14);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « fenêtre avec panneau latéral » (panneau Brouillons) : la colonne
// de gauche est pleine, l'éditeur à droite. Glyphes dessinés, pas de fichier externe.
QIcon draftsPanelIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawRect(2, 4, 18, 14);
    painter.fillRect(3, 5, 5, 12, Qt::darkGray); // le panneau
    painter.drawLine(8, 4, 8, 18);
    painter.drawLine(11, 8, 17, 8); // le texte de l'éditeur
    painter.drawLine(11, 11, 17, 11);
    painter.drawLine(11, 14, 15, 14);
    painter.end();
    return QIcon(pixmap);
}

// Dessine une icône « poubelle » classique, dans le même esprit que
// saveIcon() : glyphes dessinés, pas de fichier externe.
QIcon trashIcon()
{
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::darkGray);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(3, 6, 19, 6); // couvercle
    painter.drawPolyline(QPolygon({QPoint(8, 6), QPoint(8, 3), QPoint(14, 3), QPoint(14, 6)})); // poignée
    painter.drawPolygon(QPolygon({QPoint(5, 6), QPoint(6, 19), QPoint(16, 19), QPoint(17, 6)})); // cuve
    painter.drawLine(9, 10, 9, 15);
    painter.drawLine(13, 10, 13, 15);
    painter.end();
    return QIcon(pixmap);
}

} // namespace

CornerToolButton::CornerToolButton(int height, QWidget *parent)
    : QToolButton(parent)
    , m_forcedHeight(height)
{
}

void CornerToolButton::setForcedHeight(int height)
{
    m_forcedHeight = height;
}

QSize CornerToolButton::sizeHint() const
{
    return QSize(QToolButton::sizeHint().width(), m_forcedHeight);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Éditeur de note");
    resize(900, 650);
    // Qt n'affiche pas les infobulles d'une fenêtre inactive (focus ailleurs,
    // lancement automatique au démarrage...) : on les veut toujours.
    setAttribute(Qt::WA_AlwaysShowToolTips);

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(false);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->tabBar()->setIconSize(QSize(14, 14));
    m_tabs->setStyleSheet(R"(
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
    )");
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::updateTitle);
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::checkCurrentExternalChange);
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::updateStatusBar);
    // les onglets épinglés restent groupés à gauche : après un glisser-déposer on
    // remet l'ordre en place au relâchement de la souris (pas pendant le geste)
    m_tabs->tabBar()->installEventFilter(this);
    m_tabs->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabs->tabBar(), &QTabBar::customContextMenuRequested, this, &MainWindow::showTabContextMenu);

    setAcceptDrops(true);
    connect(qApp, &QApplication::applicationStateChanged, this, &MainWindow::onAppStateChanged);

    // Deux boutons "+" pour le même bouton logique : voir Cpp/CLAUDE.md ou
    // Python/main.py pour le détail du pourquoi (débordement des onglets /
    // flèches de défilement natives).
    m_newTabButton = buildNewTabButton(m_tabs->tabBar());
    const int barHeight = m_tabs->tabBar()->sizeHint().height();
    m_newTabCornerButton = new CornerToolButton(barHeight - 1, nullptr);
    styleNewTabButton(m_newTabCornerButton);
    connect(m_newTabCornerButton, &QToolButton::clicked, this, [this] { newTab(); });
    m_newTabCornerButton->hide();
    m_tabs->setCornerWidget(m_newTabCornerButton, Qt::TopRightCorner);
    repositionNewTabButton();

    m_draftsBrowser = new DraftsBrowser(this);
    connect(m_draftsBrowser, &DraftsBrowser::openRequested, this, &MainWindow::openDraft);
    connect(m_draftsBrowser, &DraftsBrowser::deleteRequested, this, &MainWindow::trashDraftEntry);
    connect(m_draftsBrowser, &DraftsBrowser::renameRequested, this, &MainWindow::renameDraftEntry);
    connect(m_draftsBrowser, &DraftsBrowser::actionRequested, this, &MainWindow::handleDraftsContextAction);
    connect(m_draftsBrowser, &DraftsBrowser::bulkActionRequested, this, &MainWindow::handleBulkAction);

    m_draftsSearch = new QLineEdit(this);
    m_draftsSearch->setPlaceholderText("Rechercher...");
    m_draftsSearch->setToolTip("Filtre les notes par nom ou par contenu");
    connect(m_draftsSearch, &QLineEdit::textChanged, m_draftsBrowser, &DraftsBrowser::setFilterText);

    m_draftsSort = new QComboBox(this);
    m_draftsSort->addItem("Date", "date");
    m_draftsSort->addItem("Nom", "name");
    connect(m_draftsSort, &QComboBox::currentIndexChanged, this, [this](int i) {
        m_draftsBrowser->setSortMode(m_draftsSort->itemData(i).toString());
    });

    auto *draftsToolbar = new QHBoxLayout();
    draftsToolbar->addWidget(m_draftsSearch);
    draftsToolbar->addWidget(m_draftsSort);

    m_trashButton = new QToolButton(this);
    m_trashButton->setText("Corbeille...");
    m_trashButton->setIcon(trashIcon());
    m_trashButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_trashButton, &QToolButton::clicked, this, &MainWindow::showTrash);

    auto *sidebar = new QWidget(this);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(4);
    sidebarLayout->addWidget(new QLabel("Brouillons", sidebar));
    sidebarLayout->addLayout(draftsToolbar);
    sidebarLayout->addWidget(m_draftsBrowser);
    sidebarLayout->addWidget(m_trashButton);

    // Le volet d'aperçu est unique et partagé (il suit l'onglet actif), dans un
    // splitter interne : le splitter principal garde deux entrées, ce qui
    // préserve le format de window.json.
    m_preview = new QTextBrowser(this);
    m_preview->setOpenExternalLinks(true);
    m_preview->hide();
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(250);
    connect(m_previewTimer, &QTimer::timeout, this, &MainWindow::renderPreview);
    // la plage de défilement de l'aperçu se précise après la mise en page : on recale
    connect(m_preview->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int) { syncPreviewScroll(); });
    m_editorSplitter = new QSplitter(this);
    m_editorSplitter->addWidget(m_tabs);
    m_editorSplitter->addWidget(m_preview);
    m_editorSplitter->setStretchFactor(0, 1);
    m_editorSplitter->setStretchFactor(1, 1);
    m_editorSplitter->setSizes({1, 1});

    m_splitter = new QSplitter(this);
    m_splitter->addWidget(sidebar);
    m_splitter->addWidget(m_editorSplitter);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({180, 720});
    setCentralWidget(m_splitter);

    const Session::WindowState windowState = Session::loadWindowState();
    if (windowState.valid) {
        resize(windowState.width, windowState.height);
        if (windowState.hasPosition) {
            // Ignore a saved position that no longer lands on any screen (e.g.
            // an external monitor that has since been unplugged).
            const QRect saved(windowState.x, windowState.y, windowState.width, windowState.height);
            const QList<QScreen *> screens = QGuiApplication::screens();
            const bool visible = std::any_of(screens.begin(), screens.end(), [&](QScreen *screen) {
                return screen->availableGeometry().intersects(saved);
            });
            if (visible)
                move(windowState.x, windowState.y);
        }
        if (!windowState.splitterSizes.isEmpty()) {
            m_splitter->setSizes(windowState.splitterSizes);
            if (windowState.splitterSizes[0] > 0)
                m_draftsPanelWidth = windowState.splitterSizes[0];
        }
    }

    m_findDialog = new FindReplaceDialog(this);

    createActions();
    createMenu();
    createToolBar();
    createStatusBar();

    // réglages d'affichage mémorisés (les valeurs par défaut sont : retour à la
    // ligne activé, aperçu Markdown masqué)
    if (windowState.hasSidebar && !windowState.sidebarVisible)
        m_draftsPanelAction->setChecked(false);
    if (windowState.hasWordWrap && !windowState.wordWrap)
        m_wordWrapAction->setChecked(false);
    if (windowState.hasMarkdownPreview && windowState.markdownPreview)
        m_previewAction->setChecked(true);

    restoreSession();
}

QToolButton *MainWindow::buildNewTabButton(QWidget *parent)
{
    auto *button = new QToolButton(parent);
    styleNewTabButton(button);
    button->setFixedSize(24, 24);
    connect(button, &QToolButton::clicked, this, [this] { newTab(); });
    return button;
}

void MainWindow::createActions()
{
    m_newAction = new QAction(newNoteIcon(), "&Nouveau", this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, [this] { newTab(); });

    m_openAction = new QAction(openIcon(), "&Ouvrir...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);

    m_saveAction = new QAction(saveIcon(), "&Enregistrer", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);

    m_saveAsAction = new QAction(saveAsIcon(), "Enregistrer &sous...", this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);

    m_exportHtmlAction = new QAction("&Exporter en HTML...", this);
    m_exportHtmlAction->setEnabled(false);
    connect(m_exportHtmlAction, &QAction::triggered, this, &MainWindow::exportHtml);

    m_closeTabAction = new QAction(closeTabIcon(), "&Fermer l'onglet", this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    connect(m_closeTabAction, &QAction::triggered, this, [this] { closeTab(m_tabs->currentIndex()); });

    m_quitAction = new QAction("&Quitter", this);
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &QWidget::close);

    m_undoAction = new QAction("Annuler", this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->undo(); });

    m_redoAction = new QAction("Rétablir", this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->redo(); });

    m_cutAction = new QAction("Couper", this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    connect(m_cutAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->cut(); });

    m_copyAction = new QAction("Copier", this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->copy(); });

    m_pasteAction = new QAction("Coller", this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->paste(); });

    m_selectAllAction = new QAction("Tout sélectionner", this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(m_selectAllAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->selectAll(); });

    m_findAction = new QAction(findIcon(), "&Rechercher...", this);
    m_findAction->setShortcut(QKeySequence::Find);
    connect(m_findAction, &QAction::triggered, m_findDialog, &FindReplaceDialog::showForFind);

    m_replaceAction = new QAction(replaceIcon(), "Rechercher / &Remplacer...", this);
    m_replaceAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(m_replaceAction, &QAction::triggered, m_findDialog, &FindReplaceDialog::showForReplace);

    m_findNextAction = new QAction("Suivant", this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    connect(m_findNextAction, &QAction::triggered, m_findDialog, &FindReplaceDialog::findNext);

    m_aboutAction = new QAction("À &propos...", this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    m_pinAction = new QAction(QIcon(pinPixmap(22)), "Épingler l'onglet", this);
    connect(m_pinAction, &QAction::triggered, this, [this] { setCurrentPinned(true); });

    m_unpinAction = new QAction(QIcon(unpinPixmap(22)), "Détacher l'onglet", this);
    connect(m_unpinAction, &QAction::triggered, this, [this] { setCurrentPinned(false); });

    m_trashAction = new QAction(trashIcon(), "&Corbeille...", this);
    connect(m_trashAction, &QAction::triggered, this, &MainWindow::showTrash);

    m_draftsPanelAction = new QAction(draftsPanelIcon(), "Afficher / masquer le panneau Brouillons", this);
    m_draftsPanelAction->setCheckable(true);
    m_draftsPanelAction->setChecked(true);
    connect(m_draftsPanelAction, &QAction::toggled, this, &MainWindow::setDraftsPanelVisible);

    m_wordWrapAction = new QAction(wordWrapIcon(), "Retour automatique à la ligne", this);
    m_wordWrapAction->setCheckable(true);
    m_wordWrapAction->setChecked(true);
    connect(m_wordWrapAction, &QAction::toggled, this, &MainWindow::setWordWrap);

    m_previewAction = new QAction(previewIcon(), "Aperçu Markdown", this);
    m_previewAction->setCheckable(true);
    m_previewAction->setEnabled(false);
    connect(m_previewAction, &QAction::toggled, this, [this](bool) { updatePreviewState(); });
}

void MainWindow::createToolBar()
{
    auto *toolbar = new QToolBar("Barre d'outils", this);
    toolbar->setMovable(false);
    toolbar->addAction(m_newAction);
    toolbar->addAction(m_openAction);
    toolbar->addAction(m_saveAction);
    toolbar->addAction(m_saveAsAction);
    toolbar->addAction(m_closeTabAction);
    toolbar->addAction(m_pinAction);
    toolbar->addAction(m_unpinAction);
    toolbar->addAction(m_trashAction);
    toolbar->addSeparator();
    toolbar->addAction(m_findAction);
    toolbar->addAction(m_replaceAction);
    toolbar->addSeparator();
    toolbar->addAction(m_draftsPanelAction);
    toolbar->addAction(m_wordWrapAction);
    toolbar->addAction(m_previewAction);
    addToolBar(toolbar);
}

void MainWindow::setWordWrap(bool enabled)
{
    m_wordWrapEnabled = enabled;
    const auto mode = enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap;
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (auto *editor = qobject_cast<Editor *>(m_tabs->widget(i)))
            editor->setLineWrapMode(mode);
    }
}

// Affiche ou masque le panneau Brouillons ; il retrouve sa largeur en revenant.
void MainWindow::setDraftsPanelVisible(bool visible)
{
    QWidget *panel = m_splitter->widget(0);
    if (!visible) {
        // (fenêtre pas encore affichée, au lancement : les tailles ne sont pas fiables)
        if (isVisible() && !panel->isHidden() && m_splitter->sizes().value(0) > 0)
            m_draftsPanelWidth = m_splitter->sizes().value(0);
        panel->hide();
    } else {
        panel->show();
        const int total = m_splitter->width() - m_splitter->handleWidth();
        m_splitter->setSizes({m_draftsPanelWidth, qMax(1, total - m_draftsPanelWidth)});
    }
    if (Editor *editor = currentEditor())
        editor->setFocus();
}

QList<int> MainWindow::splitterSizesToSave() const
{
    QList<int> sizes = m_splitter->sizes();
    if (m_splitter->widget(0)->isHidden())
        // la largeur d'avant le masquage, prise sur celle de l'éditeur
        return {m_draftsPanelWidth, qMax(1, sizes.value(1) - m_draftsPanelWidth - m_splitter->handleWidth())};
    return sizes;
}

void MainWindow::createStatusBar()
{
    m_statusMode = new QLabel(this);
    m_statusPosition = new QLabel(this);
    m_statusCounts = new QLabel(this);
    m_statusEncoding = new QLabel("UTF-8", this);
    QStatusBar *bar = statusBar();
    bar->addPermanentWidget(m_statusMode);
    bar->addPermanentWidget(m_statusPosition);
    bar->addPermanentWidget(m_statusCounts);
    bar->addPermanentWidget(m_statusEncoding);
    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    Editor *editor = currentEditor();
    if (!editor) {
        m_statusMode->setText(QString());
        m_statusPosition->setText(QString());
        m_statusCounts->setText(QString());
        m_statusEncoding->setText(QString());
        return;
    }

    m_statusMode->setText(editor->commandMode() ? "-- COMMANDE --" : "-- INSERTION --");
    m_statusMode->setStyleSheet(editor->commandMode() ? "font-weight: bold;" : QString());
    const QTextCursor cursor = editor->textCursor();
    const int line = cursor.blockNumber() + 1;
    const int col = cursor.columnNumber() + 1;
    m_statusPosition->setText(QString("Ligne %1, Colonne %2").arg(line).arg(col));

    const QString text = editor->toPlainText();
    const int words = static_cast<int>(text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size());
    const int chars = static_cast<int>(text.size());
    const QString wordLabel = words <= 1 ? "mot" : "mots";
    const QString charLabel = chars <= 1 ? "caractère" : "caractères";
    m_statusCounts->setText(QString("%1 %2, %3 %4").arg(words).arg(wordLabel).arg(chars).arg(charLabel));

    m_statusEncoding->setText("UTF-8");
}

void MainWindow::createMenu()
{
    QMenuBar *menu = menuBar();

    QMenu *fileMenu = menu->addMenu("&Fichier");
    fileMenu->addAction(m_newAction);
    fileMenu->addAction(m_openAction);
    m_recentMenu = fileMenu->addMenu("Notes fermées récemment");
    m_recentMenu->setToolTipsVisible(true);
    connect(fileMenu, &QMenu::aboutToShow, this, &MainWindow::refreshRecentMenu);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addAction(m_exportHtmlAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_trashAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeTabAction);
    fileMenu->addAction(m_quitAction);

    QMenu *editMenu = menu->addMenu("&Édition");
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(m_cutAction);
    editMenu->addAction(m_copyAction);
    editMenu->addAction(m_pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(m_selectAllAction);

    QMenu *searchMenu = menu->addMenu("&Rechercher");
    searchMenu->addAction(m_findAction);
    searchMenu->addAction(m_replaceAction);
    searchMenu->addAction(m_findNextAction);

    QMenu *helpMenu = menu->addMenu("&Aide");
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "À propos",
        "<h3>Éditeur de note</h3>"
        "<p>Éditeur de note à onglets écrit en C++ avec Qt6.</p>"
        "<p>Numéros de ligne, coloration syntaxique, recherche/remplacement "
        "et restauration automatique de session.</p>");
}

Editor *MainWindow::currentEditor() const
{
    return qobject_cast<Editor *>(m_tabs->currentWidget());
}

Editor *MainWindow::newTab(const QString &filePath, const QString &content, const QString &defaultName,
                            const QString &sessionId, bool modified)
{
    auto *editor = new Editor();
    if (!sessionId.isEmpty())
        editor->sessionId = sessionId;
    editor->setPlainText(content);
    editor->setFilePath(filePath);
    editor->defaultName = filePath.isEmpty() ? (!defaultName.isEmpty() ? defaultName : timestampName()) : QString();
    editor->document()->setModified(modified);
    editor->pinned = Session::isPinned(editor->sessionId);
    Session::removeRecent(editor->sessionId); // une note (ré)ouverte n'est plus « fermée »
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this](bool) { updateTitle(); });
    connect(editor, &Editor::autosaveRequested, this, [this, editor] { autosaveTab(editor); });
    editor->setLineWrapMode(m_wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(editor, &Editor::modeChanged, this, &MainWindow::updateStatusBar);
    connect(editor, &QPlainTextEdit::textChanged, this, &MainWindow::updateStatusBar);
    connect(editor, &QPlainTextEdit::textChanged, this, &MainWindow::schedulePreview);
    // (le défilement d'un onglet qui n'est pas l'actif ne compte pas)
    connect(editor->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, editor](int) {
        if (editor == currentEditor())
            syncPreviewScroll();
    });

    const QString label = !filePath.isEmpty() ? QFileInfo(filePath).fileName() : editor->defaultName;
    const int index = m_tabs->addTab(editor, label);
    refreshTabButton(editor);
    m_tabs->setCurrentIndex(index);
    if (editor->pinned)
        normalizeTabOrder(); // note épinglée rouverte : dans le groupe de gauche
    editor->setFocus();

    Session::TabSnapshot snap;
    snap.id = editor->sessionId;
    snap.filePath = editor->filePath;
    snap.defaultName = editor->defaultName;
    snap.modified = editor->document()->isModified();
    snap.content = editor->toPlainText();
    Session::saveDraft(snap);

    refreshDraftsBrowser();
    repositionNewTabButton();
    return editor;
}

// Décor de l'onglet : la croix de fermeture à droite du nom ou, quand la note
// est épinglée (donc non fermable), une punaise à gauche du nom (icône d'onglet
// native) et plus de croix.
void MainWindow::refreshTabButton(Editor *editor)
{
    const int index = m_tabs->indexOf(editor);
    if (index == -1)
        return;
    QTabBar *bar = m_tabs->tabBar();
    if (editor->pinned) {
        bar->setTabButton(index, QTabBar::RightSide, nullptr);
        m_tabs->setTabIcon(index, QIcon(pinPixmap(14)));
    } else {
        m_tabs->setTabIcon(index, QIcon());
        bar->setTabButton(index, QTabBar::RightSide, makeCloseButton());
    }
    refreshTabTooltips();
}

// Même infobulle que dans le panneau Brouillons : le chemin du fichier.
void MainWindow::refreshTabTooltips()
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
        if (!editor)
            continue;
        const QString name = !editor->filePath.isEmpty() ? QFileInfo(editor->filePath).fileName() : editor->defaultName;
        QString tip = pathTooltip(editor->filePath, name, editor->sessionId);
        if (editor->pinned)
            tip += "\nNote épinglée";
        m_tabs->setTabToolTip(i, tip);
    }
}

void MainWindow::applyPinned(const QString &draftId, bool pinned)
{
    Session::setPinned(draftId, pinned);
    const int index = tabIndexForId(draftId);
    if (index != -1) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(index));
        editor->pinned = pinned;
        refreshTabButton(editor);
    }
}

void MainWindow::finishPinChange()
{
    normalizeTabOrder();
    repositionNewTabButton();
    refreshDraftsBrowser();
    updatePinAction();
}

void MainWindow::setPinned(const QString &draftId, bool pinned)
{
    applyPinned(draftId, pinned);
    finishPinChange();
}

// Action du menu de la sélection multiple du panneau Brouillons.
void MainWindow::handleBulkAction(const QString &action, const QVector<Session::DraftEntry> &entries)
{
    QStringList ids;
    for (const Session::DraftEntry &entry : entries)
        ids << entry.id;
    if (action == "close") {
        closeEntries(entries);
    } else if (action == "pin" || action == "unpin") {
        const bool pinned = action == "pin";
        for (const QString &id : ids) {
            if (Session::isPinned(id) != pinned)
                applyPinned(id, pinned);
        }
        finishPinChange();
        m_draftsBrowser->selectIds(ids); // la sélection survit à l'action
    } else if (action == "trash") {
        trashEntries(entries);
    }
}

QWidget *MainWindow::makeCloseButton()
{
    auto *button = new QToolButton();
    button->setText(QString(QChar(0x2715))); // "✕"
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(18, 18);
    button->setStyleSheet(R"(
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
    )");
    connect(button, &QToolButton::clicked, this, [this, button] { closeTabByButton(button); });

    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->addWidget(button);
    return wrapper;
}

void MainWindow::closeTabByButton(QToolButton *button)
{
    QTabBar *bar = m_tabs->tabBar();
    for (int i = 0; i < bar->count(); ++i) {
        QWidget *widget = bar->tabButton(i, QTabBar::RightSide);
        if (widget && widget->findChild<QToolButton *>() == button) {
            closeTab(i);
            return;
        }
    }
}

QString MainWindow::timestampName()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QString base = now.toString("yyMMdd_HHmmss");
    const int hundredths = now.time().msec() / 10;
    return base + QString("%1").arg(hundredths, 2, 10, QChar('0'));
}

void MainWindow::repositionNewTabButton()
{
    QTimer::singleShot(0, this, &MainWindow::toggleNewTabButtonMode);
}

QVector<QToolButton *> MainWindow::nativeScrollButtons() const
{
    // bar->children() (non-recursive) holds the native scroll-arrow buttons
    // (only present when tabs overflow) as direct QToolButton children;
    // per-tab close buttons live one level deeper, inside their wrapper.
    QVector<QToolButton *> result;
    for (QObject *c : m_tabs->tabBar()->children()) {
        auto *btn = qobject_cast<QToolButton *>(c);
        if (btn && btn != m_newTabButton && btn->isVisible())
            result.append(btn);
    }
    return result;
}

void MainWindow::toggleNewTabButtonMode()
{
    const bool overflowing = !nativeScrollButtons().isEmpty();
    m_newTabButton->setVisible(!overflowing);
    if (overflowing) {
        // la hauteur de la barre n'est fiable qu'une fois des onglets
        // présents (0 à la construction) : on la recalcule ici et on force
        // Qt à requêter le sizeHint mis à jour avant d'afficher le bouton.
        const int barHeight = m_tabs->tabBar()->height();
        m_newTabCornerButton->setForcedHeight(qMax(1, barHeight - 1));
        m_newTabCornerButton->updateGeometry();
    }
    m_newTabCornerButton->setVisible(overflowing);
    if (overflowing)
        return;
    QTimer::singleShot(0, this, &MainWindow::positionInlineNewTabButton);
}

void MainWindow::positionInlineNewTabButton()
{
    if (!nativeScrollButtons().isEmpty())
        return;
    QTabBar *bar = m_tabs->tabBar();
    const int buttonWidth = m_newTabButton->width();
    const int afterTabsX = bar->count() > 0 ? bar->tabRect(bar->count() - 1).right() + 4 : 4;
    const int maxX = bar->width() - buttonWidth - 4;
    const int x = qMax(4, qMin(afterTabsX, maxX));
    const int y = qMax(0, (bar->height() - m_newTabButton->height()) / 2);
    m_newTabButton->move(x, y);
}

void MainWindow::refreshDraftsBrowser()
{
    QSet<QString> openIds;
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (auto *editor = qobject_cast<Editor *>(m_tabs->widget(i)))
            openIds.insert(editor->sessionId);
    }
    m_draftsBrowser->refresh(openIds);
    highlightActiveDraft();
}

void MainWindow::highlightActiveDraft()
{
    if (Editor *editor = currentEditor())
        m_draftsBrowser->selectDraft(editor->sessionId);
    else
        m_draftsBrowser->setCurrentItem(nullptr);
}

void MainWindow::openDraft(const Session::DraftEntry &entry)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
        if (editor && editor->sessionId == entry.id) {
            m_tabs->setCurrentIndex(i);
            return;
        }
    }
    if (!entry.filePath.isEmpty()) {
        // le même fichier est déjà ouvert dans un autre onglet : on y bascule
        for (int i = 0; i < m_tabs->count(); ++i) {
            auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
            if (editor && editor->filePath == entry.filePath) {
                m_tabs->setCurrentIndex(i);
                return;
            }
        }
    }
    const QString content = Session::readDraft(entry.id);
    newTab(entry.filePath, content, entry.defaultName, entry.id, entry.modified);
}

void MainWindow::trashDraftEntry(const Session::DraftEntry &entry)
{
    if (Session::isPinned(entry.id)) {
        statusBar()->showMessage("Note épinglée : détachez-la pour la mettre à la corbeille.", 3000);
        return;
    }
    if (Session::isExternalFile(entry.filePath)) {
        statusBar()->showMessage("Fichier extérieur : fermez-le plutôt que de le mettre à la corbeille.", 3000);
        return;
    }
    const QString label = !entry.filePath.isEmpty()
        ? QFileInfo(entry.filePath).fileName()
        : (!entry.defaultName.isEmpty() ? entry.defaultName : entry.id.left(8));
    QString text = QString("Mettre « %1 » à la corbeille ?").arg(label);
    if (!entry.filePath.isEmpty()) {
        // ce n'est que le brouillon interne qui part à la corbeille ; le fichier réel
        // (dans ~/.noteeditor/docs/ ou ailleurs) n'est jamais touché par cette action
        text += "\n\nLe fichier n'est pas supprimé du disque : il reste à son emplacement. Seule la note disparaît de l'application.";
    }
    const auto result = QMessageBox::question(this, "Mettre à la corbeille", text, QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) {
        trashNow(entry);
        afterTrash();
    }
}

void MainWindow::trashNow(const Session::DraftEntry &entry)
{
    const int index = tabIndexForId(entry.id);
    if (index != -1) {
        // La corbeille doit recevoir le texte à jour (l'autosave est différé), et
        // l'onglet doit disparaître sans passer par closeTab(), qui réarchiverait
        // le brouillon qu'on met à la corbeille.
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(index));
        autosaveTab(editor);
        m_tabs->removeTab(index);
        editor->deleteLater();
    }
    Session::trashDraft(entry.id);
}

void MainWindow::afterTrash()
{
    refreshDraftsBrowser();
    repositionNewTabButton();
    updateTitle();
}

// Mise à la corbeille de plusieurs notes : une seule confirmation, les notes
// épinglées sont ignorées.
// Mise à la corbeille de plusieurs notes : une seule confirmation ; les notes épinglées et
// les fichiers extérieurs à ~/.noteeditor sont ignorés (ces derniers se ferment plutôt
// qu'ils ne se mettent à la corbeille).
void MainWindow::trashEntries(const QVector<Session::DraftEntry> &entries)
{
    QVector<Session::DraftEntry> targets;
    int skippedPinned = 0, skippedExternal = 0;
    for (const Session::DraftEntry &e : entries) {
        if (Session::isPinned(e.id))
            ++skippedPinned;
        else if (Session::isExternalFile(e.filePath))
            ++skippedExternal;
        else
            targets.append(e);
    }
    if (targets.isEmpty()) {
        QString msg;
        if (skippedPinned && skippedExternal)
            msg = "Notes épinglées ou fichiers extérieurs : détachez ou fermez-les pour les mettre à la corbeille.";
        else if (skippedPinned)
            msg = "Notes épinglées : détachez-les pour les mettre à la corbeille.";
        else
            msg = "Fichiers extérieurs : fermez-les plutôt que de les mettre à la corbeille.";
        statusBar()->showMessage(msg, 3000);
        return;
    }
    if (targets.size() == 1 && !skippedPinned && !skippedExternal) {
        trashDraftEntry(targets.first());
        return;
    }
    QString text = QString("Mettre %1 note%2 à la corbeille ?").arg(targets.size()).arg(targets.size() > 1 ? "s" : "");
    if (skippedPinned) {
        const QString s = skippedPinned > 1 ? "s" : "";
        text += QString("\n(%1 note%2 épinglée%2 ignorée%2.)").arg(skippedPinned).arg(s);
    }
    if (skippedExternal) {
        const QString s = skippedExternal > 1 ? "s" : "";
        text += QString("\n(%1 fichier%2 extérieur%2 ignoré%2 — fermez-le%2 plutôt.)").arg(skippedExternal).arg(s);
    }
    const bool hasFiles = std::any_of(targets.begin(), targets.end(),
        [](const Session::DraftEntry &e) { return !e.filePath.isEmpty(); });
    if (hasFiles) {
        text += "\n\nLes fichiers associés ne sont pas supprimés du disque : ils restent à leur emplacement. "
                "Seules les notes disparaissent de l'application.";
    }
    const auto result = QMessageBox::question(this, "Mettre à la corbeille", text, QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) {
        for (const Session::DraftEntry &entry : targets)
            trashNow(entry);
        afterTrash();
    }
}

void MainWindow::showTrash()
{
    auto *dialog = new TrashDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &TrashDialog::restored, this, &MainWindow::refreshDraftsBrowser);
    dialog->exec();
}

void MainWindow::autosaveTab(Editor *editor)
{
    if (m_tabs->indexOf(editor) == -1)
        return; // l'onglet a été fermé avant que le délai ne s'écoule

    Session::TabSnapshot snap;
    snap.id = editor->sessionId;
    snap.filePath = editor->filePath;
    snap.defaultName = editor->defaultName;
    snap.modified = editor->document()->isModified();
    snap.content = editor->toPlainText();
    Session::saveDraft(snap);
}

void MainWindow::renameTab(int index)
{
    Editor *editor = qobject_cast<Editor *>(m_tabs->widget(index));
    if (!editor || !editor->filePath.isEmpty())
        return;
    bool ok = false;
    const QString newName = QInputDialog::getText(this, "Renommer", "Nouveau nom :",
                                                    QLineEdit::Normal, editor->defaultName, &ok)
                                 .trimmed();
    if (!ok || newName.isEmpty())
        return;
    editor->defaultName = newName;

    Session::TabSnapshot snap;
    snap.id = editor->sessionId;
    snap.filePath = editor->filePath;
    snap.defaultName = editor->defaultName;
    snap.modified = editor->document()->isModified();
    snap.content = editor->toPlainText();
    Session::saveDraft(snap);

    updateTitle();
    refreshDraftsBrowser();
}

void MainWindow::renameDraftEntry(const Session::DraftEntry &entry)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
        if (editor && editor->sessionId == entry.id) {
            renameTab(i);
            return;
        }
    }
    bool ok = false;
    const QString newName = QInputDialog::getText(this, "Renommer", "Nouveau nom :",
                                                    QLineEdit::Normal, entry.defaultName, &ok)
                                 .trimmed();
    if (!ok || newName.isEmpty())
        return;
    Session::renameDraft(entry.id, newName);
    refreshDraftsBrowser();
}

int MainWindow::tabIndexForId(const QString &draftId) const
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
        if (editor && editor->sessionId == draftId)
            return i;
    }
    return -1;
}

void MainWindow::duplicateDraftEntry(const Session::DraftEntry &entry)
{
    const int index = tabIndexForId(entry.id);
    if (index != -1) {
        duplicateTab(index);
        return;
    }
    newTab(QString(), Session::readDraft(entry.id));
}

void MainWindow::showVersionHistoryForEntry(const Session::DraftEntry &entry)
{
    openDraft(entry);
    const int index = tabIndexForId(entry.id);
    if (index != -1) {
        if (auto *editor = qobject_cast<Editor *>(m_tabs->widget(index)))
            showVersionHistory(editor);
    }
}

void MainWindow::handleDraftsContextAction(const QString &action, const Session::DraftEntry &entry)
{
    const int index = tabIndexForId(entry.id);
    if (action == "close") {
        if (index != -1)
            closeTab(index);
    } else if (action == "close_others") {
        if (index != -1)
            closeOtherTabs(index);
    } else if (action == "close_right") {
        if (index != -1)
            closeTabsToTheRight(index);
    } else if (action == "close_all") {
        closeAllTabs();
    } else if (action == "duplicate") {
        duplicateDraftEntry(entry);
    } else if (action == "history") {
        showVersionHistoryForEntry(entry);
    } else if (action == "toggle_pin") {
        setPinned(entry.id, !Session::isPinned(entry.id));
    } else if (action == "open_folder") {
        openFolderOf(entry.filePath);
    }
}

void MainWindow::duplicateTab(int index)
{
    Editor *editor = qobject_cast<Editor *>(m_tabs->widget(index));
    if (!editor)
        return;
    newTab(QString(), editor->toPlainText());
}

void MainWindow::closeOtherTabs(int index)
{
    QWidget *keep = m_tabs->widget(index);
    for (int i = m_tabs->count() - 1; i >= 0; --i) {
        if (m_tabs->widget(i) != keep && !closeTab(i))
            break;
    }
}

void MainWindow::closeAllTabs()
{
    for (int i = m_tabs->count() - 1; i >= 0; --i) {
        if (!closeTab(i))
            break;
    }
}

void MainWindow::closeTabsToTheRight(int index)
{
    for (int i = m_tabs->count() - 1; i > index; --i) {
        if (!closeTab(i))
            break;
    }
}

// Ferme les onglets des notes sélectionnées dans le panneau Brouillons.
void MainWindow::closeEntries(const QVector<Session::DraftEntry> &entries)
{
    for (const Session::DraftEntry &entry : entries) {
        const int index = tabIndexForId(entry.id);
        if (index != -1 && !closeTab(index))
            break;
    }
}

void MainWindow::showVersionHistory(Editor *editor)
{
    auto *dialog = new VersionHistoryDialog(editor->sessionId, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &VersionHistoryDialog::restoreRequested, this,
            [this, editor](const QString &content) { applyRestoredVersion(editor, content); });
    dialog->exec();
}

void MainWindow::applyRestoredVersion(Editor *editor, const QString &content)
{
    editor->setPlainText(content);
    editor->document()->setModified(true);
    updateTitle();
}

void MainWindow::showTabContextMenu(const QPoint &pos)
{
    QTabBar *bar = m_tabs->tabBar();
    const int index = bar->tabAt(pos);
    if (index == -1)
        return;
    Editor *editor = qobject_cast<Editor *>(m_tabs->widget(index));

    QMenu menu(this);
    QAction *closeAction = menu.addAction("Fermer");
    closeAction->setEnabled(editor && !editor->pinned);
    QAction *closeOthersAction = menu.addAction("Fermer les autres");
    QAction *closeRightAction = menu.addAction("Fermer à droite");
    closeRightAction->setEnabled(index < m_tabs->count() - 1);
    QAction *closeAllAction = menu.addAction("Fermer tout");
    menu.addSeparator();
    QAction *pinAction = editor ? menu.addAction(editor->pinned ? "Détacher" : "Épingler") : nullptr;
    QAction *rememberAction = editor ? menu.addAction("Mémoriser à la fermeture") : nullptr;
    if (rememberAction) {
        rememberAction->setCheckable(true);
        rememberAction->setChecked(Session::isRemembered(editor->sessionId));
        rememberAction->setToolTip(kRememberTooltip);
        menu.setToolTipsVisible(true);
    }
    QAction *duplicateAction = menu.addAction("Dupliquer");
    QAction *renameAction = (editor && editor->filePath.isEmpty()) ? menu.addAction("Renommer...") : nullptr;
    QAction *historyAction = (editor && !Session::listVersions(editor->sessionId).isEmpty())
        ? menu.addAction("Historique des versions...")
        : nullptr;
    menu.addSeparator();
    // le nom marche pour toute note (fichier ou nom de la note) ; le chemin
    // n'existe que pour une note liée à un fichier
    QAction *copyNameAction = menu.addAction("Copier le nom du fichier");
    QAction *copyPathAction = menu.addAction("Copier le chemin complet du fichier");
    copyPathAction->setEnabled(editor && !editor->filePath.isEmpty());
    QAction *openFolderAction = menu.addAction("Ouvrir le dossier du fichier");
    openFolderAction->setEnabled(editor && !editor->filePath.isEmpty());
    menu.addSeparator();
    QAction *trashAction = menu.addAction("Mettre à la corbeille");
    trashAction->setEnabled(editor && !editor->pinned && !Session::isExternalFile(editor->filePath));

    QAction *chosen = menu.exec(bar->mapToGlobal(pos));
    if (chosen == closeAction)
        closeTab(index);
    else if (chosen == closeOthersAction)
        closeOtherTabs(index);
    else if (chosen == closeRightAction)
        closeTabsToTheRight(index);
    else if (chosen == closeAllAction)
        closeAllTabs();
    else if (pinAction && chosen == pinAction)
        setPinned(editor->sessionId, !editor->pinned);
    else if (rememberAction && chosen == rememberAction)
        Session::setRemembered(editor->sessionId, !Session::isRemembered(editor->sessionId));
    else if (chosen == duplicateAction)
        duplicateTab(index);
    else if (renameAction && chosen == renameAction)
        renameTab(index);
    else if (historyAction && chosen == historyAction)
        showVersionHistory(editor);
    else if (editor && chosen == copyNameAction)
        QApplication::clipboard()->setText(!editor->filePath.isEmpty() ? QFileInfo(editor->filePath).fileName()
                                                                       : editor->defaultName);
    else if (editor && chosen == copyPathAction)
        QApplication::clipboard()->setText(editor->filePath);
    else if (editor && chosen == openFolderAction)
        openFolderOf(editor->filePath);
    else if (editor && chosen == trashAction) {
        Session::DraftEntry entry;
        entry.id = editor->sessionId;
        entry.filePath = editor->filePath;
        entry.defaultName = editor->defaultName;
        entry.modified = editor->document()->isModified();
        trashDraftEntry(entry);
    }
}

void MainWindow::checkCurrentExternalChange()
{
    checkExternalChange(currentEditor());
}

void MainWindow::onAppStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive)
        checkCurrentExternalChange();
}

void MainWindow::checkExternalChange(Editor *editor)
{
    if (!editor || editor->filePath.isEmpty())
        return;
    const QFileInfo fi(editor->filePath);
    if (!fi.exists())
        return;
    const qint64 mtime = fi.lastModified().toMSecsSinceEpoch();

    if (editor->diskMTime < 0) {
        editor->diskMTime = mtime;
        return;
    }
    if (mtime <= editor->diskMTime)
        return;
    editor->diskMTime = mtime;

    const auto result = QMessageBox::warning(this, "Fichier modifié en dehors de l'éditeur",
        QString("« %1 » a été modifié par un autre programme.\n"
                "Voulez-vous recharger son contenu depuis le disque ? "
                "Les modifications non enregistrées dans cet onglet seront perdues.")
            .arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::No);
    if (result != QMessageBox::Yes)
        return;

    QFile file(editor->filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", QString("Impossible de recharger le fichier :\n%1").arg(file.errorString()));
        return;
    }
    editor->setPlainText(QString::fromUtf8(file.readAll()));
    editor->document()->setModified(false);
    updateTitle();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        const QString path = url.toLocalFile();
        if (!path.isEmpty())
            openPath(path);
    }
    event->acceptProposedAction();
}

void MainWindow::restoreSession()
{
    QString activeId;
    const QVector<Session::TabSnapshot> entries = Session::loadSession(&activeId);
    if (entries.isEmpty()) {
        newTab();
        return;
    }
    for (int i = 0; i < entries.size(); ++i) {
        const Session::TabSnapshot &entry = entries[i];
        Editor *editor = newTab(entry.filePath, entry.content, entry.defaultName, entry.id, entry.modified);
        if (!entry.filePath.isEmpty() && QFileInfo::exists(entry.filePath))
            editor->diskMTime = QFileInfo(entry.filePath).lastModified().toMSecsSinceEpoch();
        const QJsonObject history = Session::loadHistory(entry.id);
        if (!history.isEmpty())
            editor->restoreHistory(history);
        if (entry.cursor >= 0)
            editor->restoreView(entry.cursor, entry.scroll, entry.anchor);
    }
    normalizeTabOrder();
    const int activeIndex = activeId.isEmpty() ? -1 : tabIndexForId(activeId);
    m_tabs->setCurrentIndex(activeIndex != -1 ? activeIndex : 0);
}

QString MainWindow::tabLabel(Editor *editor) const
{
    const QString name = !editor->filePath.isEmpty() ? QFileInfo(editor->filePath).fileName() : editor->defaultName;
    return editor->document()->isModified() ? "*" + name : name;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_tabs->tabBar() && event->type() == QEvent::MouseButtonRelease)
        QTimer::singleShot(0, this, &MainWindow::normalizeTabOrder);
    return QMainWindow::eventFilter(watched, event);
}

// Regroupe les onglets épinglés à gauche, sans changer l'ordre relatif des
// épinglés entre eux ni des autres (partition stable).
void MainWindow::normalizeTabOrder()
{
    if (m_normalizingTabs)
        return;
    m_normalizingTabs = true;
    QTabBar *bar = m_tabs->tabBar();
    int target = 0;
    bool moved = false;
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
        if (editor && editor->pinned) {
            if (i != target) {
                bar->moveTab(i, target);
                moved = true;
            }
            ++target;
        }
    }
    if (moved)
        repositionNewTabButton();
    m_normalizingTabs = false;
}

// Ouvre le dossier du fichier dans le gestionnaire de fichiers du bureau.
void MainWindow::openFolderOf(const QString &filePath)
{
    const QString folder = filePath.isEmpty() ? QString() : QFileInfo(filePath).absolutePath();
    if (folder.isEmpty() || !QFileInfo(folder).isDir()) {
        statusBar()->showMessage("Dossier introuvable : " + (folder.isEmpty() ? QString("(aucun fichier)") : folder), 4000);
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(folder)))
        statusBar()->showMessage("Impossible d'ouvrir le dossier : " + folder, 4000);
}

// Remplit le sous-menu des notes fermées récemment (la plus récente d'abord).
void MainWindow::refreshRecentMenu()
{
    m_recentMenu->clear();
    const QVector<Session::DraftEntry> entries = Session::listRecent();
    if (entries.isEmpty())
        m_recentMenu->addAction("Aucune note fermée récemment")->setEnabled(false);
    for (const Session::DraftEntry &entry : entries) {
        const QString label = !entry.filePath.isEmpty() ? QFileInfo(entry.filePath).fileName()
                                                        : (!entry.defaultName.isEmpty() ? entry.defaultName : entry.id.left(8));
        QAction *action = m_recentMenu->addAction(label);
        action->setToolTip(pathTooltip(entry.filePath, label, entry.id));
        connect(action, &QAction::triggered, this, [this, entry] { reopenRecent(entry); });
    }
    m_recentMenu->addSeparator();
    if (!entries.isEmpty())
        connect(m_recentMenu->addAction("Effacer la liste"), &QAction::triggered, this, [] { Session::clearRecent(); });
    connect(m_recentMenu->addAction("Nombre de notes mémorisées..."), &QAction::triggered, this,
            &MainWindow::askRecentLimit);
}

void MainWindow::askRecentLimit()
{
    bool ok = false;
    const int value = QInputDialog::getInt(
        this, "Notes fermées récemment",
        QString("Nombre de notes à mémoriser (%1 à %2) :").arg(Session::kMinRecent).arg(Session::kMaxRecent),
        Session::recentLimit(), Session::kMinRecent, Session::kMaxRecent, 1, &ok);
    if (ok)
        Session::setRecentLimit(value);
}

void MainWindow::reopenRecent(const Session::DraftEntry &entry)
{
    if (!entry.filePath.isEmpty() && Session::isExternalFile(entry.filePath)) {
        if (!QFileInfo::exists(entry.filePath)) {
            Session::removeRecent(entry.id);
            statusBar()->showMessage("Fichier introuvable : " + entry.filePath, 4000);
            return;
        }
        openPath(entry.filePath); // relit le fichier sur le disque
        return;
    }
    for (const Session::DraftEntry &draft : Session::listDrafts()) {
        if (draft.id == entry.id) {
            openDraft(draft);
            return;
        }
    }
    Session::removeRecent(entry.id);
}

void MainWindow::updatePinAction()
{
    if (!m_pinAction || !m_unpinAction)
        return;
    Editor *editor = currentEditor();
    const bool pinned = editor && editor->pinned;
    m_pinAction->setEnabled(editor && !pinned);
    m_unpinAction->setEnabled(pinned);
}

void MainWindow::setCurrentPinned(bool pinned)
{
    if (Editor *editor = currentEditor())
        setPinned(editor->sessionId, pinned);
}

bool MainWindow::isMarkdown(const Editor *editor)
{
    if (!editor || editor->filePath.isEmpty())
        return false;
    const QString suffix = QFileInfo(editor->filePath).suffix().toLower();
    return suffix == "md" || suffix == "markdown";
}

// Le volet n'est visible que si l'aperçu est activé ET que l'onglet actif est un
// fichier Markdown ; l'icône n'est active que pour un tel onglet.
void MainWindow::updatePreviewState()
{
    if (!m_previewAction)
        return;
    Editor *editor = currentEditor();
    const bool markdown = isMarkdown(editor);
    m_previewAction->setEnabled(markdown);
    m_exportHtmlAction->setEnabled(markdown);
    const bool show = markdown && m_previewAction->isChecked();
    const bool wasVisible = m_preview->isVisible();
    const bool sameNote = markdown && m_hasPreviewKey && m_previewNoteId == editor->sessionId;
    m_preview->setVisible(show);
    if (show) {
        if (wasVisible && sameNote)
            m_previewTimer->start(); // simple changement d'état (ex. « modifié ») : rendu différé comme à la frappe
        else
            renderPreview();
        syncPreviewScroll();
    }
}

// Le défilement de l'aperçu suit celui de l'éditeur, proportionnellement à la longueur
// de chacun.
void MainWindow::syncPreviewScroll()
{
    Editor *editor = currentEditor();
    if (!editor || !m_preview->isVisible())
        return;
    const QScrollBar *source = editor->verticalScrollBar();
    QScrollBar *target = m_preview->verticalScrollBar();
    const double ratio = source->maximum() > 0 ? double(source->value()) / source->maximum() : 0.0;
    target->setValue(qRound(ratio * target->maximum()));
}

// Exporte le rendu de la note Markdown active dans un fichier HTML.
void MainWindow::exportHtml()
{
    Editor *editor = currentEditor();
    if (!isMarkdown(editor))
        return;
    const QString start = QFileInfo(editor->filePath).absolutePath() + "/" + QFileInfo(editor->filePath).completeBaseName() + ".html";
    QString path = QFileDialog::getSaveFileName(this, "Exporter en HTML", start,
                                                "Pages HTML (*.html *.htm);;Tous les fichiers (*)");
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".html";
    if (!writeHtml(editor, path)) {
        QMessageBox::warning(this, "Exporter en HTML", "Impossible d'écrire le fichier :\n" + path);
        return;
    }
    statusBar()->showMessage("Exporté en HTML : " + path, 4000);
}

// Page HTML autonome (UTF-8, titre = nom du fichier) du texte actuel de la note. Les images
// et liens relatifs restent relatifs au dossier du fichier Markdown : ils s'affichent si la
// page est enregistrée dans ce même dossier (proposé par défaut).
bool MainWindow::writeHtml(const Editor *editor, const QString &path)
{
    QTextDocument doc;
    doc.setMarkdown(editor->toPlainText());
    doc.setMetaInformation(QTextDocument::DocumentTitle, QFileInfo(editor->filePath).fileName());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(doc.toHtml().toUtf8());
    return true;
}

void MainWindow::schedulePreview()
{
    if (m_preview->isVisible())
        m_previewTimer->start();
}

void MainWindow::renderPreview()
{
    Editor *editor = currentEditor();
    if (!isMarkdown(editor) || !m_preview->isVisible())
        return;
    const QString text = editor->toPlainText();
    if (m_hasPreviewKey && m_previewNoteId == editor->sessionId && m_previewPath == editor->filePath
        && m_previewText == text)
        return;
    const bool sameNote = m_hasPreviewKey && m_previewNoteId == editor->sessionId;
    QScrollBar *bar = m_preview->verticalScrollBar();
    const int scroll = sameNote ? bar->value() : 0;
    m_hasPreviewKey = true;
    m_previewNoteId = editor->sessionId;
    m_previewPath = editor->filePath;
    m_previewText = text;
    // les images et liens relatifs se résolvent depuis le dossier du fichier
    m_preview->setSearchPaths({QFileInfo(editor->filePath).absolutePath()});
    m_preview->setMarkdown(text);
    bar->setValue(scroll);
    syncPreviewScroll();
    QTimer::singleShot(0, this, &MainWindow::syncPreviewScroll); // la plage de défilement se met à jour après la mise en page
}

void MainWindow::updateTitle()
{
    refreshTabTooltips();
    updatePinAction();
    updatePreviewState();
    Editor *editor = currentEditor();
    if (!editor) {
        setWindowTitle("Éditeur de note");
        highlightActiveDraft();
        return;
    }
    const int index = m_tabs->currentIndex();
    m_tabs->setTabText(index, tabLabel(editor));
    setWindowTitle(tabLabel(editor) + " — Éditeur de note");
    highlightActiveDraft();
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(this, "Ouvrir un fichier", QString(), kFileFilters);
    if (!path.isEmpty())
        openPath(path);
}

void MainWindow::openPath(const QString &path)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
        if (editor && editor->filePath == path) {
            m_tabs->setCurrentIndex(i);
            return;
        }
    }
    const Session::DraftEntry existing = Session::findDraftForPath(path);
    if (!existing.id.isEmpty() && existing.modified) {
        // une session précédente de ce fichier a laissé des modifications non
        // enregistrées : on les retrouve plutôt que de les écraser
        openDraft(existing);
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", QString("Impossible d'ouvrir le fichier :\n%1").arg(file.errorString()));
        return;
    }
    const QString content = QString::fromUtf8(file.readAll());
    // même identifiant que le brouillon existant du fichier : une seule entrée
    // par fichier dans le panneau, et l'historique des versions suit
    Editor *editor = newTab(path, content, QString(), existing.id);
    editor->diskMTime = QFileInfo(path).lastModified().toMSecsSinceEpoch();
}

bool MainWindow::saveFile()
{
    Editor *editor = currentEditor();
    if (!editor)
        return false;
    if (editor->filePath.isEmpty()) {
        QDir().mkpath(Session::docsDir());
        const QString path = Session::docsDir() + "/" + editor->defaultName + ".md";
        return writeFile(editor, path);
    }
    return writeFile(editor, editor->filePath);
}

bool MainWindow::saveFileAs()
{
    Editor *editor = currentEditor();
    if (!editor)
        return false;
    const QString start = !editor->filePath.isEmpty()
        ? editor->filePath
        : Session::docsDir() + "/" + editor->defaultName + ".md";
    QString selectedFilter = saveFilterFor(start);
    const QString path = QFileDialog::getSaveFileName(this, "Enregistrer sous", start, kFileFilters, &selectedFilter);
    if (path.isEmpty())
        return false;
    return writeFile(editor, path);
}

// Enregistrer (sous) vers un fichier qui a déjà une note fermée : on reprend son
// identifiant au lieu de créer une seconde entrée pour le même fichier dans le
// panneau Brouillons (son historique des versions et son état épinglé sont
// conservés). Si cette note est ouverte dans un autre onglet, on laisse les
// choses telles quelles.
void MainWindow::adoptExistingDraft(Editor *editor, const QString &path)
{
    if (path == editor->filePath)
        return;
    const Session::DraftEntry existing = Session::findDraftForPath(path);
    if (existing.id.isEmpty() || existing.id == editor->sessionId)
        return;
    if (tabIndexForId(existing.id) != -1)
        return;
    const QString oldId = editor->sessionId;
    const bool pinned = editor->pinned || existing.pinned;
    Session::mergeVersions(oldId, existing.id);
    Session::deleteDraft(oldId);
    editor->sessionId = existing.id;
    if (pinned != existing.pinned)
        Session::setPinned(existing.id, pinned);
    if (pinned != editor->pinned) {
        editor->pinned = pinned;
        refreshTabButton(editor);
    }
}

bool MainWindow::writeFile(Editor *editor, const QString &path)
{
    adoptExistingDraft(editor, path);
    if (QFileInfo::exists(path)) {
        QFile previous(path);
        if (previous.open(QIODevice::ReadOnly | QIODevice::Text))
            Session::saveVersion(editor->sessionId, QString::fromUtf8(previous.readAll()));
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", QString("Impossible d'enregistrer le fichier :\n%1").arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << editor->toPlainText();
    file.close();

    editor->setFilePath(path);
    editor->document()->setModified(false);
    editor->diskMTime = QFileInfo(path).lastModified().toMSecsSinceEpoch();

    Session::TabSnapshot snap;
    snap.id = editor->sessionId;
    snap.filePath = editor->filePath;
    snap.defaultName = editor->defaultName;
    snap.modified = false;
    snap.content = editor->toPlainText();
    Session::saveDraft(snap);

    updateTitle();
    refreshDraftsBrowser();
    return true;
}

// Alerte avant de fermer un fichier extérieur modifié.
MainWindow::CloseChoice MainWindow::askSaveBeforeClose(Editor *editor)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle("Modifications non enregistrées");
    box.setText(QString("« %1 » a été modifié et n'est pas enregistré.").arg(QFileInfo(editor->filePath).fileName()));
    box.setInformativeText("Enregistrez-le pour ne pas perdre vos modifications : une fois fermé, il ne sera plus "
                           "listé dans les brouillons.");
    QPushButton *save = box.addButton("Enregistrer", QMessageBox::AcceptRole);
    QPushButton *discard = box.addButton("Ne pas enregistrer", QMessageBox::DestructiveRole);
    box.addButton("Annuler", QMessageBox::RejectRole);
    box.setDefaultButton(save);
    box.exec();
    if (box.clickedButton() == save)
        return CloseChoice::Save;
    if (box.clickedButton() == discard)
        return CloseChoice::Discard;
    return CloseChoice::Cancel;
}

// Après « Ne pas enregistrer » : le brouillon (masqué) ne doit pas garder les
// modifications abandonnées, sinon elles ressusciteraient à la réouverture.
void MainWindow::archiveUnmodified(Editor *editor)
{
    QString content = editor->toPlainText();
    QFile file(editor->filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        content = QString::fromUtf8(file.readAll());

    Session::TabSnapshot snap;
    snap.id = editor->sessionId;
    snap.filePath = editor->filePath;
    snap.defaultName = editor->defaultName;
    snap.modified = false;
    snap.content = content;
    Session::saveDraft(snap);
}

bool MainWindow::closeTab(int index)
{
    auto *editor = qobject_cast<Editor *>(m_tabs->widget(index));
    if (!editor)
        return true;
    if (editor->pinned) {
        // Seul point de passage de toutes les fermetures (croix, Ctrl+W, menus,
        // « fermer les autres/à droite/tout ») : les notes épinglées sont
        // simplement ignorées par les fermetures en lot.
        statusBar()->showMessage("Note épinglée : détachez-la pour la fermer.", 3000);
        return true;
    }
    const bool external = Session::isExternalFile(editor->filePath);
    if (external && editor->document()->isModified()) {
        const CloseChoice choice = askSaveBeforeClose(editor);
        if (choice == CloseChoice::Cancel)
            return false;
        if (choice == CloseChoice::Save) {
            if (!writeFile(editor, editor->filePath))
                return false;
        } else {
            archiveUnmodified(editor);
        }
    }
    if (editor->document()->isModified() && !external) {
        Session::TabSnapshot snap;
        snap.id = editor->sessionId;
        snap.filePath = editor->filePath;
        snap.defaultName = editor->defaultName;
        snap.modified = true;
        snap.content = editor->toPlainText();
        Session::saveDraft(snap);
    }

    // une note vide sans fichier n'a rien à rouvrir, sauf si elle a demandé à être mémorisée
    if (!editor->filePath.isEmpty() || !editor->toPlainText().trimmed().isEmpty() || Session::isRemembered(editor->sessionId)) {
        Session::DraftEntry recent;
        recent.id = editor->sessionId;
        recent.filePath = editor->filePath;
        recent.defaultName = editor->defaultName;
        Session::addRecent(recent);
    }
    m_tabs->removeTab(index);
    editor->deleteLater();
    refreshDraftsBrowser();
    repositionNewTabButton();
    updateTitle();
    return true;
}

void MainWindow::saveSessionToDisk()
{
    QVector<Session::TabSnapshot> tabsInfo;
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *editor = qobject_cast<Editor *>(m_tabs->widget(i));
        if (!editor)
            continue;
        Session::TabSnapshot snap;
        snap.id = editor->sessionId;
        snap.filePath = editor->filePath;
        snap.defaultName = editor->defaultName;
        snap.modified = editor->document()->isModified();
        snap.content = editor->toPlainText();
        editor->viewState(&snap.cursor, &snap.scroll, &snap.anchor);
        snap.history = editor->historyState();
        tabsInfo.append(snap);
    }
    Editor *activeEditor = currentEditor();
    const QString activeId = activeEditor ? activeEditor->sessionId : QString();
    Session::saveSession(tabsInfo, activeId);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSessionToDisk();
    Session::saveWindowState(width(), height(), splitterSizesToSave(), x(), y(), m_wordWrapAction->isChecked(),
                             m_previewAction->isChecked(), m_draftsPanelAction->isChecked());
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    repositionNewTabButton();
}
