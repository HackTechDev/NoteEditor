#pragma once

#include "Session.h"

#include <QMainWindow>
#include <QSet>
#include <QString>
#include <QToolButton>

class Editor;
class DraftsBrowser;
class FindReplaceDialog;
class QAction;
class QSplitter;
class QTabWidget;
class QListWidgetItem;

// QToolButton whose sizeHint() forces its size, so QTabWidget sizes/centers
// it correctly as a corner widget (Qt bases that on sizeHint(), not on a
// size imposed afterwards via setFixedSize() — see MainWindow.cpp).
class CornerToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit CornerToolButton(int height, QWidget *parent = nullptr);

    void setForcedHeight(int height);
    QSize sizeHint() const override;

private:
    int m_forcedHeight;
};

// Owns the tab widget, menus/actions, the drafts sidebar, and all file I/O
// (open/save/close). Mirrors Python/main.py's MainWindow.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    Editor *currentEditor() const;
    Editor *newTab(const QString &filePath = QString(), const QString &content = QString(),
                   const QString &defaultName = QString(), const QString &sessionId = QString(),
                   bool modified = false);
    void closeTab(int index);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void updateTitle();
    void showAbout();
    void toggleNewTabButtonMode();
    void positionInlineNewTabButton();
    void openDraft(const Session::DraftEntry &entry);
    void deleteDraftEntry(const Session::DraftEntry &entry);

private:
    void createActions();
    void createMenu();
    QToolButton *buildNewTabButton(QWidget *parent);
    QWidget *makeCloseButton();
    void closeTabByButton(QToolButton *button);
    QVector<QToolButton *> nativeScrollButtons() const;
    void repositionNewTabButton();
    void refreshDraftsBrowser();
    void highlightActiveDraft();
    bool writeFile(Editor *editor, const QString &path);
    QString tabLabel(Editor *editor) const;
    static QString timestampName();
    void restoreSession();
    void saveSessionToDisk();

    QTabWidget *m_tabs;
    QToolButton *m_newTabButton;
    CornerToolButton *m_newTabCornerButton;
    DraftsBrowser *m_draftsBrowser;
    QSplitter *m_splitter;
    FindReplaceDialog *m_findDialog;

    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeTabAction;
    QAction *m_quitAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_selectAllAction;
    QAction *m_findAction;
    QAction *m_replaceAction;
    QAction *m_findNextAction;
    QAction *m_aboutAction;
};
