#include "MainWindow.h"
#include "DraftsBrowser.h"
#include "Editor.h"
#include "FindReplaceDialog.h"
#include "TrashDialog.h"
#include "VersionHistoryDialog.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
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
#include <QRegularExpression>
#include <QResizeEvent>
#include <QSplitter>
#include <QStatusBar>
#include <QStringConverter>
#include <QTabBar>
#include <QTabWidget>
#include <QTextCursor>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

namespace {

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

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(false);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
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

    m_draftsSearch = new QLineEdit(this);
    m_draftsSearch->setPlaceholderText("Rechercher...");
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
    connect(m_trashButton, &QToolButton::clicked, this, &MainWindow::showTrash);

    auto *sidebar = new QWidget(this);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(4);
    sidebarLayout->addWidget(new QLabel("Brouillons", sidebar));
    sidebarLayout->addLayout(draftsToolbar);
    sidebarLayout->addWidget(m_draftsBrowser);
    sidebarLayout->addWidget(m_trashButton);

    m_splitter = new QSplitter(this);
    m_splitter->addWidget(sidebar);
    m_splitter->addWidget(m_tabs);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({180, 720});
    setCentralWidget(m_splitter);

    const Session::WindowState windowState = Session::loadWindowState();
    if (windowState.valid) {
        resize(windowState.width, windowState.height);
        if (!windowState.splitterSizes.isEmpty())
            m_splitter->setSizes(windowState.splitterSizes);
    }

    m_findDialog = new FindReplaceDialog(this);

    createActions();
    createMenu();
    createToolBar();
    createStatusBar();

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

    m_closeTabAction = new QAction("&Fermer l'onglet", this);
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

    m_findAction = new QAction("&Rechercher...", this);
    m_findAction->setShortcut(QKeySequence::Find);
    connect(m_findAction, &QAction::triggered, m_findDialog, &FindReplaceDialog::showForFind);

    m_replaceAction = new QAction("Rechercher / &Remplacer...", this);
    m_replaceAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(m_replaceAction, &QAction::triggered, m_findDialog, &FindReplaceDialog::showForReplace);

