#include "taskwidget.h"
#include "taskdata.h"
#include "lang.h"

#include <QCheckBox>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
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
// TaskRow – per-task inline widget row with inline editing
// ---------------------------------------------------------------------------
class TaskRow : public QWidget {
    Q_OBJECT
public:
    TaskRow(const QString &id, const QString &text, bool completed, QWidget *parent)
        : QWidget(parent), m_id(id)
    {
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(4, 2, 4, 2);
        lay->setSpacing(4);

        m_checkBox = new QCheckBox(this);
        m_checkBox->setChecked(completed);
        connect(m_checkBox, &QCheckBox::toggled, this, [this](bool checked) {
            emit checkStateChanged(m_id, checked);
        });
        lay->addWidget(m_checkBox);

        // Stacked widget: label (view) / line edit (edit)
        m_stack = new QStackedWidget(this);
        m_label = new QLabel(text, this);
        m_label->setWordWrap(true);
        m_editor = new QLineEdit(this);
        m_editor->setPlaceholderText(loc("Task Text:"));
        m_stack->addWidget(m_label);   // index 0 = view
        m_stack->addWidget(m_editor);  // index 1 = edit
        m_stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        lay->addWidget(m_stack, /*stretch=*/1);

        connect(m_editor, &QLineEdit::returnPressed, this, &TaskRow::commitEdit);

        m_editBtn = new QPushButton(QStringLiteral("\u270E"), this); // ✎
        m_editBtn->setFixedSize(26, 26);
        m_editBtn->setFlat(true);
        m_editBtn->setToolTip(loc("Edit Task"));
        connect(m_editBtn, &QPushButton::clicked, this, &TaskRow::startEditing);
        lay->addWidget(m_editBtn);

        m_deleteBtn = new QPushButton(QStringLiteral("\U0001F5D1"), this); // 🗑
        m_deleteBtn->setFixedSize(26, 26);
        m_deleteBtn->setFlat(true);
        m_deleteBtn->setToolTip(loc("Delete"));
        connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
            emit deleteRequested(m_id);
        });
        lay->addWidget(m_deleteBtn);
    }

    QString id() const { return m_id; }
    QString text() const { return m_label->text(); }
    bool isChecked() const { return m_checkBox->isChecked(); }
    void setText(const QString &t) { m_label->setText(t); }

    void startEditing()
    {
        m_editor->setText(m_label->text());
        m_stack->setCurrentIndex(1);
        m_editor->setFocus();
        m_editor->selectAll();
    }

    void setTaskFontSize(int px)
    {
        m_label->setStyleSheet(QStringLiteral("font-size:%1px;").arg(px));
    }

    void setButtonStyle(const QString &style)
    {
        m_editBtn->setStyleSheet(style);
        m_deleteBtn->setStyleSheet(style);
    }

signals:
    void checkStateChanged(const QString &id, bool checked);
    void editFinished(const QString &id, const QString &newText);
    void editCancelled(const QString &id);
    void deleteRequested(const QString &id);

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape && m_stack->currentIndex() == 1) {
            cancelEdit();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    void commitEdit()
    {
        QString newText = m_editor->text().trimmed();
        if (newText.isEmpty()) {
            cancelEdit();
            return;
        }
        m_label->setText(newText);
        m_stack->setCurrentIndex(0);
        emit editFinished(m_id, newText);
    }

    void cancelEdit()
    {
        m_stack->setCurrentIndex(0);
        emit editCancelled(m_id);
    }

    QString m_id;
    QCheckBox *m_checkBox;
    QStackedWidget *m_stack;
    QLabel *m_label;
    QLineEdit *m_editor;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
};

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
    m_taskList->setSelectionMode(QAbstractItemView::NoSelection);
    m_taskList->setFocusPolicy(Qt::NoFocus);
    m_taskList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    lay->addWidget(m_taskList, /*stretch=*/1);

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
    m_taskList->blockSignals(true);
    m_taskList->clear();

    const auto tasks = m_data->loadTasks(m_currentDate);
    for (const auto &t : tasks) {
        auto *item = new QListWidgetItem(m_taskList);
        auto *row = new TaskRow(t.id, t.text, t.completed, m_taskList);

        connect(row, &TaskRow::checkStateChanged, this, &TaskWidget::onRowChecked);
        connect(row, &TaskRow::editFinished, this, &TaskWidget::onRowEditFinished);
        connect(row, &TaskRow::editCancelled, this, &TaskWidget::onRowEditCancelled);
        connect(row, &TaskRow::deleteRequested, this, &TaskWidget::onRowDelete);

        m_taskList->setItemWidget(item, row);
        item->setSizeHint(row->sizeHint());
    }

    {
        auto *addItem = new QListWidgetItem(m_taskList);
        auto *addBtn = new QPushButton(
            QStringLiteral("+ ") + loc("Add Task"), m_taskList);
        addBtn->setStyleSheet(kBtnStyle);
        addBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(addBtn, &QPushButton::clicked, this, &TaskWidget::onAddTask);

        m_taskList->setItemWidget(addItem, addBtn);
        addItem->setSizeHint(addBtn->sizeHint());
    }

    {
        auto *cleanupItem = new QListWidgetItem(m_taskList);
        auto *cleanupWidget = new QWidget(m_taskList);
        auto *cl = new QHBoxLayout(cleanupWidget);
        cl->setContentsMargins(4, 0, 4, 2);
        cl->addStretch();
        auto *cleanupBtn = new QPushButton(QStringLiteral("\U0001F9F9"), cleanupWidget);
        cleanupBtn->setFixedSize(24, 24);
        cleanupBtn->setFlat(true);
        cleanupBtn->setToolTip(loc("Cleanup"));
        connect(cleanupBtn, &QPushButton::clicked, this, &TaskWidget::onCleanup);
        cl->addWidget(cleanupBtn);
        m_taskList->setItemWidget(cleanupItem, cleanupWidget);
        cleanupItem->setSizeHint(cleanupWidget->sizeHint());
    }

    m_taskList->blockSignals(false);
}

