#include "pomodoro_widget.h"
#include "lang.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
PomodoroWidget::PomodoroWidget(QWidget *parent)
    : QWidget(parent)
    , m_timer(new PomodoroTimer(this))
{
    setupUi();

    connect(m_timer, &PomodoroTimer::tick,            this, &PomodoroWidget::onTick);
    connect(m_timer, &PomodoroTimer::finished,        this, &PomodoroWidget::onFinished);
    connect(m_timer, &PomodoroTimer::stateChanged,    this, &PomodoroWidget::onStateChanged);
    connect(m_timer, &PomodoroTimer::sessionCompleted, this, &PomodoroWidget::onSessionCompleted);

    connect(m_btnStart, &QPushButton::clicked, this, &PomodoroWidget::onStartPause);
    connect(m_btnReset, &QPushButton::clicked, this, &PomodoroWidget::onReset);

    connect(LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &PomodoroWidget::refreshTexts);

    // Connect spinbox to timer duration
    connect(m_minutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
                m_timer->setDuration(value);
                refreshDisplay();
            });

    // Initialize from saved default
    m_minutesSpinBox->setValue(LocaleManager::instance()->defaultDuration());
    m_timer->setDuration(m_minutesSpinBox->value());

    refreshDisplay();
    refreshButtons();
}

// ---------------------------------------------------------------------------
int PomodoroWidget::completedSessions() const
{
    return m_timer->completedSessions();
}

// ---------------------------------------------------------------------------
void PomodoroWidget::startTimedSession(const QString &taskId, int minutes)
{
    m_activeTaskId = taskId;
    m_minutesSpinBox->setValue(minutes);
    m_timer->setDuration(minutes);
    m_timer->reset();
    m_timer->start();
}

// ---------------------------------------------------------------------------
void PomodoroWidget::setupUi()
{
    auto *lay = new QVBoxLayout(this);
    lay->setSpacing(12);
    lay->setContentsMargins(24, 16, 24, 16);

    m_display = new QLabel(QStringLiteral("25:00"));
    m_display->setAlignment(Qt::AlignCenter);
    m_display->setStyleSheet(
        QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                       "color:#2c3e50;background:#ecf0f1;border-radius:10px;padding:16px"));
    lay->addWidget(m_display);

    m_status = new QLabel(loc("Ready"));
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#7f8c8d"));
    lay->addWidget(m_status);

    // Centered HBox with spinbox, min label, and buttons
    auto *cl = new QHBoxLayout;
    cl->setSpacing(8);
    cl->addStretch();

    m_minutesSpinBox = new QSpinBox;
    m_minutesSpinBox->setRange(1, 120);
    m_minutesSpinBox->setValue(25);
    m_minutesSpinBox->setFixedWidth(60);
    m_minutesSpinBox->setStyleSheet(QStringLiteral("font-size:13px;"));
    cl->addWidget(m_minutesSpinBox);

    m_minUnit = new QLabel(loc("min"));
    m_minUnit->setStyleSheet(QStringLiteral("font-size:12px;color:#7f8c8d"));
    cl->addWidget(m_minUnit);

    m_btnStart = new QPushButton(QStringLiteral("\u25B6")); // ▶
    m_btnStart->setFixedSize(40, 36);
    m_btnStart->setStyleSheet(
        QStringLiteral("font-size:16px;font-weight:bold;color:#27ae60;"));
    cl->addWidget(m_btnStart);

    m_btnReset = new QPushButton(QStringLiteral("\u25A0")); // ■
    m_btnReset->setFixedSize(40, 36);
    m_btnReset->setStyleSheet(
        QStringLiteral("font-size:16px;font-weight:bold;color:#e74c3c;"));
    cl->addWidget(m_btnReset);

    cl->addStretch();
    lay->addLayout(cl);

    m_sessions = new QLabel(loc("Sessions: %1").arg(0));
    m_sessions->setAlignment(Qt::AlignCenter);
    m_sessions->setStyleSheet(QStringLiteral("font-size:12px;color:#95a5a6"));
    lay->addWidget(m_sessions);

    lay->addStretch();
}

// ── Language ──────────────────────────────────────────────────────────────

void PomodoroWidget::refreshTexts()
{
    m_status->setText(loc("Ready"));
    m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#7f8c8d"));
    m_btnReset->setText(QStringLiteral("\u25A0"));
    m_sessions->setText(loc("Sessions: %1").arg(m_timer->completedSessions()));
    m_minUnit->setText(loc("min"));
    refreshDisplay();
    refreshButtons();
}

// ── Slots ─────────────────────────────────────────────────────────────────

void PomodoroWidget::onTick(int)              { refreshDisplay(); }
void PomodoroWidget::onSessionCompleted(int n)
{
    m_sessions->setText(loc("Sessions: %1").arg(n));
}

void PomodoroWidget::onFinished()
{
    m_display->setStyleSheet(
        QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                       "color:white;background:#e74c3c;border-radius:10px;padding:16px"));
    m_status->setText(loc("Done!"));
    m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#e74c3c;font-weight:bold"));
    QApplication::beep();

    if (!m_activeTaskId.isEmpty()) {
        emit timedSessionFinished(m_activeTaskId);
        m_activeTaskId.clear();
    }

    refreshButtons();
}

void PomodoroWidget::onStateChanged(PomodoroTimer::State s)
{
    refreshButtons();
    refreshDisplay();

    if (s == PomodoroTimer::State::Running) {
        m_status->setText(loc("Running..."));
        m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#27ae60"));
        m_display->setStyleSheet(
            QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                           "color:#2c3e50;background:#ecf0f1;border-radius:10px;padding:16px"));
    } else if (s == PomodoroTimer::State::Paused) {
        m_status->setText(loc("Paused"));
        m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#f39c12"));
    } else {
        m_status->setText(loc("Ready"));
        m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#7f8c8d"));
        m_display->setStyleSheet(
            QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                           "color:#2c3e50;background:#ecf0f1;border-radius:10px;padding:16px"));
    }
}

void PomodoroWidget::onStartPause()
{
    switch (m_timer->state()) {
    case PomodoroTimer::State::Stopped:
    case PomodoroTimer::State::Paused:  m_timer->start(); break;
    case PomodoroTimer::State::Running: m_timer->pause(); break;
    }
}

void PomodoroWidget::onReset() { m_timer->reset(); }

// ── Helpers ───────────────────────────────────────────────────────────────

void PomodoroWidget::refreshDisplay()
{
    m_display->setText(fmt(m_timer->remainingSeconds()));
}

void PomodoroWidget::refreshButtons()
{
    switch (m_timer->state()) {
    case PomodoroTimer::State::Running: m_btnStart->setText(QStringLiteral("\u23F8")); break;  // ⏸
    case PomodoroTimer::State::Paused:  m_btnStart->setText(QStringLiteral("\u25B6")); break;  // ▶
    case PomodoroTimer::State::Stopped: m_btnStart->setText(QStringLiteral("\u25B6")); break;  // ▶
    }
}

QString PomodoroWidget::fmt(int secs)
{
    return QStringLiteral("%1:%2")
        .arg(secs / 60, 2, 10, QChar('0'))
        .arg(secs % 60, 2, 10, QChar('0'));
}
