#include "mainwindow.h"
#include "pomodoro.h"
#include "taskwidget.h"
#include "notewidget.h"
#include "lang.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_timer(new PomodoroTimer(this))
    , m_taskData(new TaskData(dataDirPath()))
    , m_noteData(new NoteData(dataDirPath()))
{
    QDir().mkpath(dataDirPath() + QStringLiteral("/tasks"));
    QDir().mkpath(dataDirPath() + QStringLiteral("/notes"));

    setupUi();

    connect(m_timer, &PomodoroTimer::tick,            this, &MainWindow::onTick);
    connect(m_timer, &PomodoroTimer::finished,        this, &MainWindow::onFinished);
    connect(m_timer, &PomodoroTimer::stateChanged,    this, &MainWindow::onStateChanged);
    connect(m_timer, &PomodoroTimer::modeChanged,     this, &MainWindow::onModeChanged);
    connect(m_timer, &PomodoroTimer::sessionCompleted, this, &MainWindow::onSessionCompleted);
    connect(LocaleManager::instance(), &LocaleManager::languageChanged, this, &MainWindow::refreshTexts);

    refreshDisplay();
    refreshButtons();
}

// ---------------------------------------------------------------------------
QString MainWindow::dataDirPath() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/data");
}

// ---------------------------------------------------------------------------
void MainWindow::setupUi()
{
    setWindowTitle(loc("Pomodoro"));
    resize(500, 540);
    setMinimumSize(440, 440);

    auto *cw = new QWidget(this);
    setCentralWidget(cw);
    auto *mainLayout = new QVBoxLayout(cw);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ── Navigation bar ──────────────────────────────────────────────────
    auto *navBar = new QHBoxLayout;
    navBar->setSpacing(0);
    navBar->setContentsMargins(12, 8, 12, 0);

    m_btnTimer = new QPushButton(loc("Timer"));
    m_btnTasks = new QPushButton(loc("Tasks"));
    m_btnNotes = new QPushButton(loc("Notes"));

    for (auto *btn : {m_btnTimer, m_btnTasks, m_btnNotes}) {
        btn->setMinimumHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
    }

    navBar->addWidget(m_btnTimer);
    navBar->addWidget(m_btnTasks);
    navBar->addWidget(m_btnNotes);
    navBar->addStretch();

    setupLanguageCombo(navBar);

    mainLayout->addLayout(navBar);

    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("color:#bdc3c7;"));
    mainLayout->addWidget(sep);

    // ── Stacked pages ───────────────────────────────────────────────────
    m_stack = new QStackedWidget;
    mainLayout->addWidget(m_stack, 1);

    auto *timerPage = new QWidget;
    setupTimerPage(timerPage);
    m_stack->addWidget(timerPage);

    m_taskWidget = new TaskWidget(m_taskData);
    m_stack->addWidget(m_taskWidget);

    m_noteWidget = new NoteWidget(m_noteData);
    m_stack->addWidget(m_noteWidget);

    connect(m_btnTimer, &QPushButton::clicked, this, [this] { onNavClicked(0); });
    connect(m_btnTasks, &QPushButton::clicked, this, [this] { onNavClicked(1); });
    connect(m_btnNotes, &QPushButton::clicked, this, [this] { onNavClicked(2); });

    onNavClicked(0);
}

// ---------------------------------------------------------------------------
void MainWindow::setupLanguageCombo(QHBoxLayout *navBar)
{
    m_langCombo = new QComboBox;
    m_langCombo->setMinimumHeight(28);
    m_langCombo->setMaximumWidth(90);
    m_langCombo->setStyleSheet(
        QStringLiteral("QComboBox { font-size:12px; padding:2px 6px; border:1px solid #bdc3c7; "
                       "border-radius:4px; color:#7f8c8d; background:transparent; }"
                       "QComboBox:hover { border-color:#95a5a6; }"
                       "QComboBox::drop-down { border:none; }"));

    LocaleManager *loc = LocaleManager::instance();
    const QStringList langs = loc->languages();
    for (const QString &lang : langs)
        m_langCombo->addItem(loc->langName(lang), lang);

    int idx = m_langCombo->findData(loc->currentLang());
    if (idx >= 0) m_langCombo->setCurrentIndex(idx);

    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);

    navBar->addWidget(m_langCombo);
}

