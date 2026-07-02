#include "taskwidget.h"
#include "taskdata.h"
#include "lang.h"

#include <QDateEdit>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QUuid>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Style helpers – match mainwindow.cpp palette
// ---------------------------------------------------------------------------
static const char *kBtnStyle =
    "QPushButton {"
    "  color:#2c3e50;"
    "  background:#ecf0f1;"
    "  border:1px solid #bdc3c7;"
    "  border-radius:4px;"
    "  padding:6px 12px;"
    "  min-height:32px;"
    "}"
    "QPushButton:hover   { background:#d5dbdb; }"
    "QPushButton:pressed { background:#bdc3c7; }"
    "QPushButton:disabled { color:#95a5a6; background:#ecf0f1; }";

static const char *kDateEditStyle =
    "QDateEdit {"
    "  font-size:14px;"
    "  font-weight:bold;"
    "  color:#2c3e50;"
    "  background:#ecf0f1;"
    "  border:1px solid #bdc3c7;"
    "  border-radius:4px;"
    "  padding:4px 8px;"
    "}";

static const char *kListStyle =
    "QListWidget {"
    "  color:#2c3e50;"
    "  background:#ecf0f1;"
    "  border:1px solid #bdc3c7;"
    "  border-radius:4px;"
    "  padding:4px;"
    "}"
    "QListWidget::item { padding:4px 0px; }"
    "QListWidget::item:selected { background:#d5dbdb; }";

static const char *kStatusStyle =
    "font-size:12px;color:#7f8c8d";

static const char *kNoListLabelStyle =
    "font-size:13px;color:#7f8c8d;padding:12px";

static const char *kNavBtnStyle =
    "QPushButton {"
    "  font-size:14px;"
    "  font-weight:bold;"
    "  color:#2c3e50;"
    "  background:#ecf0f1;"
    "  border:1px solid #bdc3c7;"
    "  border-radius:4px;"
    "  padding:4px 10px;"
    "  min-height:28px;"
    "  min-width:36px;"
    "}"
    "QPushButton:hover   { background:#d5dbdb; }"
    "QPushButton:pressed { background:#bdc3c7; }"
    "QPushButton:disabled { color:#95a5a6; background:#ecf0f1; }";

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
TaskWidget::TaskWidget(TaskData *data, QWidget *parent)
    : QWidget(parent), m_data(data), m_currentDate(QDate::currentDate())
{
    buildUi();

    m_taskFontSize = LocaleManager::instance()->taskFontSize();
    applyFontSize();

    refreshAll();

    connect(LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &TaskWidget::refreshTexts);
    connect(LocaleManager::instance(), &LocaleManager::taskFontSizeChanged,
            this, &TaskWidget::setTaskFontSize);
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------
void TaskWidget::buildUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(24, 16, 24, 16);

    // --- Date navigation bar (always visible) ---
    mainLayout->addLayout(buildDateNav());

    // --- Status label (always visible) ---
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(kStatusStyle);
    mainLayout->addWidget(m_statusLabel);

    // --- Stacked widget for list / create views ---
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(buildTaskListView());   // index 0
    m_stack->addWidget(buildCreateView());      // index 1
    mainLayout->addWidget(m_stack, /*stretch=*/1);

    // --- Font size control ---
    {
        auto *fontRow = new QHBoxLayout;
        fontRow->setSpacing(8);
        m_fontLabel = new QLabel(loc("Font Size:"), this);
        m_fontLabel->setStyleSheet(QStringLiteral("font-size:12px; color:#7f8c8d;"));
        m_fontSpinBox = new QSpinBox(this);
        m_fontSpinBox->setRange(8, 48);
        m_fontSpinBox->setValue(m_taskFontSize);
        m_fontSpinBox->setSuffix(QStringLiteral(" px"));
        m_fontSpinBox->setStyleSheet(QStringLiteral("font-size:12px; padding:2px 4px;"));
        m_fontSpinBox->setToolTip(loc("Adjust task list font size"));
        fontRow->addWidget(m_fontLabel);
        fontRow->addWidget(m_fontSpinBox);
        fontRow->addStretch();
        mainLayout->addLayout(fontRow);

        connect(m_fontSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int size) {
            LocaleManager::instance()->setTaskFontSize(size);
        });
    }

    mainLayout->addStretch();
}

