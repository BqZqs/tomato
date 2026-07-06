#pragma once

#include <QObject>
#include <QTimer>

class PomodoroTimer : public QObject
{
    Q_OBJECT

public:
    enum class State { Stopped, Running, Paused };

    explicit PomodoroTimer(QObject *parent = nullptr);

    int remainingSeconds() const { return m_remaining; }
    int totalSeconds() const { return m_total; }
    State state() const { return m_state; }
    int completedSessions() const { return m_completed; }

    int durationMinutes() const { return m_durationSec / 60; }
    void setDuration(int minutes);

public slots:
    void start();
    void pause();
    void reset();

signals:
    void tick(int remaining);
    void finished();
    void stateChanged(State state);
    void sessionCompleted(int total);

private slots:
    void onTimeout();

private:
    void applyDuration();

    QTimer *m_timer;
    int m_remaining = 0;
    int m_total = 0;
    State m_state = State::Stopped;
    int m_completed = 0;

    int m_durationSec = 25 * 60;
};