// ---------------------------------------------------------------------------
void MainWindow::setupTimerPage(QWidget *page)
{
    auto *lay = new QVBoxLayout(page);
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

    connect(m_rbWork,  &QRadioButton::clicked, this, &MainWindow::onModeRadio);
    connect(m_rbShort, &QRadioButton::clicked, this, &MainWindow::onModeRadio);
    connect(m_rbLong,  &QRadioButton::clicked, this, &MainWindow::onModeRadio);

    auto *bl = new QHBoxLayout;
    m_btnStart = new QPushButton(loc("Start"));
    m_btnReset = new QPushButton(loc("Reset"));
    m_btnStart->setMinimumHeight(36);
    m_btnReset->setMinimumHeight(36);
    bl->addWidget(m_btnStart);
    bl->addWidget(m_btnReset);
    lay->addLayout(bl);

    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartPause);
    connect(m_btnReset, &QPushButton::clicked, this, &MainWindow::onReset);

    m_sessions = new QLabel(loc("Sessions: %1").arg(0));
    m_sessions->setAlignment(Qt::AlignCenter);
    m_sessions->setStyleSheet(QStringLiteral("font-size:12px;color:#95a5a6"));
    lay->addWidget(m_sessions);

    lay->addStretch();
}

// ── Language ──────────────────────────────────────────────────────────────

void MainWindow::onLanguageChanged(int)
{
    QString lang = m_langCombo->currentData().toString();
    LocaleManager::instance()->setLanguage(lang);
}

void MainWindow::refreshTexts()
{
    setWindowTitle(loc("Pomodoro"));

    m_btnTimer->setText(loc("Timer"));
    m_btnTasks->setText(loc("Tasks"));
    m_btnNotes->setText(loc("Notes"));

    m_status->setText(loc("Ready"));
    m_modeGroup->setTitle(loc("Mode"));
    m_rbWork->setText(loc("Work (25 min)"));
    m_rbShort->setText(loc("Short Break (5 min)"));
    m_rbLong->setText(loc("Long Break (15 min)"));

    refreshButtons();
    refreshDisplay();
    onSessionCompleted(m_timer->completedSessions());
}

// ── Navigation ────────────────────────────────────────────────────────────

void MainWindow::onNavClicked(int index)
{
    m_stack->setCurrentIndex(index);
    updateNavButtons(index);
}

void MainWindow::updateNavButtons(int active)
{
    const QString activeStyle =
        QStringLiteral("font-size:14px; font-weight:bold; padding:8px 20px; border:none;"
                       "border-bottom:3px solid #27ae60; color:#2c3e50; background:transparent;");
    const QString inactiveStyle =
        QStringLiteral("font-size:14px; font-weight:bold; padding:8px 20px; border:none;"
                        "border-bottom:3px solid transparent; color:#7f8c8d; background:transparent;");

    m_btnTimer->setStyleSheet(active == 0 ? activeStyle : inactiveStyle);
    m_btnTasks->setStyleSheet(active == 1 ? activeStyle : inactiveStyle);
    m_btnNotes->setStyleSheet(active == 2 ? activeStyle : inactiveStyle);
}

// ── Slots ─────────────────────────────────────────────────────────────────

void MainWindow::onTick(int)              { refreshDisplay(); }
void MainWindow::onSessionCompleted(int n)
{
    m_sessions->setText(loc("Sessions: %1").arg(n));
}

void MainWindow::onFinished()
{
    m_display->setStyleSheet(
        QStringLiteral("font-size:56px;font-weight:bold;font-family:Consolas;"
                       "color:white;background:#e74c3c;border-radius:10px;padding:16px"));
    m_status->setText(loc("Done!"));
    m_status->setStyleSheet(QStringLiteral("font-size:13px;color:#e74c3c;font-weight:bold"));
    QApplication::beep();
    refreshButtons();
}

void MainWindow::onStateChanged(PomodoroTimer::State s)
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

// ── Helpers ───────────────────────────────────────────────────────────────

void MainWindow::refreshDisplay()
{
    m_display->setText(fmt(m_timer->remainingSeconds()));
}

void MainWindow::refreshButtons()
{
    bool stopped = (m_timer->state() == PomodoroTimer::State::Stopped);
    m_modeGroup->setEnabled(stopped);

    switch (m_timer->state()) {
    case PomodoroTimer::State::Running: m_btnStart->setText(loc("Pause"));  break;
    case PomodoroTimer::State::Paused:  m_btnStart->setText(loc("Resume")); break;
    case PomodoroTimer::State::Stopped: m_btnStart->setText(loc("Start"));  break;
    }
}

QString MainWindow::fmt(int secs)
{
    return QStringLiteral("%1:%2")
        .arg(secs / 60, 2, 10, QChar('0'))
        .arg(secs % 60, 2, 10, QChar('0'));
}