// ---------------------------------------------------------------------------
// Date navigation bar
// ---------------------------------------------------------------------------
QHBoxLayout *TaskWidget::buildDateNav()
{
    auto *row = new QHBoxLayout;
    row->setSpacing(8);

    m_btnPrev = new QPushButton(QStringLiteral("\u25C0"), this); // ◀
    m_btnPrev->setStyleSheet(kNavBtnStyle);
    m_btnPrev->setToolTip(loc("Previous day"));
    row->addWidget(m_btnPrev);

    m_dateEdit = new QDateEdit(m_currentDate, this);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setAlignment(Qt::AlignCenter);
    m_dateEdit->setStyleSheet(kDateEditStyle);
    row->addWidget(m_dateEdit, /*stretch=*/1);

    m_btnNext = new QPushButton(QStringLiteral("\u25B6"), this); // ▶
    m_btnNext->setStyleSheet(kNavBtnStyle);
    m_btnNext->setToolTip(loc("Next day"));
    row->addWidget(m_btnNext);

    m_btnToday = new QPushButton(loc("Today"), this);
    m_btnToday->setStyleSheet(kNavBtnStyle);
    m_btnToday->setToolTip(loc("Go to today"));
    row->addWidget(m_btnToday);

    // Connections
    connect(m_btnPrev,  &QPushButton::clicked, this, &TaskWidget::onPrevDay);
    connect(m_btnNext,  &QPushButton::clicked, this, &TaskWidget::onNextDay);
    connect(m_btnToday, &QPushButton::clicked, this, &TaskWidget::onToday);
    connect(m_dateEdit, &QDateEdit::dateChanged, this,
            &TaskWidget::onDateEditChanged);

    return row;
}

// ---------------------------------------------------------------------------
// Task list view (page 0)
// ---------------------------------------------------------------------------
QWidget *TaskWidget::buildTaskListView()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setSpacing(8);
    lay->setContentsMargins(0, 0, 0, 0);

    // --- Task list ---
    m_taskList = new QListWidget(page);
    m_taskList->setStyleSheet(kListStyle);
    m_taskList->setAlternatingRowColors(false);
    m_taskList->setSelectionMode(QAbstractItemView::SingleSelection);
    lay->addWidget(m_taskList, /*stretch=*/1);

    connect(m_taskList, &QListWidget::itemChanged,
            this, &TaskWidget::onItemCheckChanged);

    // --- Action buttons row 1 ---
    auto *btnRow1 = new QHBoxLayout;
    btnRow1->setSpacing(6);

    m_btnAdd = new QPushButton(loc("+ Add"), page);
    m_btnAdd->setStyleSheet(kBtnStyle);
    btnRow1->addWidget(m_btnAdd);

    m_btnEdit = new QPushButton(loc("\u270E Edit"), page); // ✎
    m_btnEdit->setStyleSheet(kBtnStyle);
    btnRow1->addWidget(m_btnEdit);

    m_btnDelete = new QPushButton(loc("\U0001F5D1 Delete"), page); // 🗑
    m_btnDelete->setStyleSheet(kBtnStyle);
    btnRow1->addWidget(m_btnDelete);

    lay->addLayout(btnRow1);

    // --- Action buttons row 2 ---
    auto *btnRow2 = new QHBoxLayout;
    btnRow2->setSpacing(6);

    m_btnToggle = new QPushButton(loc("\u2713 Toggle"), page); // ✓
    m_btnToggle->setStyleSheet(kBtnStyle);
    btnRow2->addWidget(m_btnToggle);

    m_btnCleanup = new QPushButton(loc("\U0001F9F9 Cleanup"), page); // 🧹
    m_btnCleanup->setStyleSheet(kBtnStyle);
    btnRow2->addWidget(m_btnCleanup);

    lay->addLayout(btnRow2);

    // Connections
    connect(m_btnAdd,     &QPushButton::clicked, this, &TaskWidget::onAddTask);
    connect(m_btnEdit,    &QPushButton::clicked, this, &TaskWidget::onEditTask);
    connect(m_btnDelete,  &QPushButton::clicked, this, &TaskWidget::onDeleteTask);
    connect(m_btnToggle,  &QPushButton::clicked, this, &TaskWidget::onToggleTask);
    connect(m_btnCleanup, &QPushButton::clicked, this, &TaskWidget::onCleanup);

    return page;
}

