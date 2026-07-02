#pragma once
#include <QWidget>
#include "pomodoro.h"

class QLabel;
class QPushButton;
class QRadioButton;
class QGroupBox;

class PomodoroWidget : public QWidget {
    Q_OBJECT
public:
    explicit PomodoroWidget(QWidget *parent = nullptr);
    int completedSessions() const;  // delegate to m_timer
public slots:
    void refreshTexts(); // re-translate all UI strings

private slots:
    void onTick(int remaining);
    void onFinished();
    void onStateChanged(PomodoroTimer::State s);
    void onModeChanged(PomodoroTimer::Mode m);
    void onSessionCompleted(int n);
    void onStartPause();
    void onReset();
    void onModeRadio();

private:
    void setupUi();
    void refreshDisplay();
    void refreshButtons();
    static QString fmt(int secs);

    PomodoroTimer *m_timer;

    QLabel *m_display;
    QLabel *m_status;
    QLabel *m_sessions;
    QRadioButton *m_rbWork;
    QRadioButton *m_rbShort;
    QRadioButton *m_rbLong;
    QPushButton *m_btnStart;
    QPushButton *m_btnReset;
    QGroupBox *m_modeGroup;
};
