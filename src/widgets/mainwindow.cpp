#include "mainwindow.h"
#include "daily_task_data.h"
#include "daily_task_widget.h"
#include "lang.h"
#include "notedata.h"
#include "notewidget.h"
#include "pomodoro_widget.h"
#include "taskdata.h"
#include "taskwidget.h"
#include "theme.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_taskData(new TaskData(dataDirPath()))
    , m_noteData(new NoteData(dataDirPath()))
    , m_dailyData(new DailyTaskData(dataDirPath()))
{
    QDir().mkpath(dataDirPath() + QStringLiteral("/tasks"));
    QDir().mkpath(dataDirPath() + QStringLiteral("/notes"));
    QDir().mkpath(dataDirPath() + QStringLiteral("/daily"));

    setupUi();

    connect(LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &MainWindow::refreshTexts);
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
    resize(1280, 720);
    setMinimumSize(800, 500);

    // ── Global window style ────────────────────────────────────────────
    setStyleSheet(QStringLiteral(
        "QMainWindow { background-color:#F0F2F5; }"
        "QWidget { font-family:'Segoe UI','SF Pro Display',-apple-system,sans-serif; }"));

    auto *cw = new QWidget(this);
    cw->setObjectName(QStringLiteral("centralContainer"));
    cw->setStyleSheet(QStringLiteral("#centralContainer { background:#F0F2F5; }"));
    setCentralWidget(cw);
    auto *mainLayout = new QVBoxLayout(cw);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ── Top bar ──────────────────────────────────────────────────────────
    auto *topBar = new QWidget;
    topBar->setFixedHeight(42);
    topBar->setStyleSheet(Theme::kTopBarStyleSheet);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 0, 12, 0);
    topLayout->setSpacing(8);

    m_titleLabel = new QLabel(loc("Pomodoro"));
    m_titleLabel->setStyleSheet(QStringLiteral(
        "font-size:15px; font-weight:bold; color:#1A1A2E; border:none;"));
    topLayout->addWidget(m_titleLabel);
    topLayout->addStretch();

    auto *cleanupBtn = new QPushButton(QStringLiteral("\U0001F9F9"));
    cleanupBtn->setFixedSize(28, 28);
    cleanupBtn->setFlat(true);
    cleanupBtn->setToolTip(loc("Cleanup"));
    cleanupBtn->setStyleSheet(QStringLiteral(
        "QPushButton { font-size:14px; border:none; color:#7B7D8C; }"
        "QPushButton:hover { color:#6C63FF; }"));
    connect(cleanupBtn, &QPushButton::clicked, this, [this]() {
        m_taskWidget->triggerCleanup();
    });
    topLayout->addWidget(cleanupBtn);

    m_fontLabel = new QLabel(loc("Font Size:"));
    m_fontLabel->setStyleSheet(QStringLiteral(
        "font-size:12px; color:#7B7D8C; border:none;"));
    topLayout->addWidget(m_fontLabel);

    m_fontCombo = new QComboBox;
    m_fontCombo->setEditable(true);
    m_fontCombo->setInsertPolicy(QComboBox::NoInsert);
    m_fontCombo->setValidator(new QIntValidator(-4, 16, m_fontCombo));
    m_fontCombo->addItems({QStringLiteral("-4"), QStringLiteral("-2"),
                           QStringLiteral("-1"), QStringLiteral("0"),
                           QStringLiteral("+1"), QStringLiteral("+2"),
                           QStringLiteral("+4"), QStringLiteral("+8"),
                           QStringLiteral("+12"), QStringLiteral("+16")});
    m_fontCombo->setCurrentText(
        QString::number(LocaleManager::instance()->fontOffset()));
    m_fontCombo->setMaximumWidth(72);
    m_fontCombo->setStyleSheet(QStringLiteral(
        "QComboBox { font-size:12px; padding:2px 4px; "
        "border:1px solid #D1D5DB; border-radius:6px; "
        "color:#7B7D8C; background:#F8F9FB; }"
        "QComboBox:hover { border-color:#9CA3AF; }"
        "QComboBox:focus { border-color:#6C63FF; }"
        "QComboBox::drop-down { width:20px; border:none; }"
        "QComboBox::down-arrow { image:none; border-left:4px solid transparent;"
        " border-right:4px solid transparent; border-top:5px solid #7B7D8C;"
        " margin-right:4px; }"));
    connect(m_fontCombo, &QComboBox::currentTextChanged, this, [](const QString &text) {
        bool ok;
        int val = text.toInt(&ok);
        if (ok)
            LocaleManager::instance()->setFontOffset(val);
    });
    topLayout->addWidget(m_fontCombo);

    m_langCombo = new QComboBox;
    m_langCombo->setMinimumHeight(28);
    m_langCombo->setMaximumWidth(90);
    m_langCombo->setStyleSheet(QStringLiteral(
        "QComboBox { font-size:12px; padding:2px 6px; "
        "border:1px solid #D1D5DB; border-radius:6px; "
        "color:#7B7D8C; background:#F8F9FB; }"
        "QComboBox:hover { border-color:#9CA3AF; }"
        "QComboBox::drop-down { width:20px; border:none; }"
        "QComboBox::down-arrow { image:none; border-left:4px solid transparent;"
        " border-right:4px solid transparent; border-top:5px solid #7B7D8C;"
        " margin-right:4px; }"));

    LocaleManager *loc = LocaleManager::instance();
    const QStringList langs = loc->languages();
    for (const QString &lang : langs)
        m_langCombo->addItem(loc->langName(lang), lang);
    int idx = m_langCombo->findData(loc->currentLang());
    if (idx >= 0)
        m_langCombo->setCurrentIndex(idx);
    topLayout->addWidget(m_langCombo);
    mainLayout->addWidget(topBar);
    mainLayout->addSpacing(10);

    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);

    // ── 4-quadrant layout ────────────────────────────────────────────
    auto wrapPanel = [](QWidget *inner) -> QFrame * {
        auto *frame = new QFrame;
        frame->setFrameStyle(QFrame::NoFrame);
        frame->setStyleSheet(Theme::kPanelStyleSheet);
        auto *lay = new QVBoxLayout(frame);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->addWidget(inner);

        auto *shadow = new QGraphicsDropShadowEffect(frame);
        shadow->setBlurRadius(16);
        shadow->setColor(QColor(0, 0, 0, 18));
        shadow->setOffset(0, 2);
        frame->setGraphicsEffect(shadow);
        return frame;
    };

    m_leftSplitter = new QSplitter(Qt::Vertical);
    m_leftSplitter->setHandleWidth(10);
    m_leftSplitter->setChildrenCollapsible(false);
    m_leftSplitter->setStyleSheet(Theme::kSplitterStyleSheet);

    m_pomodoroWidget = new PomodoroWidget;
    m_leftSplitter->addWidget(wrapPanel(m_pomodoroWidget));

    m_dailyWidget = new DailyTaskWidget(m_dailyData);
    m_leftSplitter->addWidget(wrapPanel(m_dailyWidget));

    m_leftSplitter->setSizes({200, 520});
    m_leftSplitter->setStretchFactor(0, 1);
    m_leftSplitter->setStretchFactor(1, 2);

    m_rightSplitter = new QSplitter(Qt::Vertical);
    m_rightSplitter->setHandleWidth(10);
    m_rightSplitter->setChildrenCollapsible(false);
    m_rightSplitter->setStyleSheet(Theme::kSplitterStyleSheet);

    m_taskWidget = new TaskWidget(m_taskData);
    m_rightSplitter->addWidget(wrapPanel(m_taskWidget));

    m_noteWidget = new NoteWidget(m_noteData);
    m_rightSplitter->addWidget(wrapPanel(m_noteWidget));

    m_rightSplitter->setSizes({260, 460});
    m_rightSplitter->setStretchFactor(0, 1);
    m_rightSplitter->setStretchFactor(1, 2);

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setHandleWidth(10);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->setStyleSheet(Theme::kSplitterStyleSheet);
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_rightSplitter);

    m_mainSplitter->setSizes({640, 640});
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_mainSplitter, 1);

    // Signal bridges: DailyTaskWidget ↔ PomodoroWidget
    connect(m_pomodoroWidget, &PomodoroWidget::timedSessionFinished,
            m_dailyWidget, &DailyTaskWidget::onTimedSessionFinished);
    connect(m_dailyWidget, &DailyTaskWidget::startTimerForTask,
            m_pomodoroWidget, &PomodoroWidget::startTimedSession);
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
    m_titleLabel->setText(loc("Pomodoro"));
    m_fontLabel->setText(loc("Font Size:"));
}

// ── Fullscreen (F11) ──────────────────────────────────────────────────────

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F11) {
        toggleFullscreen();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::toggleFullscreen()
{
    if (m_fullscreen) {
        showNormal();
        m_fullscreen = false;
    } else {
        showFullScreen();
        m_fullscreen = true;
    }
}