// ---------------------------------------------------------------------------
// Create view (page 1)
// ---------------------------------------------------------------------------
QWidget *TaskWidget::buildCreateView()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setSpacing(10);
    lay->setContentsMargins(0, 8, 0, 0);

    // Info message
    auto *msg = new QLabel(loc("No task list for this date."), page);
    msg->setAlignment(Qt::AlignCenter);
    msg->setStyleSheet(kNoListLabelStyle);
    lay->addWidget(msg);

    lay->addStretch();

    // Create buttons
    m_btnCreateEmpty = new QPushButton(loc("Create Empty List"), page);
    m_btnCreateEmpty->setStyleSheet(kBtnStyle);
    lay->addWidget(m_btnCreateEmpty);

    m_btnInheritAll = new QPushButton(loc("Inherit All from..."), page);
    m_btnInheritAll->setStyleSheet(kBtnStyle);
    lay->addWidget(m_btnInheritAll);

    m_btnInheritIncomplete = new QPushButton(loc("Inherit Incomplete from..."), page);
    m_btnInheritIncomplete->setStyleSheet(kBtnStyle);
    lay->addWidget(m_btnInheritIncomplete);

    lay->addStretch();

    connect(m_btnCreateEmpty,       &QPushButton::clicked,
            this, &TaskWidget::onCreateEmpty);
    connect(m_btnInheritAll,        &QPushButton::clicked,
            this, &TaskWidget::onInheritAll);
    connect(m_btnInheritIncomplete,  &QPushButton::clicked,
            this, &TaskWidget::onInheritIncomplete);

    return page;
}

// ---------------------------------------------------------------------------
// Date navigation helpers
// ---------------------------------------------------------------------------
QPair<QDate, QDate> TaskWidget::navWindow() const
{
    return m_data->validWindow();
}

bool TaskWidget::isAtWindowStart() const
{
    return m_currentDate <= navWindow().first;
}

bool TaskWidget::isAtWindowEnd() const
{
    return m_currentDate >= navWindow().second;
}

// ---------------------------------------------------------------------------
// Refresh – top-level entry point
// ---------------------------------------------------------------------------
void TaskWidget::refreshAll()
{
    refreshDateNav();

    bool hasTasks = m_data->hasTaskFile(m_currentDate);

    if (hasTasks) {
        m_stack->setCurrentIndex(0);
        populateTaskList();
        updateStatus();
    } else {
        m_stack->setCurrentIndex(1);
        m_statusLabel->setText(loc("No task list for this date."));
    }
}

// ---------------------------------------------------------------------------
// Translation refresh – re-sets all user-visible texts
// ---------------------------------------------------------------------------
void TaskWidget::refreshTexts()
{
    // Date navigation
    m_btnToday->setText(loc("Today"));
    m_btnPrev->setToolTip(loc("Previous day"));
    m_btnNext->setToolTip(loc("Next day"));
    m_btnToday->setToolTip(loc("Go to today"));

    // Action buttons
    m_btnAdd->setText(loc("+ Add"));
    m_btnEdit->setText(loc("\u270E Edit"));
    m_btnDelete->setText(loc("\U0001F5D1 Delete"));
    m_btnToggle->setText(loc("\u2713 Toggle"));
    m_btnCleanup->setText(loc("\U0001F9F9 Cleanup"));

    // Font size control
    m_fontLabel->setText(loc("Font Size:"));
    m_fontSpinBox->setToolTip(loc("Adjust task list font size"));

    // Create view buttons
    m_btnCreateEmpty->setText(loc("Create Empty List"));
    m_btnInheritAll->setText(loc("Inherit All from..."));
    m_btnInheritIncomplete->setText(loc("Inherit Incomplete from..."));

    // Refresh status label and list
    refreshAll();
}

// ---------------------------------------------------------------------------
// Date navigation bar refresh
// ---------------------------------------------------------------------------
void TaskWidget::refreshDateNav()
{
    // Block signals so we don't trigger onDateEditChanged during programmatic set
    m_dateEdit->blockSignals(true);

    auto [winStart, winEnd] = navWindow();
    m_dateEdit->setDateRange(winStart, winEnd);
    m_dateEdit->setDate(m_currentDate);

    m_dateEdit->blockSignals(false);

    m_btnPrev->setEnabled(!isAtWindowStart());
    m_btnNext->setEnabled(!isAtWindowEnd());
}

