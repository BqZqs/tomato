#include "pomodoro_widget.h"
#include "lang.h"

#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
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
    connect(m_timer, &PomodoroTimer::modeChanged,     this, &PomodoroWidget::onModeChanged);
    connect(m_timer, &PomodoroTimer::sessionCompleted, this, &PomodoroWidget::onSessionCompleted);

    connect(m_rbWork,  &QRadioButton::clicked, this, &PomodoroWidget::onModeRadio);
    connect(m_rbShort, &QRadioButton::clicked, this, &PomodoroWidget::onModeRadio);
    connect(m_rbLong,  &QRadioButton::clicked, this, &PomodoroWidget::onModeRadio);
    connect(m_btnStart, &QPushButton::clicked, this, &PomodoroWidget::onStartPause);
    connect(m_btnReset, &QPushButton::clicked, this, &PomodoroWidget::onReset);

    connect(LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &PomodoroWidget::refreshTexts);
    connect(LocaleManager::instance(), &LocaleManager::durationsChanged,
            this, [this]() {
                auto *lm = LocaleManager::instance();
                m_workSpinBox->blockSignals(true);
                m_shortSpinBox->blockSignals(true);
                m_longSpinBox->blockSignals(true);
                m_workSpinBox->setValue(lm->workDuration());
                m_shortSpinBox->setValue(lm->shortDuration());
                m_longSpinBox->setValue(lm->longDuration());
                m_workSpinBox->blockSignals(false);
                m_shortSpinBox->blockSignals(false);
                m_longSpinBox->blockSignals(false);
                m_timer->setWorkDuration(lm->workDuration());
                m_timer->setShortDuration(lm->shortDuration());
                m_timer->setLongDuration(lm->longDuration());
                refreshDisplay();
                refreshTexts();
            });

    // Read initial durations from settings and apply to timer
    auto *lm = LocaleManager::instance();
    m_timer->setWorkDuration(lm->workDuration());
    m_timer->setShortDuration(lm->shortDuration());
    m_timer->setLongDuration(lm->longDuration());
    m_workSpinBox->setValue(lm->workDuration());
    m_shortSpinBox->setValue(lm->shortDuration());
    m_longSpinBox->setValue(lm->longDuration());

    // Connect spinbox changes
    connect(m_workSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PomodoroWidget::onDurationChanged);
    connect(m_shortSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PomodoroWidget::onDurationChanged);
    connect(m_longSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PomodoroWidget::onDurationChanged);

    refreshDisplay();
    refreshButtons();
}

// ---------------------------------------------------------------------------
int PomodoroWidget::completedSessions() const
{
    return m_timer->completedSessions();
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

    m_modeGroup = new QGroupBox(loc("Mode"));
    auto *ml = new QVBoxLayout(m_modeGroup);
    m_rbWork  = new QRadioButton(loc("Work (25 min)"));
    m_rbShort = new QRadioButton(loc("Short Break (5 min)"));
    m_rbLong  = new QRadioButton(loc("Long Break (15 min)"));
    m_rbWork->setChecked(true);
    ml->addWidget(m_rbWork);
    ml->addWidget(m_rbShort);
    ml->addWidget(m_rbLong);
    lay->addWidget(m_modeGroup);

    // Duration group
    m_durGroup = new QGroupBox(loc("Duration"));
    auto *dl = new QGridLayout(m_durGroup);
    dl->setSpacing(6);

    m_workSpinBox = new QSpinBox;
    m_workSpinBox->setRange(1, 120);
    m_workSpinBox->setValue(m_timer->workMinutes());
    m_workSpinBox->setFixedWidth(70);
    m_workSpinBox->setStyleSheet(QStringLiteral("font-size:12px;color:#2c3e50;padding:2px 4px"));
    m_workLabel = new QLabel(loc("Work:"));
    m_workLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#7f8c8d"));
    m_workUnit = new QLabel(loc("min"));
    m_workUnit->setStyleSheet(QStringLiteral("font-size:12px;color:#7f8c8d"));
    dl->addWidget(m_workLabel, 0, 0);
    dl->addWidget(m_workSpinBox, 0, 1);
    dl->addWidget(m_workUnit, 0, 2);

    m_shortSpinBox = new QSpinBox;
    m_shortSpinBox->setRange(1, 30);
    m_shortSpinBox->setValue(m_timer->shortMinutes());
    m_shortSpinBox->setFixedWidth(70);
    m_shortSpinBox->setStyleSheet(QStringLiteral("font-size:12px;color:#2c3e50;padding:2px 4px"));
    m_shortLabel = new QLabel(loc("Short Break:"));
    m_shortLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#7f8c8d"));
    m_shortUnit = new QLabel(loc("min"));
    m_shortUnit->setStyleSheet(QStringLiteral("font-size:12px;color:#7f8c8d"));
    dl->addWidget(m_shortLabel, 1, 0);
    dl->addWidget(m_shortSpinBox, 1, 1);
    dl->addWidget(m_shortUnit, 1, 2);

    m_longSpinBox = new QSpinBox;
    m_longSpinBox->setRange(1, 60);
    m_longSpinBox->setValue(m_timer->longMinutes());
    m_longSpinBox->setFixedWidth(70);
    m_longSpinBox->setStyleSheet(QStringLiteral("font-size:12px;color:#2c3e50;padding:2px 4px"));
    m_longLabel = new QLabel(loc("Long Break:"));
    m_longLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#7f8c8d"));
    m_longUnit = new QLabel(loc("min"));
    m_longUnit->setStyleSheet(QStringLiteral("font-size:12px;color:#7f8c8d"));
    dl->addWidget(m_longLabel, 2, 0);
    dl->addWidget(m_longSpinBox, 2, 1);
    dl->addWidget(m_longUnit, 2, 2);

    lay->addWidget(m_durGroup);

    auto *bl = new QHBoxLayout;
    m_btnStart = new QPushButton(loc("Start"));
    m_btnReset = new QPushButton(loc("Reset"));
    m_btnStart->setMinimumHeight(36);
    m_btnReset->setMinimumHeight(36);
    bl->addWidget(m_btnStart);
    bl->addWidget(m_btnReset);
    lay->addLayout(bl);

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
    m_modeGroup->setTitle(loc("Mode"));
    m_rbWork->setText(loc("Work (%1 min)").arg(m_timer->workMinutes()));
    m_rbShort->setText(loc("Short Break (%1 min)").arg(m_timer->shortMinutes()));
    m_rbLong->setText(loc("Long Break (%1 min)").arg(m_timer->longMinutes()));
    m_durGroup->setTitle(loc("Duration"));
    m_btnReset->setText(loc("Reset"));
    m_sessions->setText(loc("Sessions: %1").arg(m_timer->completedSessions()));

    // Also update the row labels and units in the Duration group
    m_workLabel->setText(loc("Work:"));
    m_shortLabel->setText(loc("Short Break:"));
    m_longLabel->setText(loc("Long Break:"));
    m_workUnit->setText(loc("min"));
    m_shortUnit->setText(loc("min"));
    m_longUnit->setText(loc("min"));

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
    } else if (m_timer->remainingSeconds() > 0 || m_timer->totalSeconds() == 0) {
        m_status->setText(loc("Ready"));
        m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#7f8c8d"));
        m_display->setStyleSheet(
            QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                           "color:#2c3e50;background:#ecf0f1;border-radius:10px;padding:16px"));
    }
}

