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
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QMenu;
class QSplitter;
class QTextBrowser;
class QTimer;
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
    // Renvoie false seulement si l'utilisateur a annulé (les fermetures en lot
    // s'arrêtent alors) ; true sinon, y compris pour une note épinglée ignorée.
    bool closeTab(int index);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void updateTitle();
    void showAbout();
    void toggleNewTabButtonMode();
    void positionInlineNewTabButton();
    void openDraft(const Session::DraftEntry &entry);
    void trashDraftEntry(const Session::DraftEntry &entry);
    void renameDraftEntry(const Session::DraftEntry &entry);
    void showTrash();
    void showTabContextMenu(const QPoint &pos);
    void checkCurrentExternalChange();
    void onAppStateChanged(Qt::ApplicationState state);
    void setWordWrap(bool enabled);
    void updateStatusBar();
    void handleDraftsContextAction(const QString &action, const Session::DraftEntry &entry);

private:
    void createActions();
    void createMenu();
    void createToolBar();
    void createStatusBar();
    QToolButton *buildNewTabButton(QWidget *parent);
    QWidget *makeCloseButton();
    void refreshTabButton(Editor *editor);
    void refreshTabTooltips();
    void setPinned(const QString &draftId, bool pinned);
    void applyPinned(const QString &draftId, bool pinned);
    void finishPinChange();
    void handleBulkAction(const QString &action, const QVector<Session::DraftEntry> &entries);
    void trashNow(const Session::DraftEntry &entry);
    void afterTrash();
    void trashEntries(const QVector<Session::DraftEntry> &entries);
    void updatePinAction();
    void refreshRecentMenu();
    void reopenRecent(const Session::DraftEntry &entry);
    void normalizeTabOrder();
    void setCurrentPinned(bool pinned);
    static bool isMarkdown(const Editor *editor);
    void updatePreviewState();
    void schedulePreview();
    void renderPreview();
    void closeTabByButton(QToolButton *button);
    QVector<QToolButton *> nativeScrollButtons() const;
    void repositionNewTabButton();
    void refreshDraftsBrowser();
    void highlightActiveDraft();
    bool writeFile(Editor *editor, const QString &path);
    void adoptExistingDraft(Editor *editor, const QString &path);
    QString tabLabel(Editor *editor) const;
    static QString timestampName();
    void restoreSession();
    void saveSessionToDisk();
    void openPath(const QString &path);
    void autosaveTab(Editor *editor);
    void renameTab(int index);
    void duplicateTab(int index);
    void closeOtherTabs(int index);
    void closeAllTabs();
    void closeTabsToTheRight(int index);
    void closeEntries(const QVector<Session::DraftEntry> &entries);
    enum class CloseChoice { Save, Discard, Cancel };
    CloseChoice askSaveBeforeClose(Editor *editor);
    void archiveUnmodified(Editor *editor);
    void showVersionHistory(Editor *editor);
    void applyRestoredVersion(Editor *editor, const QString &content);
    void checkExternalChange(Editor *editor);
    int tabIndexForId(const QString &draftId) const;
    void duplicateDraftEntry(const Session::DraftEntry &entry);
    void showVersionHistoryForEntry(const Session::DraftEntry &entry);

    QTabWidget *m_tabs;
    QToolButton *m_newTabButton;
    CornerToolButton *m_newTabCornerButton;
    DraftsBrowser *m_draftsBrowser;
    QLineEdit *m_draftsSearch;
    QComboBox *m_draftsSort;
    QToolButton *m_trashButton;
    QSplitter *m_splitter;
    QMenu *m_recentMenu = nullptr;
    QSplitter *m_editorSplitter;
    QTextBrowser *m_preview;
    QTimer *m_previewTimer;
    QString m_previewNoteId, m_previewPath, m_previewText;
    bool m_hasPreviewKey = false;
    FindReplaceDialog *m_findDialog;
    QLabel *m_statusPosition;
    QLabel *m_statusCounts;
    QLabel *m_statusEncoding;

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
    QAction *m_trashAction;
    QAction *m_pinAction = nullptr;
    QAction *m_unpinAction = nullptr;
    QAction *m_previewAction = nullptr;
    QAction *m_wordWrapAction;
    bool m_wordWrapEnabled = true;
    bool m_normalizingTabs = false;
};