// ---------------------------------------------------------------------------
// Populate task list from data layer
// ---------------------------------------------------------------------------
void TaskWidget::populateTaskList()
{
    // Block signals while rebuilding to avoid spurious itemChanged emissions
    m_taskList->blockSignals(true);
    m_taskList->clear();

    const auto tasks = m_data->loadTasks(m_currentDate);
    for (const auto &t : tasks) {
        auto *item = new QListWidgetItem(t.text, m_taskList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(t.completed ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, t.id);
        // order is implicit via list position (already sorted by TaskData)
    }

    m_taskList->blockSignals(false);
}

// ---------------------------------------------------------------------------
// Save current list widget contents back to data layer
// ---------------------------------------------------------------------------
void TaskWidget::saveCurrentList()
{
    QList<TaskItem> tasks;
    tasks.reserve(m_taskList->count());

    for (int i = 0; i < m_taskList->count(); ++i) {
        auto *item = m_taskList->item(i);
        TaskItem t;
        t.id        = item->data(Qt::UserRole).toString();
        t.text      = item->text();
        t.completed = (item->checkState() == Qt::Checked);
        t.order     = i;
        tasks.append(t);
    }

    m_data->saveTasks(m_currentDate, tasks);
    updateStatus();
}

// ---------------------------------------------------------------------------
// Update status label
// ---------------------------------------------------------------------------
void TaskWidget::updateStatus()
{
    if (!m_data->hasTaskFile(m_currentDate)) {
        m_statusLabel->setText(loc("No task list for this date."));
        return;
    }

    int total = m_taskList->count();
    int done  = 0;
    for (int i = 0; i < total; ++i) {
        if (m_taskList->item(i)->checkState() == Qt::Checked)
            ++done;
    }

        m_statusLabel->setText(
            loc("%1 tasks, %2 done").arg(total).arg(done));
}

// ---------------------------------------------------------------------------
// Font size
// ---------------------------------------------------------------------------
void TaskWidget::applyFontSize()
{
    const QString btnStyle = QStringLiteral("font-size:%1px;").arg(m_taskFontSize)
                             + QString::fromLatin1(kBtnStyle);
    const QString listStyle = QStringLiteral("font-size:%1px;").arg(m_taskFontSize)
                              + QString::fromLatin1(kListStyle);

    m_taskList->setStyleSheet(listStyle);

    m_btnAdd->setStyleSheet(btnStyle);
    m_btnEdit->setStyleSheet(btnStyle);
    m_btnDelete->setStyleSheet(btnStyle);
    m_btnToggle->setStyleSheet(btnStyle);
    m_btnCleanup->setStyleSheet(btnStyle);
    m_btnCreateEmpty->setStyleSheet(btnStyle);
    m_btnInheritAll->setStyleSheet(btnStyle);
    m_btnInheritIncomplete->setStyleSheet(btnStyle);
}

void TaskWidget::setTaskFontSize(int size)
{
    if (size == m_taskFontSize)
        return;
    m_taskFontSize = size;
    applyFontSize();
    if (m_fontSpinBox) {
        m_fontSpinBox->blockSignals(true);
        m_fontSpinBox->setValue(size);
        m_fontSpinBox->blockSignals(false);
    }
}

// ---------------------------------------------------------------------------
// Date navigation slots
// ---------------------------------------------------------------------------
void TaskWidget::onPrevDay()
{
    if (isAtWindowStart())
        return;
    m_currentDate = m_currentDate.addDays(-1);
    refreshAll();
}

void TaskWidget::onNextDay()
{
    if (isAtWindowEnd())
        return;
    m_currentDate = m_currentDate.addDays(1);
    refreshAll();
}

void TaskWidget::onToday()
{
    m_currentDate = QDate::currentDate();
    refreshAll();
}

void TaskWidget::onDateEditChanged(const QDate &date)
{
    if (date == m_currentDate)
        return;
    m_currentDate = date;
    refreshAll();
}

// ---------------------------------------------------------------------------
// Task list action slots
// ---------------------------------------------------------------------------
void TaskWidget::onAddTask()
{
    bool ok = false;
    QString text = QInputDialog::getText(
        this, loc("Add Task"), loc("Task Text:"),
        QLineEdit::Normal, QString(), &ok);

    if (!ok || text.trimmed().isEmpty())
        return;

    // Generate new ID
    QString newId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Build new item
    auto *item = new QListWidgetItem(text.trimmed(), m_taskList);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);
    item->setData(Qt::UserRole, newId);

    saveCurrentList();
}