// ---------------------------------------------------------------------------
// Save current list widget contents back to data layer
// ---------------------------------------------------------------------------
void TaskWidget::saveCurrentList()
{
    QList<TaskItem> tasks;

    // Iterate all items except the last two (Add row + Cleanup row)
    int taskCount = m_taskList->count() - 2;
    tasks.reserve(taskCount);

    for (int i = 0; i < taskCount; ++i) {
        auto *item = m_taskList->item(i);
        auto *row = qobject_cast<TaskRow *>(m_taskList->itemWidget(item));
        if (!row)
            continue;

        TaskItem t;
        t.id        = row->id();
        t.text      = row->text();
        t.completed = row->isChecked();
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

    // Count real tasks (exclude Add row and Cleanup row at the end)
    int total = m_taskList->count() - 2;
    int done  = 0;
    for (int i = 0; i < total; ++i) {
        auto *row = qobject_cast<TaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row && row->isChecked())
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

    // Apply font size to all TaskRow widgets
    for (int i = 0; i < m_taskList->count() - 2; ++i) {
        auto *row = qobject_cast<TaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row) {
            row->setTaskFontSize(m_taskFontSize);
            row->setButtonStyle(btnStyle);
        }
    }

    // Style the "Add" row button (second-to-last item)
    if (m_taskList->count() >= 2) {
        auto *addBtn = qobject_cast<QPushButton *>(
            m_taskList->itemWidget(m_taskList->item(m_taskList->count() - 2)));
        if (addBtn)
            addBtn->setStyleSheet(btnStyle);
    }

    // Cleanup row button (last item) – find the button inside its widget
    if (m_taskList->count() >= 1) {
        auto *cleanupWidget = m_taskList->itemWidget(m_taskList->item(m_taskList->count() - 1));
        if (cleanupWidget) {
            auto *cleanupBtn = cleanupWidget->findChild<QPushButton *>();
            if (cleanupBtn)
                cleanupBtn->setStyleSheet(btnStyle);
        }
    }

    // Create view buttons
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
    QString newId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Insert new empty task row just before the Add row
    int insertPos = m_taskList->count() - 2; // before add + cleanup rows
    if (insertPos < 0) insertPos = 0;

    auto *item = new QListWidgetItem;
    auto *row = new TaskRow(newId, QString(), false, m_taskList);

    connect(row, &TaskRow::checkStateChanged, this, &TaskWidget::onRowChecked);
    connect(row, &TaskRow::editFinished, this, &TaskWidget::onRowEditFinished);
    connect(row, &TaskRow::editCancelled, this, &TaskWidget::onRowEditCancelled);
    connect(row, &TaskRow::deleteRequested, this, &TaskWidget::onRowDelete);

    m_taskList->insertItem(insertPos, item);
    m_taskList->setItemWidget(item, row);
    item->setSizeHint(row->sizeHint());

    // Immediately start inline editing
    row->startEditing();
}

void TaskWidget::onRowChecked(const QString &id, bool checked)
{
    Q_UNUSED(id);
    Q_UNUSED(checked);
    saveCurrentList();
}

void TaskWidget::onRowEditFinished(const QString &id, const QString &newText)
{
    Q_UNUSED(newText);
    auto *row = findRowById(id);
    if (!row)
        return;
    saveCurrentList();
    if (m_taskList->count() == 3)
        refreshAll();
}

void TaskWidget::onRowEditCancelled(const QString &id)
{
    auto *row = findRowById(id);
    if (row && row->text().isEmpty()) {
        for (int i = 0; i < m_taskList->count() - 2; ++i) {
            auto *r = qobject_cast<TaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
            if (r && r->id() == id) {
                delete m_taskList->takeItem(i);
                saveCurrentList();
                return;
            }
        }
    }
}

void TaskWidget::onRowDelete(const QString &id)
{
    for (int i = 0; i < m_taskList->count() - 2; ++i) {
        auto *row = qobject_cast<TaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row && row->id() == id) {
            delete m_taskList->takeItem(i);
            saveCurrentList();
            return;
        }
    }
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

// ---------------------------------------------------------------------------
// Helper – find a TaskRow by task id
// ---------------------------------------------------------------------------
TaskRow *TaskWidget::findRowById(const QString &id) const
{
    for (int i = 0; i < m_taskList->count() - 2; ++i) {
        auto *row = qobject_cast<TaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row && row->id() == id)
            return row;
    }
    return nullptr;
}

#include "taskwidget.moc"
