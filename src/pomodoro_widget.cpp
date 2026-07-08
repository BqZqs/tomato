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
    connect(LocaleManager::instance(), &LocaleManager::fontOffsetChanged,
            this, &PomodoroWidget::onFontOffsetChanged);

    onFontOffsetChanged(LocaleManager::instance()->fontOffset());

    // Connect all three spinboxes to update timer duration
    auto updateDuration = [this]() {
        int totalSecs = m_hoursSpinBox->value() * 3600
                      + m_minutesSpinBox->value() * 60
                      + m_secondsSpinBox->value();
        m_timer->setDurationSec(totalSecs);
        refreshDisplay();
    };

    connect(m_hoursSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, updateDuration);
    connect(m_minutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, updateDuration);
    connect(m_secondsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, updateDuration);

    // Initialize spinboxes from default duration
    int dur = m_timer->durationMinutes();
    m_hoursSpinBox->setValue(dur / 60);
    m_minutesSpinBox->setValue(dur % 60);
    m_secondsSpinBox->setValue(0);

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
    m_hoursSpinBox->setValue(minutes / 60);
    m_minutesSpinBox->setValue(minutes % 60);
    m_secondsSpinBox->setValue(0);
    m_timer->setDurationSec(minutes * 60);
    m_timer->reset();
    m_timer->start();
}

// ---------------------------------------------------------------------------
void PomodoroWidget::setupUi()
{
    auto *lay = new QVBoxLayout(this);
    lay->setSpacing(12);
    lay->setContentsMargins(24, 16, 24, 16);

    m_display = new QLabel(QStringLiteral("00:25:00"));
    m_display->setAlignment(Qt::AlignCenter);
    m_display->setStyleSheet(
        QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                       "color:#1A1A2E;background:#F0F2F5;border-radius:10px;padding:16px"));
    lay->addWidget(m_display);

    m_status = new QLabel(loc("Ready"));
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#7B7D8C"));
    lay->addWidget(m_status);

    // Centered HBox with H:M:S spinboxes and buttons
    auto *cl = new QHBoxLayout;
    cl->setSpacing(4);
    cl->addStretch();

    m_hoursSpinBox = new QSpinBox;
    m_hoursSpinBox->setRange(0, 99);
    m_hoursSpinBox->setValue(0);
    m_hoursSpinBox->setFixedWidth(60);
    m_hoursSpinBox->setStyleSheet(QStringLiteral("font-size:13px;"));
    cl->addWidget(m_hoursSpinBox);

    m_colon1 = new QLabel(QStringLiteral(":"));
    m_colon1->setStyleSheet(QStringLiteral("font-size:14px;font-weight:bold;color:#1A1A2E;"));
    cl->addWidget(m_colon1);

    m_minutesSpinBox = new QSpinBox;
    m_minutesSpinBox->setRange(0, 59);
    m_minutesSpinBox->setValue(25);
    m_minutesSpinBox->setFixedWidth(60);
    m_minutesSpinBox->setStyleSheet(QStringLiteral("font-size:13px;"));
    cl->addWidget(m_minutesSpinBox);

    m_colon2 = new QLabel(QStringLiteral(":"));
    m_colon2->setStyleSheet(QStringLiteral("font-size:14px;font-weight:bold;color:#1A1A2E;"));
    cl->addWidget(m_colon2);

    m_secondsSpinBox = new QSpinBox;
    m_secondsSpinBox->setRange(0, 59);
    m_secondsSpinBox->setValue(0);
    m_secondsSpinBox->setFixedWidth(60);
    m_secondsSpinBox->setStyleSheet(QStringLiteral("font-size:13px;"));
    cl->addWidget(m_secondsSpinBox);

    cl->addSpacing(12);

    m_btnStart = new QPushButton(QStringLiteral("\u25B6")); // ▶
    m_btnStart->setFixedSize(40, 36);
    m_btnStart->setStyleSheet(
        QStringLiteral("font-size:16px;font-weight:bold;color:#22C55E;"));
    cl->addWidget(m_btnStart);

    m_btnReset = new QPushButton(QStringLiteral("\u25A0")); // ■
    m_btnReset->setFixedSize(40, 36);
    m_btnReset->setStyleSheet(
        QStringLiteral("font-size:16px;font-weight:bold;color:#EF4444;"));
    cl->addWidget(m_btnReset);

    cl->addStretch();
    lay->addLayout(cl);

    m_sessions = new QLabel(loc("Sessions: %1").arg(0));
    m_sessions->setAlignment(Qt::AlignCenter);
    m_sessions->setStyleSheet(QStringLiteral("font-size:12px;color:#B0B3BF"));
    lay->addWidget(m_sessions);

    lay->addStretch();
}

// ── Language ──────────────────────────────────────────────────────────────

void PomodoroWidget::refreshTexts()
{
    m_status->setText(loc("Ready"));
    m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#7B7D8C"));
    m_btnReset->setText(QStringLiteral("\u25A0"));
    m_sessions->setText(loc("Sessions: %1").arg(m_timer->completedSessions()));
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
                       "color:white;background:#EF4444;border-radius:10px;padding:16px"));
    m_status->setText(loc("Done!"));
    m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#EF4444;font-weight:bold"));
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
        m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#22C55E"));
        m_display->setStyleSheet(
            QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                           "color:#1A1A2E;background:#F0F2F5;border-radius:10px;padding:16px"));
    } else if (s == PomodoroTimer::State::Paused) {
        m_status->setText(loc("Paused"));
        m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#F59E0B"));
    } else {
        m_status->setText(loc("Ready"));
        m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#7B7D8C"));
        m_display->setStyleSheet(
            QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                           "color:#1A1A2E;background:#F0F2F5;border-radius:10px;padding:16px"));
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

void PomodoroWidget::onFontOffsetChanged(int offset)
{
    int displaySize = qMax(20, 56 + offset);
    int statusSize = qMax(8, 13 + offset);
    int sessionsSize = qMax(8, 12 + offset);
    int spinboxSize = qMax(8, 13 + offset);
    int btnSize = qMax(8, 16 + offset);

    m_display->setStyleSheet(
        QStringLiteral("font-size:%1px;font-weight:bold;font-family:Consolas;"
                       "color:#1A1A2E;background:#F0F2F5;border-radius:10px;padding:16px")
        .arg(displaySize));
    m_status->setStyleSheet(QStringLiteral("font-size:%1px;color:#7B7D8C").arg(statusSize));
    m_sessions->setStyleSheet(QStringLiteral("font-size:%1px;color:#B0B3BF").arg(sessionsSize));
    m_hoursSpinBox->setStyleSheet(QStringLiteral("font-size:%1px; padding:2px 4px;").arg(spinboxSize));
    m_minutesSpinBox->setStyleSheet(QStringLiteral("font-size:%1px; padding:2px 4px;").arg(spinboxSize));
    m_secondsSpinBox->setStyleSheet(QStringLiteral("font-size:%1px; padding:2px 4px;").arg(spinboxSize));
    m_colon1->setStyleSheet(QStringLiteral("font-size:%1px; font-weight:bold;").arg(spinboxSize + 2));
    m_colon2->setStyleSheet(QStringLiteral("font-size:%1px; font-weight:bold;").arg(spinboxSize + 2));
    m_btnStart->setStyleSheet(
        QStringLiteral("font-size:%1px;font-weight:bold;color:#22C55E;").arg(btnSize));
    m_btnReset->setStyleSheet(
        QStringLiteral("font-size:%1px;font-weight:bold;color:#EF4444;").arg(btnSize));
}

QString PomodoroWidget::fmt(int secs)
{
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}
