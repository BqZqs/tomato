#pragma once

#include <QMainWindow>
#include "pomodoro.h"
#include "taskdata.h"
#include "notedata.h"

class QComboBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QRadioButton;
class QGroupBox;
class QStackedWidget;
class TaskWidget;
class NoteWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onTick(int remaining);
    void onFinished();
    void onStateChanged(PomodoroTimer::State s);
    void onModeChanged(PomodoroTimer::Mode m);
    void onSessionCompleted(int n);
    void onStartPause();
    void onReset();
    void onModeRadio();
    void onNavClicked(int index);
    void onLanguageChanged(int index);
    void refreshTexts();

private:
    void setupUi();
    void setupTimerPage(QWidget *page);
    void setupLanguageCombo(QHBoxLayout *navBar);
    void refreshDisplay();
    void refreshButtons();
    void updateNavButtons(int active);
    static QString fmt(int secs);
    QString dataDirPath() const;

    PomodoroTimer *m_timer;
    TaskData *m_taskData;
    NoteData *m_noteData;

    // Navigation
    QStackedWidget *m_stack;
    QPushButton *m_btnTimer;
    QPushButton *m_btnTasks;
    QPushButton *m_btnNotes;
    QComboBox *m_langCombo;

    // Timer page widgets
    QLabel *m_display;
    QLabel *m_status;
    QLabel *m_sessions;
    QRadioButton *m_rbWork;
    QRadioButton *m_rbShort;
    QRadioButton *m_rbLong;
    QPushButton *m_btnStart;
    QPushButton *m_btnReset;
    QGroupBox *m_modeGroup;

    // Task page
    TaskWidget *m_taskWidget;

    // Note page
    NoteWidget *m_noteWidget;
};
