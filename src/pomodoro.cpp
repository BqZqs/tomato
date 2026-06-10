#include "pomodoro.h"

PomodoroTimer::PomodoroTimer(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &PomodoroTimer::onTimeout);
    applyDuration();
}

void PomodoroTimer::applyDuration()
{
    switch (m_mode) {
    case Mode::Work:       m_total = WORK_SEC; break;
    case Mode::ShortBreak:  m_total = SHORT_SEC; break;
    case Mode::LongBreak:   m_total = LONG_SEC; break;
    }
    m_remaining = m_total;
}

void PomodoroTimer::start()
{
    if (m_state == State::Running) return;
    if (m_state == State::Stopped) applyDuration();
    m_state = State::Running;
    m_timer->start();
    emit stateChanged(m_state);
}

void PomodoroTimer::pause()
{
    if (m_state != State::Running) return;
    m_state = State::Paused;
    m_timer->stop();
    emit stateChanged(m_state);
}

void PomodoroTimer::reset()
{
    m_timer->stop();
    m_state = State::Stopped;
    applyDuration();
    emit tick(m_remaining);
    emit stateChanged(m_state);
}

void PomodoroTimer::setMode(Mode mode)
{
    if (m_mode == mode) return;
    m_mode = mode;
    m_timer->stop();
    m_state = State::Stopped;
    applyDuration();
    emit tick(m_remaining);
    emit stateChanged(m_state);
    emit modeChanged(m_mode);
}

void PomodoroTimer::onTimeout()
{
    --m_remaining;
    emit tick(m_remaining);

    if (m_remaining <= 0) {
        m_timer->stop();
        m_state = State::Stopped;
        emit stateChanged(m_state);

        if (m_mode == Mode::Work) {
            ++m_completed;
            emit sessionCompleted(m_completed);
        }

        emit finished();
    }
}
