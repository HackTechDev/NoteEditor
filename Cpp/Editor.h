#pragma once

#include "Highlighters.h"

#include <QJsonObject>
#include <QPlainTextEdit>
#include <QString>
#include <QVector>
#include <QTimer>
#include <QUuid>
#include <QWidget>

class QPaintEvent;
class QKeyEvent;
class QResizeEvent;
class QShowEvent;
class Editor;

constexpr int kAutosaveDelayMs = 1500;

// The line-number gutter painted to the left of an Editor.
class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    explicit LineNumberArea(Editor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Editor *m_editor;
};

// One per tab. Owns the line-number gutter, current-line highlight, and a
// stable session_id used throughout the persistence layer (Session.h).
class Editor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);

    void setFilePath(const QString &path);

    // Replace le curseur et programme le défilement : la barre de défilement n'a
    // pas encore sa plage tant que l'onglet n'a jamais été affiché.
    // `anchor` >= 0 : début de la sélection dont `cursor` est la fin.
    void restoreView(int cursor, int scroll, int anchor = -1);
    // Position du curseur, défilement et début de la sélection ; le défilement
    // programmé mais pas encore appliqué (onglet jamais affiché depuis la
    // restauration) prime.
    void viewState(int *cursor, int *scroll, int *anchor) const;

    // Historique annuler/rétablir sérialisable (positions en unités UTF-16, comme Qt),
    // ou objet vide s'il n'y en a pas. Réduit aux dernières étapes de chaque côté du
    // point courant, et à un volume de texte raisonnable.
    QJsonObject historyState() const;
    // Reconstruit la pile annuler/rétablir à partir de historyState(). Sans effet
    // (et renvoie false) si elle ne correspond pas au texte actuel de l'onglet.
    bool restoreHistory(const QJsonObject &data);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    QString filePath;    // empty == no associated file
    QString defaultName; // empty == not applicable (has filePath instead)
    QString sessionId;
    bool pinned = false; // mirrors index.json, see Session::isPinned()
    qint64 diskMTime = -1; // -1 == unknown / not tracked yet


signals:
    // Fired ~1.5s after the last keystroke, while still unsaved. MainWindow
    // uses this to archive the draft continuously, not just on tab
    // creation/close/quit.
    void autosaveRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
    void trackHistory();
    void shiftLines(bool indent);

private:
    // Qt ne permet ni de lire ni d'exporter la pile annuler/rétablir : on en garde une
    // copie sous forme d'étapes (début, texte retiré, texte ajouté).
    struct HistoryStep {
        int start;
        QString removed;
        QString added;
    };
    LineNumberArea *m_lineNumberArea;
    QSyntaxHighlighter *m_highlighter = nullptr;
    HighlighterKind m_highlighterKind = HighlighterKind::None;
    QTimer *m_autosaveTimer;
    int m_pendingScroll = -1; // -1 == nothing to apply
    QVector<HistoryStep> m_histSteps;
    int m_histPos = 0;
    QString m_histText;
    bool m_histNewStep = false;
    bool m_histBusy = false;
};