    m_findNextAction = new QAction("Suivant", this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    connect(m_findNextAction, &QAction::triggered, m_findDialog, &FindReplaceDialog::findNext);

    m_aboutAction = new QAction("À &propos...", this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    m_trashAction = new QAction("&Corbeille...", this);
    connect(m_trashAction, &QAction::triggered, this, &MainWindow::showTrash);

    m_wordWrapAction = new QAction(wordWrapIcon(), "Retour automatique à la ligne", this);
    m_wordWrapAction->setCheckable(true);
    m_wordWrapAction->setChecked(true);
    connect(m_wordWrapAction, &QAction::toggled, this, &MainWindow::setWordWrap);
}

void MainWindow::createToolBar()
{
    auto *toolbar = new QToolBar("Barre d'outils", this);
    toolbar->setMovable(false);
    toolbar->addAction(m_newAction);
    toolbar->addAction(m_openAction);
    toolbar->addAction(m_saveAction);
    toolbar->addAction(m_saveAsAction);
    toolbar->addSeparator();
    toolbar->addAction(m_wordWrapAction);
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

void MainWindow::createStatusBar()
{
    m_statusPosition = new QLabel(this);
    m_statusCounts = new QLabel(this);
    m_statusEncoding = new QLabel("UTF-8", this);
    QStatusBar *bar = statusBar();
    bar->addPermanentWidget(m_statusPosition);
    bar->addPermanentWidget(m_statusCounts);
    bar->addPermanentWidget(m_statusEncoding);
    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    Editor *editor = currentEditor();
    if (!editor) {
        m_statusPosition->setText(QString());
        m_statusCounts->setText(QString());
        m_statusEncoding->setText(QString());
        return;
    }

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
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
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
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this](bool) { updateTitle(); });
    connect(editor, &Editor::autosaveRequested, this, [this, editor] { autosaveTab(editor); });
    editor->setLineWrapMode(m_wordWrapEnabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(editor, &QPlainTextEdit::textChanged, this, &MainWindow::updateStatusBar);

    const QString label = !filePath.isEmpty() ? QFileInfo(filePath).fileName() : editor->defaultName;
    const int index = m_tabs->addTab(editor, label);
    m_tabs->tabBar()->setTabButton(index, QTabBar::RightSide, makeCloseButton());
    m_tabs->setCurrentIndex(index);
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
    const QString content = Session::readDraft(entry.id);
    newTab(entry.filePath, content, entry.defaultName, entry.id, entry.modified);
}

void MainWindow::trashDraftEntry(const Session::DraftEntry &entry)
{
    const QString label = !entry.filePath.isEmpty()
        ? QFileInfo(entry.filePath).fileName()
        : (!entry.defaultName.isEmpty() ? entry.defaultName : entry.id.left(8));
    const auto result = QMessageBox::question(this, "Mettre à la corbeille",
        QString("Mettre « %1 » à la corbeille ?").arg(label),
        QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) {
        Session::trashDraft(entry.id);
        refreshDraftsBrowser();
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
        if (m_tabs->widget(i) != keep)
            closeTab(i);
    }
}

void MainWindow::closeAllTabs()
{
    for (int i = m_tabs->count() - 1; i >= 0; --i)
        closeTab(i);
}

void MainWindow::closeTabsToTheRight(int index)
{
    for (int i = m_tabs->count() - 1; i > index; --i)
        closeTab(i);
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
    QAction *closeOthersAction = menu.addAction("Fermer les autres");
    QAction *closeRightAction = menu.addAction("Fermer à droite");
    closeRightAction->setEnabled(index < m_tabs->count() - 1);
    QAction *closeAllAction = menu.addAction("Fermer tout");
    menu.addSeparator();
    QAction *duplicateAction = menu.addAction("Dupliquer");
    QAction *renameAction = (editor && editor->filePath.isEmpty()) ? menu.addAction("Renommer...") : nullptr;
    QAction *historyAction = (editor && !Session::listVersions(editor->sessionId).isEmpty())
        ? menu.addAction("Historique des versions...")
        : nullptr;

    QAction *chosen = menu.exec(bar->mapToGlobal(pos));
    if (chosen == closeAction)
        closeTab(index);
    else if (chosen == closeOthersAction)
        closeOtherTabs(index);
    else if (chosen == closeRightAction)
        closeTabsToTheRight(index);
    else if (chosen == closeAllAction)
        closeAllTabs();
    else if (chosen == duplicateAction)
        duplicateTab(index);
    else if (renameAction && chosen == renameAction)
        renameTab(index);
    else if (historyAction && chosen == historyAction)
        showVersionHistory(editor);
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
    int activeIndex = 0;
    for (int i = 0; i < entries.size(); ++i) {
        const Session::TabSnapshot &entry = entries[i];
        Editor *editor = newTab(entry.filePath, entry.content, entry.defaultName, entry.id, entry.modified);
        if (!entry.filePath.isEmpty() && QFileInfo::exists(entry.filePath))
            editor->diskMTime = QFileInfo(entry.filePath).lastModified().toMSecsSinceEpoch();
        if (entry.id == activeId)
            activeIndex = i;
    }
    m_tabs->setCurrentIndex(activeIndex);
}

QString MainWindow::tabLabel(Editor *editor) const
{
    const QString name = !editor->filePath.isEmpty() ? QFileInfo(editor->filePath).fileName() : editor->defaultName;
    return editor->document()->isModified() ? "*" + name : name;
}

void MainWindow::updateTitle()
{
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
    const QString path = QFileDialog::getOpenFileName(this, "Ouvrir un fichier", QString(),
                                                        "Fichiers texte (*.txt);;Tous les fichiers (*)");
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
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", QString("Impossible d'ouvrir le fichier :\n%1").arg(file.errorString()));
        return;
    }
    const QString content = QString::fromUtf8(file.readAll());
    Editor *editor = newTab(path, content);
    editor->diskMTime = QFileInfo(path).lastModified().toMSecsSinceEpoch();
}

bool MainWindow::saveFile()
{
    Editor *editor = currentEditor();
    if (!editor)
        return false;
    if (editor->filePath.isEmpty()) {
        QDir().mkpath(Session::docsDir());
        const QString path = Session::docsDir() + "/" + editor->defaultName + ".txt";
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
        : Session::docsDir() + "/" + editor->defaultName + ".txt";
    const QString path = QFileDialog::getSaveFileName(this, "Enregistrer sous", start,
                                                        "Fichiers texte (*.txt);;Tous les fichiers (*)");
    if (path.isEmpty())
        return false;
    return writeFile(editor, path);
}

bool MainWindow::writeFile(Editor *editor, const QString &path)
{
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

void MainWindow::closeTab(int index)
{
    auto *editor = qobject_cast<Editor *>(m_tabs->widget(index));
    if (!editor)
        return;
    if (editor->document()->isModified()) {
        Session::TabSnapshot snap;
        snap.id = editor->sessionId;
        snap.filePath = editor->filePath;
        snap.defaultName = editor->defaultName;
        snap.modified = true;
        snap.content = editor->toPlainText();
        Session::saveDraft(snap);
    }

    m_tabs->removeTab(index);
    editor->deleteLater();
    refreshDraftsBrowser();
    repositionNewTabButton();
    updateTitle();
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
        tabsInfo.append(snap);
    }
    Editor *activeEditor = currentEditor();
    const QString activeId = activeEditor ? activeEditor->sessionId : QString();
    Session::saveSession(tabsInfo, activeId);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSessionToDisk();
    Session::saveWindowState(width(), height(), m_splitter->sizes());
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    repositionNewTabButton();
}
