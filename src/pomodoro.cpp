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
    m_total = m_durationSec;
    m_remaining = m_total;
}

void PomodoroTimer::setDuration(int minutes)
{
    if (minutes < 1 || minutes > 120) return;
    m_durationSec = minutes * 60;
    if (m_state == State::Stopped)
        applyDuration();
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

void PomodoroTimer::onTimeout()
{
    --m_remaining;
    emit tick(m_remaining);

    if (m_remaining <= 0) {
        m_timer->stop();
        m_state = State::Stopped;
        emit stateChanged(m_state);

        ++m_completed;
        emit sessionCompleted(m_completed);

        emit finished();
    }
}
