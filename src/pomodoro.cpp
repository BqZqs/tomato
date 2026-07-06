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
    case Mode::Work:       m_total = m_workSec; break;
    case Mode::ShortBreak:  m_total = m_shortSec; break;
    case Mode::LongBreak:   m_total = m_longSec; break;
    }
    m_remaining = m_total;
}

void PomodoroTimer::setWorkDuration(int minutes)
{
    if (minutes < 1 || minutes > 120) return;
    m_workSec = minutes * 60;
    if (m_mode == Mode::Work) applyDuration();
}

void PomodoroTimer::setShortDuration(int minutes)
{
    if (minutes < 1 || minutes > 30) return;
    m_shortSec = minutes * 60;
    if (m_mode == Mode::ShortBreak) applyDuration();
}

void PomodoroTimer::setLongDuration(int minutes)
{
    if (minutes < 1 || minutes > 60) return;
    m_longSec = minutes * 60;
    if (m_mode == Mode::LongBreak) applyDuration();
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