void TaskWidget::onEditTask()
{
    auto *item = m_taskList->currentItem();
    if (!item)
        return;

    bool ok = false;
    QString newText = QInputDialog::getText(
        this, loc("Edit Task"), loc("Task Text:"),
        QLineEdit::Normal, item->text(), &ok);

    if (!ok || newText.trimmed().isEmpty())
        return;

    item->setText(newText.trimmed());
    saveCurrentList();
}

void TaskWidget::onDeleteTask()
{
    auto *item = m_taskList->currentItem();
    if (!item)
        return;

    int row = m_taskList->row(item);
    delete m_taskList->takeItem(row);
    saveCurrentList();
}

void TaskWidget::onToggleTask()
{
    auto *item = m_taskList->currentItem();
    if (!item)
        return;

    Qt::CheckState newState = (item->checkState() == Qt::Checked)
                                  ? Qt::Unchecked
                                  : Qt::Checked;
    item->setCheckState(newState);
    saveCurrentList();
}

void TaskWidget::onCleanup()
{
    int count = m_data->cleanupOutsideWindow();
    QMessageBox::information(
        this, loc("Cleanup Complete"),
        loc("Deleted %1 file(s) outside the 15-day window.").arg(count));

    // Re-check current date — if the file was among those cleaned, switch view
    refreshAll();
}

void TaskWidget::onItemCheckChanged(QListWidgetItem * /*item*/)
{
    // Only save if we are currently in list view (avoid spurious saves
    // during populateTaskList which has signals blocked anyway)
    if (m_stack->currentIndex() == 0 && m_taskList->signalsBlocked() == false)
        saveCurrentList();
}

// ---------------------------------------------------------------------------
// Create action slots
// ---------------------------------------------------------------------------
void TaskWidget::onCreateEmpty()
{
    m_data->createEmptyList(m_currentDate);
    refreshAll();
}

void TaskWidget::onInheritAll()
{
    QList<QDate> dates = m_data->existingDates();
    // Exclude current date from source options
    QStringList dateStrings;
    for (const auto &d : dates) {
        if (d != m_currentDate)
            dateStrings.append(d.toString("yyyy-MM-dd"));
    }

    if (dateStrings.isEmpty()) {
        QMessageBox::information(
            this, loc("Inherit All"),
            loc("No other dates available to inherit from."));
        return;
    }

    bool ok = false;
    QString chosen = QInputDialog::getItem(
        this, loc("Inherit All"),
        loc("Select date to inherit from:"),
        dateStrings, 0, false, &ok);

    if (!ok || chosen.isEmpty())
        return;

    QDate source = QDate::fromString(chosen, "yyyy-MM-dd");
    if (!source.isValid())
        return;

    m_data->inheritAll(source, m_currentDate);
    refreshAll();
}

void TaskWidget::onInheritIncomplete()
{
    QList<QDate> dates = m_data->existingDates();
    QStringList dateStrings;
    for (const auto &d : dates) {
        if (d != m_currentDate)
            dateStrings.append(d.toString("yyyy-MM-dd"));
    }

    if (dateStrings.isEmpty()) {
        QMessageBox::information(
            this, loc("Inherit Incomplete"),
            loc("No other dates available to inherit from."));
        return;
    }

    bool ok = false;
    QString chosen = QInputDialog::getItem(
        this, loc("Inherit Incomplete"),
        loc("Select date to inherit from:"),
        dateStrings, 0, false, &ok);

    if (!ok || chosen.isEmpty())
        return;

    QDate source = QDate::fromString(chosen, "yyyy-MM-dd");
    if (!source.isValid())
        return;

    m_data->inheritIncomplete(source, m_currentDate);
    refreshAll();
}
