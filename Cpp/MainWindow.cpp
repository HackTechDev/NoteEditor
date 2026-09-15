#include "MainWindow.h"
#include "DraftsBrowser.h"
#include "Editor.h"
#include "FindReplaceDialog.h"

#include <QAction>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSplitter>
#include <QStringConverter>
#include <QTabBar>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
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
    setWindowTitle("Éditeur de texte");
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
    connect(m_draftsBrowser, &DraftsBrowser::deleteRequested, this, &MainWindow::deleteDraftEntry);

    auto *sidebar = new QWidget(this);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);
    sidebarLayout->addWidget(new QLabel("Brouillons", sidebar));
    sidebarLayout->addWidget(m_draftsBrowser);

    m_splitter = new QSplitter(this);
    m_splitter->addWidget(sidebar);
    m_splitter->addWidget(m_tabs);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({180, 720});
    setCentralWidget(m_splitter);

    m_findDialog = new FindReplaceDialog(this);

    createActions();
    createMenu();

    statusBar();
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
    m_newAction = new QAction("&Nouveau", this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, [this] { newTab(); });

    m_openAction = new QAction("&Ouvrir...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);

    m_saveAction = new QAction("&Enregistrer", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);

    m_saveAsAction = new QAction("Enregistrer &sous...", this);
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
        "<h3>Éditeur de texte</h3>"
        "<p>Éditeur de texte à onglets écrit en C++ avec Qt6.</p>"
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

void MainWindow::deleteDraftEntry(const Session::DraftEntry &entry)
{
    const QString label = !entry.filePath.isEmpty()
        ? QFileInfo(entry.filePath).fileName()
        : (!entry.defaultName.isEmpty() ? entry.defaultName : entry.id.left(8));
    const auto result = QMessageBox::question(this, "Supprimer le brouillon",
        QString("Supprimer définitivement « %1 » ? Cette action est irréversible.").arg(label),
        QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) {
        Session::deleteDraft(entry.id);
        refreshDraftsBrowser();
    }
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
        newTab(entry.filePath, entry.content, entry.defaultName, entry.id, entry.modified);
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
        setWindowTitle("Éditeur de texte");
        highlightActiveDraft();
        return;
    }
    const int index = m_tabs->currentIndex();
    m_tabs->setTabText(index, tabLabel(editor));
    setWindowTitle(tabLabel(editor) + " — Éditeur de texte");
    highlightActiveDraft();
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(this, "Ouvrir un fichier", QString(),
                                                        "Fichiers texte (*.txt);;Tous les fichiers (*)");
    if (path.isEmpty())
        return;
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
    newTab(path, content);
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
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    repositionNewTabButton();
}