void PomodoroWidget::onModeChanged(PomodoroTimer::Mode m)
{
    m_rbWork->blockSignals(true);
    m_rbShort->blockSignals(true);
    m_rbLong->blockSignals(true);
    switch (m) {
    case PomodoroTimer::Mode::Work:       m_rbWork->setChecked(true);  break;
    case PomodoroTimer::Mode::ShortBreak: m_rbShort->setChecked(true); break;
    case PomodoroTimer::Mode::LongBreak:  m_rbLong->setChecked(true);  break;
    }
    m_rbWork->blockSignals(false);
    m_rbShort->blockSignals(false);
    m_rbLong->blockSignals(false);
    refreshDisplay();
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

void PomodoroWidget::onModeRadio()
{
    PomodoroTimer::Mode m = PomodoroTimer::Mode::Work;
    if (m_rbShort->isChecked()) m = PomodoroTimer::Mode::ShortBreak;
    else if (m_rbLong->isChecked()) m = PomodoroTimer::Mode::LongBreak;
    m_timer->setMode(m);
}

void PomodoroWidget::onDurationChanged()
{
    auto *lm = LocaleManager::instance();
    int w = m_workSpinBox->value();
    int s = m_shortSpinBox->value();
    int l = m_longSpinBox->value();

    m_timer->setWorkDuration(w);
    m_timer->setShortDuration(s);
    m_timer->setLongDuration(l);

    lm->setWorkDuration(w);
    lm->setShortDuration(s);
    lm->setLongDuration(l);

    m_rbWork->setText(loc("Work (%1 min)").arg(m_timer->workMinutes()));
    m_rbShort->setText(loc("Short Break (%1 min)").arg(m_timer->shortMinutes()));
    m_rbLong->setText(loc("Long Break (%1 min)").arg(m_timer->longMinutes()));
    refreshDisplay();
}

// ── Helpers ───────────────────────────────────────────────────────────────

void PomodoroWidget::refreshDisplay()
{
    m_display->setText(fmt(m_timer->remainingSeconds()));
}

void PomodoroWidget::refreshButtons()
{
    bool stopped = (m_timer->state() == PomodoroTimer::State::Stopped);
    m_modeGroup->setEnabled(stopped);

    switch (m_timer->state()) {
    case PomodoroTimer::State::Running: m_btnStart->setText(loc("Pause"));  break;
    case PomodoroTimer::State::Paused:  m_btnStart->setText(loc("Resume")); break;
    case PomodoroTimer::State::Stopped: m_btnStart->setText(loc("Start"));  break;
    }
}

QString PomodoroWidget::fmt(int secs)
{
    return QStringLiteral("%1:%2")
        .arg(secs / 60, 2, 10, QChar('0'))
        .arg(secs % 60, 2, 10, QChar('0'));
}
