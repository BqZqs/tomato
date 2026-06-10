#pragma once

#include <QObject>
#include <QTimer>

class PomodoroTimer : public QObject
{
    Q_OBJECT

public:
    enum class State { Stopped, Running, Paused };
    enum class Mode { Work, ShortBreak, LongBreak };

    explicit PomodoroTimer(QObject *parent = nullptr);

    int remainingSeconds() const { return m_remaining; }
    int totalSeconds() const { return m_total; }
    State state() const { return m_state; }
    Mode mode() const { return m_mode; }
    int completedSessions() const { return m_completed; }

public slots:
    void start();
    void pause();
    void reset();
    void setMode(Mode mode);

signals:
    void tick(int remaining);
    void finished();
    void stateChanged(State state);
    void modeChanged(Mode mode);
    void sessionCompleted(int total);

private slots:
    void onTimeout();

private:
    void applyDuration();

    QTimer *m_timer;
    int m_remaining = 0;
    int m_total = 0;
    State m_state = State::Stopped;
    Mode m_mode = Mode::Work;
    int m_completed = 0;

    static constexpr int WORK_SEC = 25 * 60;
    static constexpr int SHORT_SEC = 5 * 60;
    static constexpr int LONG_SEC = 15 * 60;
};
