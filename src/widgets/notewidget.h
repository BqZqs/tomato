#pragma once

#include <QDate>
#include <QWidget>

class NoteData;
class QComboBox;
class QDateEdit;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;

class NoteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NoteWidget(NoteData *data, QWidget *parent = nullptr);

private slots:
    void onNotebookChanged(int index);
    void onManageNotebooks();
    void onPrevDay();
    void onNextDay();
    void onToday();
    void onDateEditChanged(const QDate &date);
    void onSave();
    void onDeleteNote();
    void onTextChanged();
    void refreshTexts();
    void onJumpPrev();
    void onJumpNext();
    void onFirst();
    void onLast();

private:
    void setupUi();
    void refreshNotebooks();
    void setDate(const QDate &date);
    void loadNote();
    void saveCurrentNoteIfDirty();
    void updateDirtyState();
    void openMostRecentNote();
    void updatePageLabel();

    NoteData *m_data = nullptr;
    QString m_currentNotebook;
    QDate m_currentDate;
    QString m_lastSavedText;
    bool m_dirty = false;

    // Notebook selector bar
    QComboBox *m_notebookCombo = nullptr;
    QPushButton *m_manageBtn = nullptr;
    QLabel *m_nbLabel = nullptr;

    // Date navigation
    QPushButton *m_prevBtn = nullptr;
    QDateEdit *m_dateEdit = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QPushButton *m_todayBtn = nullptr;

    // Editor
    QPlainTextEdit *m_editor = nullptr;

    // Notebook bar icon buttons
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;

    // Jump navigation
    QPushButton *m_jumpPrevBtn = nullptr;
    QPushButton *m_jumpNextBtn = nullptr;
    QPushButton *m_firstBtn = nullptr;
    QPushButton *m_lastBtn = nullptr;
    QLabel *m_pageLabel = nullptr;

    // No-notebooks view
    QStackedWidget *m_stack = nullptr;
    QWidget *m_noNotebooksPage = nullptr;
    QWidget *m_editorPage = nullptr;
    QPushButton *m_createFirstBtn = nullptr;
    QLabel *m_noNBLabel = nullptr;
};
