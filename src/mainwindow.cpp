#include "mainwindow.h"
#include "lang.h"
#include "notedata.h"
#include "notewidget.h"
#include "pomodoro_widget.h"
#include "taskdata.h"
#include "taskwidget.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_taskData(new TaskData(dataDirPath()))
    , m_noteData(new NoteData(dataDirPath()))
{
    QDir().mkpath(dataDirPath() + QStringLiteral("/tasks"));
    QDir().mkpath(dataDirPath() + QStringLiteral("/notes"));

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

    auto *cw = new QWidget(this);
    setCentralWidget(cw);
    auto *mainLayout = new QVBoxLayout(cw);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ── Top bar ──────────────────────────────────────────────────────────
    auto *topBar = new QWidget;
    topBar->setStyleSheet(
        QStringLiteral("background:#ecf0f1; border-bottom:1px solid #bdc3c7;"));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 8, 12, 8);
    topLayout->setSpacing(8);

    m_titleLabel = new QLabel(loc("Pomodoro"));
    m_titleLabel->setStyleSheet(
        QStringLiteral("font-size:15px; font-weight:bold; color:#2c3e50; border:none;"));
    topLayout->addWidget(m_titleLabel);
    topLayout->addStretch();

    // Language combo (same as original, but in top bar instead of nav bar)
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
    if (idx >= 0)
        m_langCombo->setCurrentIndex(idx);

    topLayout->addWidget(m_langCombo);
    mainLayout->addWidget(topBar);

    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLanguageChanged);

    // ── Splitter layout ──────────────────────────────────────────────────
    // Left: vertical splitter (Pomodoro top, Tasks bottom)
    // Right: Notes (full height)
    m_leftSplitter = new QSplitter(Qt::Vertical);
    m_leftSplitter->setHandleWidth(3);

    m_pomodoroWidget = new PomodoroWidget;
    m_leftSplitter->addWidget(m_pomodoroWidget);

    m_taskWidget = new TaskWidget(m_taskData);
    m_leftSplitter->addWidget(m_taskWidget);

    m_leftSplitter->setStretchFactor(0, 1);
    m_leftSplitter->setStretchFactor(1, 1);

    m_noteWidget = new NoteWidget(m_noteData);

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setHandleWidth(3);
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_noteWidget);

    // Initial ratio: left 40%, right 60% of 1280px
    m_mainSplitter->setSizes({512, 768});
    m_mainSplitter->setStretchFactor(0, 2);
    m_mainSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(m_mainSplitter, 1);
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
