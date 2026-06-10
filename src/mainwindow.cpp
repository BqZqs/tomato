#include "mainwindow.h"
#include "pomodoro.h"

#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_timer(new PomodoroTimer(this))
{
    setupUi();

    connect(m_timer, &PomodoroTimer::tick,            this, &MainWindow::onTick);
    connect(m_timer, &PomodoroTimer::finished,        this, &MainWindow::onFinished);
    connect(m_timer, &PomodoroTimer::stateChanged,    this, &MainWindow::onStateChanged);
    connect(m_timer, &PomodoroTimer::modeChanged,     this, &MainWindow::onModeChanged);
    connect(m_timer, &PomodoroTimer::sessionCompleted, this, &MainWindow::onSessionCompleted);

    refreshDisplay();
    refreshButtons();
}

// ---------------------------------------------------------------------------
void MainWindow::setupUi()
{
    setWindowTitle("Pomodoro");
    setFixedSize(320, 360);

    auto *cw = new QWidget(this);
    setCentralWidget(cw);
    auto *lay = new QVBoxLayout(cw);
    lay->setSpacing(12);
    lay->setContentsMargins(24, 16, 24, 16);

    // Timer display
    m_display = new QLabel("25:00");
    m_display->setAlignment(Qt::AlignCenter);
    m_display->setStyleSheet(
        "font-size:56px;font-weight:bold;font-family:Consolas;"
        "color:#2c3e50;background:#ecf0f1;border-radius:10px;padding:16px");
    lay->addWidget(m_display);

    // Status
    m_status = new QLabel("Ready");
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setStyleSheet("font-size:13px;color:#7f8c8d");
    lay->addWidget(m_status);

    // Mode
    m_modeGroup = new QGroupBox("Mode");
    auto *ml = new QVBoxLayout(m_modeGroup);
    m_rbWork  = new QRadioButton("Work (25 min)");
    m_rbShort = new QRadioButton("Short Break (5 min)");
    m_rbLong  = new QRadioButton("Long Break (15 min)");
    m_rbWork->setChecked(true);
    ml->addWidget(m_rbWork);
    ml->addWidget(m_rbShort);
    ml->addWidget(m_rbLong);
    lay->addWidget(m_modeGroup);

    connect(m_rbWork,  &QRadioButton::clicked, this, &MainWindow::onModeRadio);
    connect(m_rbShort, &QRadioButton::clicked, this, &MainWindow::onModeRadio);
    connect(m_rbLong,  &QRadioButton::clicked, this, &MainWindow::onModeRadio);

    // Buttons
    auto *bl = new QHBoxLayout;
    m_btnStart = new QPushButton("Start");
    m_btnReset = new QPushButton("Reset");
    m_btnStart->setMinimumHeight(36);
    m_btnReset->setMinimumHeight(36);
    bl->addWidget(m_btnStart);
    bl->addWidget(m_btnReset);
    lay->addLayout(bl);

    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartPause);
    connect(m_btnReset, &QPushButton::clicked, this, &MainWindow::onReset);

    // Sessions
    m_sessions = new QLabel("Sessions: 0");
    m_sessions->setAlignment(Qt::AlignCenter);
    m_sessions->setStyleSheet("font-size:12px;color:#95a5a6");
    lay->addWidget(m_sessions);

    lay->addStretch();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void MainWindow::onTick(int)              { refreshDisplay(); }
void MainWindow::onSessionCompleted(int n) { m_sessions->setText(QString("Sessions: %1").arg(n)); }

void MainWindow::onFinished()
{
    m_display->setStyleSheet(
        "font-size:56px;font-weight:bold;font-family:Consolas;"
        "color:white;background:#e74c3c;border-radius:10px;padding:16px");
    m_status->setText("Done!");
    m_status->setStyleSheet("font-size:13px;color:#e74c3c;font-weight:bold");
    QApplication::beep();
    refreshButtons();
}

void MainWindow::onStateChanged(PomodoroTimer::State s)
{
    refreshButtons();
    refreshDisplay();

    if (s == PomodoroTimer::State::Running) {
        m_status->setText("Running...");
        m_status->setStyleSheet("font-size:13px;color:#27ae60");
        m_display->setStyleSheet(
            "font-size:56px;font-weight:bold;font-family:Consolas;"
            "color:#2c3e50;background:#ecf0f1;border-radius:10px;padding:16px");
    } else if (s == PomodoroTimer::State::Paused) {
        m_status->setText("Paused");
        m_status->setStyleSheet("font-size:13px;color:#f39c12");
    } else if (m_timer->remainingSeconds() > 0 || m_timer->totalSeconds() == 0) {
        m_status->setText("Ready");
        m_status->setStyleSheet("font-size:13px;color:#7f8c8d");
        m_display->setStyleSheet(
            "font-size:56px;font-weight:bold;font-family:Consolas;"
            "color:#2c3e50;background:#ecf0f1;border-radius:10px;padding:16px");
    }
}

void MainWindow::onModeChanged(PomodoroTimer::Mode m)
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

void MainWindow::onStartPause()
{
    switch (m_timer->state()) {
    case PomodoroTimer::State::Stopped:
    case PomodoroTimer::State::Paused:  m_timer->start(); break;
    case PomodoroTimer::State::Running: m_timer->pause(); break;
    }
}

void MainWindow::onReset() { m_timer->reset(); }

void MainWindow::onModeRadio()
{
    PomodoroTimer::Mode m = PomodoroTimer::Mode::Work;
    if (m_rbShort->isChecked()) m = PomodoroTimer::Mode::ShortBreak;
    else if (m_rbLong->isChecked()) m = PomodoroTimer::Mode::LongBreak;
    m_timer->setMode(m);
}

// ---------------------------------------------------------------------------
void MainWindow::refreshDisplay()
{
    m_display->setText(fmt(m_timer->remainingSeconds()));
}

void MainWindow::refreshButtons()
{
    bool stopped = (m_timer->state() == PomodoroTimer::State::Stopped);
    m_modeGroup->setEnabled(stopped);

    switch (m_timer->state()) {
    case PomodoroTimer::State::Running: m_btnStart->setText("Pause");  break;
    case PomodoroTimer::State::Paused:  m_btnStart->setText("Resume"); break;
    case PomodoroTimer::State::Stopped: m_btnStart->setText("Start");  break;
    }
}

QString MainWindow::fmt(int secs)
{
    return QString("%1:%2")
        .arg(secs / 60, 2, 10, QChar('0'))
        .arg(secs % 60, 2, 10, QChar('0'));
}
