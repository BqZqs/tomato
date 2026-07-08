#include "taskwidget.h"
#include "lang.h"
#include "taskdata.h"
#include "theme.h"

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

// Local style helpers (not shared – simple enough to stay local)
static const char *kStatusStyle = "color:#7B7D8C";
static const char *kNoListLabelStyle = "color:#7B7D8C;padding:12px";

// ---------------------------------------------------------------------------
// TaskRow – per-task inline widget row (checklist only, no timer features)
// ---------------------------------------------------------------------------
class TaskRow : public QWidget {
    Q_OBJECT
public:
    TaskRow(const QString &id, const QString &text, bool completed,
            QWidget *parent)
        : QWidget(parent), m_id(id)
    {
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(6, 5, 6, 5);
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
    void setChecked(bool c) { m_checkBox->setChecked(c); }

    void setCompleted(bool c) {
        m_checkBox->setChecked(c);
        applyLabelStyle();
    }

    void startEditing()
    {
        m_editor->setText(m_label->text());
        m_stack->setCurrentIndex(1);
        m_editor->setFocus();
        m_editor->selectAll();
    }

    void setTaskFontSize(int px)
    {
        m_fontSize = px;
        applyLabelStyle();
        m_editor->setStyleSheet(
            QStringLiteral("font-size:%1px; padding:2px 6px;").arg(px));
    }

    void setButtonStyle(const QString &style)
    {
        m_editBtn->setStyleSheet(style);
        m_deleteBtn->setStyleSheet(style);
    }

    void applyLabelStyle()
    {
        QString style = QStringLiteral("font-size:%1px;").arg(m_fontSize);
        if (m_checkBox->isChecked())
            style += QStringLiteral("color:#B0B3BF; text-decoration:line-through;");
        else
            style += QStringLiteral("color:#1A1A2E;");
        m_label->setStyleSheet(style);
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
    int m_fontSize = 14;
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
TaskWidget::TaskWidget(TaskData *data, QWidget *parent)
    : QWidget(parent), m_data(data), m_currentDate(QDate::currentDate())
{
    buildUi();

    m_fontOffset = LocaleManager::instance()->fontOffset();
    m_taskFontSize = qMax(8, 14 + m_fontOffset);
    applyFontSize();

    refreshAll();

    connect(LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &TaskWidget::refreshTexts);
    connect(LocaleManager::instance(), &LocaleManager::fontOffsetChanged,
            this, &TaskWidget::onFontOffsetChanged);
}

void TaskWidget::triggerCleanup()
{
    onCleanup();
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------
void TaskWidget::buildUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(12, 10, 12, 10);

    // --- Date navigation bar (always visible) ---
    mainLayout->addLayout(buildDateNav());

    // --- Stacked widget for list / create views ---
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(buildTaskListView());   // index 0
    m_stack->addWidget(buildCreateView());      // index 1
    mainLayout->addWidget(m_stack, /*stretch=*/1);

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
    m_btnPrev->setStyleSheet(Theme::kNavBtnStyleSheet);
    m_btnPrev->setToolTip(loc("Previous day"));
    row->addWidget(m_btnPrev);

    m_dateEdit = new QDateEdit(m_currentDate, this);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setAlignment(Qt::AlignCenter);
    m_dateEdit->setStyleSheet(Theme::kDateEditStyleSheet);
    row->addWidget(m_dateEdit, /*stretch=*/1);

    m_btnNext = new QPushButton(QStringLiteral("\u25B6"), this); // ▶
    m_btnNext->setStyleSheet(Theme::kNavBtnStyleSheet);
    m_btnNext->setToolTip(loc("Next day"));
    row->addWidget(m_btnNext);

    m_btnToday = new QPushButton(loc("Today"), this);
    m_btnToday->setStyleSheet(Theme::kNavBtnStyleSheet);
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
    m_taskList->setStyleSheet(Theme::kListStyleSheet);
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
    m_btnCreateEmpty->setStyleSheet(Theme::kBtnStyleSheet);
    lay->addWidget(m_btnCreateEmpty);

    m_btnInheritAll = new QPushButton(loc("Inherit All from..."), page);
    m_btnInheritAll->setStyleSheet(Theme::kBtnStyleSheet);
    lay->addWidget(m_btnInheritAll);

    m_btnInheritIncomplete = new QPushButton(loc("Inherit Incomplete from..."), page);
    m_btnInheritIncomplete->setStyleSheet(Theme::kBtnStyleSheet);
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
    } else {
        m_stack->setCurrentIndex(1);
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

    // Sort by order only
    QList<TaskItem> sorted = tasks;
    std::stable_sort(sorted.begin(), sorted.end(), [](const TaskItem &a, const TaskItem &b) {
        return a.order < b.order;
    });

    for (const auto &t : sorted) {
        auto *item = new QListWidgetItem(m_taskList);
        auto *row = new TaskRow(t.id, t.text, t.completed, m_taskList);

        connect(row, &TaskRow::checkStateChanged, this, &TaskWidget::onRowChecked);
        connect(row, &TaskRow::editFinished, this, &TaskWidget::onRowEditFinished);
        connect(row, &TaskRow::editCancelled, this, &TaskWidget::onRowEditCancelled);
        connect(row, &TaskRow::deleteRequested, this, &TaskWidget::onRowDelete);

        m_taskList->setItemWidget(item, row);
        item->setSizeHint(row->sizeHint());
    }

    // Single "Add Task" button
    {
        auto *addItem = new QListWidgetItem(m_taskList);
        auto *addWidget = new QWidget(m_taskList);
        auto *addLay = new QHBoxLayout(addWidget);
        addLay->setContentsMargins(4, 2, 4, 2);
        addLay->setSpacing(4);

        auto *addBtn = new QPushButton(
            QStringLiteral("+ ") + loc("Add Task"), addWidget);
        addBtn->setStyleSheet(Theme::kBtnStyleSheet);
        connect(addBtn, &QPushButton::clicked, this, &TaskWidget::onAddTask);
        addLay->addWidget(addBtn, 1);

        m_taskList->setItemWidget(addItem, addWidget);
        addItem->setSizeHint(addWidget->sizeHint());
    }

    m_taskList->blockSignals(false);
    applyFontSize();
}

// ---------------------------------------------------------------------------
// Save current list widget contents back to data layer
// ---------------------------------------------------------------------------
void TaskWidget::saveCurrentList()
{
    QList<TaskItem> tasks;

    // Iterate all items except the last two (Add row + Cleanup row)
    int taskCount = m_taskList->count() - 1;
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
}

// ---------------------------------------------------------------------------
// Font size
// ---------------------------------------------------------------------------
void TaskWidget::applyFontSize()
{
    int btnSize = qMax(8, 13 + m_fontOffset);
    int navSize = qMax(8, 14 + m_fontOffset);
    int statusSize = qMax(8, 12 + m_fontOffset);

    const QString btnStyle = QStringLiteral("font-size:%1px;").arg(btnSize)
                             + QString::fromLatin1(Theme::kBtnStyleSheet);
    const QString navBtnStyle = QStringLiteral("font-size:%1px;").arg(navSize)
                                + QString::fromLatin1(Theme::kNavBtnStyleSheet);
    const QString dateEditStyle = QStringLiteral("font-size:%1px;").arg(navSize)
                                  + QString::fromLatin1(Theme::kDateEditStyleSheet);

    // Apply font size to all TaskRow widgets
    for (int i = 0; i < m_taskList->count() - 1; ++i) {
        auto *row = qobject_cast<TaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row) {
            row->setTaskFontSize(m_taskFontSize);
            row->setButtonStyle(btnStyle);
        }
    }

    if (m_taskList->count() >= 1) {
        auto *addWidget = m_taskList->itemWidget(m_taskList->item(m_taskList->count() - 1));
        if (addWidget) {
            auto buttons = addWidget->findChildren<QPushButton *>();
            for (auto *btn : buttons)
                btn->setStyleSheet(btnStyle);
        }
    }

    // Create view buttons
    m_btnCreateEmpty->setStyleSheet(btnStyle);
    m_btnInheritAll->setStyleSheet(btnStyle);
    m_btnInheritIncomplete->setStyleSheet(btnStyle);

    // Date navigation
    m_dateEdit->setStyleSheet(dateEditStyle);
    m_btnPrev->setStyleSheet(navBtnStyle);
    m_btnNext->setStyleSheet(navBtnStyle);
    m_btnToday->setStyleSheet(navBtnStyle);
}

void TaskWidget::onFontOffsetChanged(int offset)
{
    m_fontOffset = offset;
    m_taskFontSize = qMax(8, 14 + offset);
    applyFontSize();
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
    int insertPos = m_taskList->count() - 1; // before add + cleanup rows
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

    applyFontSize();

    // Immediately start inline editing
    row->startEditing();
}

void TaskWidget::onRowChecked(const QString &id, bool checked)
{
    auto *row = findRowById(id);
    if (row)
        row->setCompleted(checked);
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
        for (int i = 0; i < m_taskList->count() - 1; ++i) {
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
    for (int i = 0; i < m_taskList->count() - 1; ++i) {
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
    if (count > 0)
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
    for (int i = 0; i < m_taskList->count() - 1; ++i) {
        auto *row = qobject_cast<TaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row && row->id() == id)
            return row;
    }
    return nullptr;
}

#include "taskwidget.moc"
