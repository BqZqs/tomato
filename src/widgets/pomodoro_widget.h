#pragma once
#include <QWidget>
#include "pomodoro.h"

class QLabel;
class QPushButton;
class QSpinBox;

class PomodoroWidget : public QWidget {
    Q_OBJECT
public:
    explicit PomodoroWidget(QWidget *parent = nullptr);
    int completedSessions() const;

public slots:
    void refreshTexts();
    void startTimedSession(const QString &taskId, int minutes);
    void onFontOffsetChanged(int offset);

signals:
    void timedSessionFinished(const QString &taskId, int elapsedMinutes);

private slots:
    void onTick(int remaining);
    void onFinished();
    void onStateChanged(PomodoroTimer::State s);
    void onSessionCompleted(int n);
    void onStartPause();
    void onReset();
    void onFinish();

private:
    void setupUi();
    void refreshDisplay();
    void refreshButtons();
    static QString fmt(int secs);

    PomodoroTimer *m_timer;

    QLabel *m_display;
    QLabel *m_status;
    QLabel *m_sessions;
    QSpinBox *m_hoursSpinBox;
    QSpinBox *m_minutesSpinBox;
    QSpinBox *m_secondsSpinBox;
    QLabel *m_colon1;
    QLabel *m_colon2;
    QPushButton *m_btnStart;
    QPushButton *m_btnReset;
    QPushButton *m_btnFinish;

    QString m_activeTaskId;
};
