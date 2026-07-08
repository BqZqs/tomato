#include "daily_task_widget.h"
#include "daily_task_data.h"
#include "lang.h"
#include "theme.h"

#include <QComboBox>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QUuid>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Helper – combined dialog for adding/editing timed tasks
// ---------------------------------------------------------------------------
static bool showTimedTaskDialog(QWidget *parent, const QString &title,
                                 QString &text, int &minutes)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);

    auto *lay = new QFormLayout(&dlg);

    auto *textEdit = new QLineEdit(text);
    lay->addRow(loc("Task Text:"), textEdit);

    auto *spinBox = new QSpinBox;
    spinBox->setRange(1, 120);
    spinBox->setValue(minutes);
    lay->addRow(loc("Duration (minutes):"), spinBox);

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addRow(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    text = textEdit->text().trimmed();
    minutes = spinBox->value();
    return true;
}

// ---------------------------------------------------------------------------
// DailyTaskRow – per-task inline widget row
// Layout: [▶] [⏱ Nm] [task text] [✎] [🗑]
// ---------------------------------------------------------------------------
class DailyTaskRow : public QWidget {
    Q_OBJECT
public:
    DailyTaskRow(const QString &id, const QString &text, bool completed,
                 int durationMinutes, QWidget *parent)
        : QWidget(parent), m_id(id), m_durationMinutes(durationMinutes)
    {
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(6, 5, 6, 5);
        lay->setSpacing(4);

        m_playBtn = new QPushButton(QStringLiteral("\u25B6"), this); // ▶
        m_playBtn->setFixedSize(26, 26);
        m_playBtn->setFlat(true);
        m_playBtn->setToolTip(loc("Start timer for this task"));
        connect(m_playBtn, &QPushButton::clicked, this, [this]() {
            emit playRequested(m_id, m_durationMinutes);
        });
        lay->addWidget(m_playBtn);

        m_durationLabel = new QLabel(this);
        updateDurationLabel();
        lay->addWidget(m_durationLabel);

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

        connect(m_editor, &QLineEdit::returnPressed, this, &DailyTaskRow::commitEdit);

        m_editBtn = new QPushButton(QStringLiteral("\u270E"), this); // ✎
        m_editBtn->setFixedSize(26, 26);
        m_editBtn->setFlat(true);
        m_editBtn->setToolTip(loc("Edit Task"));
        connect(m_editBtn, &QPushButton::clicked, this, &DailyTaskRow::startEditing);
        lay->addWidget(m_editBtn);

        m_deleteBtn = new QPushButton(QStringLiteral("\U0001F5D1"), this); // 🗑
        m_deleteBtn->setFixedSize(26, 26);
        m_deleteBtn->setFlat(true);
        m_deleteBtn->setToolTip(loc("Delete"));
        connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
            emit deleteRequested(m_id);
        });
        lay->addWidget(m_deleteBtn);

        if (completed)
            setCompleted(true);
    }

    QString id() const { return m_id; }
    QString text() const { return m_label->text(); }
    int durationMinutes() const { return m_durationMinutes; }
    bool isCompleted() const { return m_completed; }
    void setText(const QString &t) { m_label->setText(t); }

    void setCompleted(bool c) {
        m_completed = c;
        applyLabelStyle();
    }

    void startEditing()
    {
        QString newText = m_label->text();
        int newMin = m_durationMinutes;
        if (!showTimedTaskDialog(this, loc("Edit Task"), newText, newMin)) {
            emit editCancelled(m_id);
            return;
        }
        if (newText.trimmed().isEmpty()) {
            emit editCancelled(m_id);
            return;
        }

        m_label->setText(newText.trimmed());
        if (newMin != m_durationMinutes) {
            m_durationMinutes = newMin;
            updateDurationLabel();
            emit durationChanged(m_id, m_durationMinutes);
        }
        emit editFinished(m_id, newText.trimmed());
    }

    void setTaskFontSize(int px)
    {
        m_fontSize = px;
        applyLabelStyle();
        m_durationLabel->setStyleSheet(
            QStringLiteral("font-size:%1px; color:#7B7D8C; padding:0 2px;").arg(px - 1));
    }

    void setButtonStyle(const QString &style)
    {
        m_playBtn->setStyleSheet(style);
        m_editBtn->setStyleSheet(style);
        m_deleteBtn->setStyleSheet(style);
    }

    void applyLabelStyle()
    {
        QString style = QStringLiteral("font-size:%1px;").arg(m_fontSize);
        if (m_completed)
            style += QStringLiteral("color:#B0B3BF; text-decoration:line-through;");
        m_label->setStyleSheet(style);
    }

signals:
    void playRequested(const QString &id, int minutes);
    void editFinished(const QString &id, const QString &newText);
    void editCancelled(const QString &id);
    void deleteRequested(const QString &id);
    void durationChanged(const QString &id, int newMinutes);

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

    void updateDurationLabel()
    {
        m_durationLabel->setText(
            QStringLiteral("\u23F1 %1m").arg(m_durationMinutes));
    }

    QString m_id;
    int m_durationMinutes = 0;
    bool m_completed = false;
    int m_fontSize = 14;
    QPushButton *m_playBtn;
    QLabel *m_durationLabel;
    QStackedWidget *m_stack;
    QLabel *m_label;
    QLineEdit *m_editor;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
};

// ---------------------------------------------------------------------------
// DailyTaskWidget
// ---------------------------------------------------------------------------
DailyTaskWidget::DailyTaskWidget(DailyTaskData *data, QWidget *parent)
    : QWidget(parent), m_data(data), m_weekday(currentWeekday())
{
    buildUi();
    populateTaskList();

    m_fontOffset = LocaleManager::instance()->fontOffset();
    applyFontSize();
    connect(LocaleManager::instance(), &LocaleManager::fontOffsetChanged,
            this, &DailyTaskWidget::onFontOffsetChanged);
}

void DailyTaskWidget::buildUi()
{
    auto *lay = new QVBoxLayout(this);
    lay->setSpacing(6);
    lay->setContentsMargins(12, 10, 12, 10);

    {
        auto *row = new QHBoxLayout;
        row->setSpacing(8);

        m_prevBtn = new QPushButton(QStringLiteral("\u25C0"), this);
        m_prevBtn->setStyleSheet(Theme::kNavBtnStyleSheet);
        m_prevBtn->setToolTip(loc("Previous day"));
        connect(m_prevBtn, &QPushButton::clicked, this, &DailyTaskWidget::onPrevDay);
        row->addWidget(m_prevBtn);

        m_weekdayCombo = new QComboBox(this);
        m_weekdayCombo->setStyleSheet(Theme::kComboStyleSheet);
        m_weekdayCombo->addItem(loc("Monday"));
        m_weekdayCombo->addItem(loc("Tuesday"));
        m_weekdayCombo->addItem(loc("Wednesday"));
        m_weekdayCombo->addItem(loc("Thursday"));
        m_weekdayCombo->addItem(loc("Friday"));
        m_weekdayCombo->addItem(loc("Saturday"));
        m_weekdayCombo->addItem(loc("Sunday"));
        m_weekdayCombo->setCurrentIndex(m_weekday - 1);
        row->addWidget(m_weekdayCombo, 1);

        m_nextBtn = new QPushButton(QStringLiteral("\u25B6"), this);
        m_nextBtn->setStyleSheet(Theme::kNavBtnStyleSheet);
        m_nextBtn->setToolTip(loc("Next day"));
        connect(m_nextBtn, &QPushButton::clicked, this, &DailyTaskWidget::onNextDay);
        row->addWidget(m_nextBtn);

        m_thisWeekBtn = new QPushButton(loc("Today"), this);
        m_thisWeekBtn->setStyleSheet(Theme::kBtnStyleSheet);
        connect(m_thisWeekBtn, &QPushButton::clicked, this, &DailyTaskWidget::onThisWeek);
        row->addWidget(m_thisWeekBtn);

        lay->addLayout(row);
    }

    connect(m_weekdayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DailyTaskWidget::onWeekdayChanged);
    connect(m_thisWeekBtn, &QPushButton::clicked, this, &DailyTaskWidget::onThisWeek);

    m_taskList = new QListWidget(this);
    m_taskList->setStyleSheet(Theme::kListStyleSheet);
    m_taskList->setAlternatingRowColors(false);
    m_taskList->setSelectionMode(QAbstractItemView::NoSelection);
    m_taskList->setFocusPolicy(Qt::NoFocus);
    m_taskList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    lay->addWidget(m_taskList, 1);

    lay->addStretch();

    connect(LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &DailyTaskWidget::refreshTexts);
}

// ---------------------------------------------------------------------------
// Populate task list from data layer
// ---------------------------------------------------------------------------
void DailyTaskWidget::populateTaskList()
{
    m_taskList->blockSignals(true);
    m_taskList->clear();

    const auto tasks = m_data->loadTasks(m_weekday);

    for (const auto &t : tasks) {
        auto *item = new QListWidgetItem(m_taskList);
        auto *row = new DailyTaskRow(t.id, t.text, t.completed, t.durationMinutes, m_taskList);

        connect(row, &DailyTaskRow::playRequested, this, &DailyTaskWidget::onRowPlay);
        connect(row, &DailyTaskRow::editFinished, this, &DailyTaskWidget::onRowEditFinished);
        connect(row, &DailyTaskRow::editCancelled, this, &DailyTaskWidget::onRowEditCancelled);
        connect(row, &DailyTaskRow::deleteRequested, this, &DailyTaskWidget::onRowDelete);
        connect(row, &DailyTaskRow::durationChanged, this, &DailyTaskWidget::onRowDurationChanged);

        m_taskList->setItemWidget(item, row);
        item->setSizeHint(row->sizeHint());
    }

    // Add button row
    {
        auto *addItem = new QListWidgetItem(m_taskList);
        auto *addWidget = new QWidget(m_taskList);
        auto *addLay = new QHBoxLayout(addWidget);
        addLay->setContentsMargins(4, 2, 4, 2);

        auto *addBtn = new QPushButton(
            QStringLiteral("+ ") + loc("Add Timed Task"), addWidget);
        addBtn->setStyleSheet(Theme::kBtnStyleSheet);
        connect(addBtn, &QPushButton::clicked, this, &DailyTaskWidget::onAddTimedTask);
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
void DailyTaskWidget::saveCurrentList()
{
    QList<DailyTaskItem> tasks;

    int taskCount = m_taskList->count() - 1; // exclude add button row
    tasks.reserve(taskCount);

    for (int i = 0; i < taskCount; ++i) {
        auto *item = m_taskList->item(i);
        auto *row = qobject_cast<DailyTaskRow *>(m_taskList->itemWidget(item));
        if (!row)
            continue;

        DailyTaskItem t;
        t.id        = row->id();
        t.text      = row->text();
        t.durationMinutes = row->durationMinutes();
        t.completed = row->isCompleted();
        t.order     = i;
        tasks.append(t);
    }

    m_data->saveTasks(m_weekday, tasks);
}

// ---------------------------------------------------------------------------
// Current weekday helper
// ---------------------------------------------------------------------------
int DailyTaskWidget::currentWeekday() const
{
    int dow = QDate::currentDate().dayOfWeek(); // 1=Mon..7=Sun
    return dow;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void DailyTaskWidget::onAddTimedTask()
{
    QString text;
    int minutes = 25;
    if (!showTimedTaskDialog(this, loc("Add Timed Task"), text, minutes))
        return;
    if (text.trimmed().isEmpty()) return;

    QString newId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    int insertPos = m_taskList->count() - 1;
    if (insertPos < 0) insertPos = 0;

    auto *item = new QListWidgetItem;
    auto *row = new DailyTaskRow(newId, text.trimmed(), false, minutes, m_taskList);

    connect(row, &DailyTaskRow::playRequested, this, &DailyTaskWidget::onRowPlay);
    connect(row, &DailyTaskRow::editFinished, this, &DailyTaskWidget::onRowEditFinished);
    connect(row, &DailyTaskRow::editCancelled, this, &DailyTaskWidget::onRowEditCancelled);
    connect(row, &DailyTaskRow::deleteRequested, this, &DailyTaskWidget::onRowDelete);
    connect(row, &DailyTaskRow::durationChanged, this, &DailyTaskWidget::onRowDurationChanged);

    m_taskList->insertItem(insertPos, item);
    m_taskList->setItemWidget(item, row);
    item->setSizeHint(row->sizeHint());

    applyFontSize();

    saveCurrentList();
}

void DailyTaskWidget::onRowPlay(const QString &id, int minutes)
{
    emit startTimerForTask(id, minutes);
}

void DailyTaskWidget::onTimedSessionFinished(const QString &taskId)
{
    auto *row = findRowById(taskId);
    if (!row) return;
    row->setCompleted(true);
    saveCurrentList();
}

void DailyTaskWidget::onRowEditFinished(const QString &id, const QString &newText)
{
    Q_UNUSED(newText);
    auto *row = findRowById(id);
    if (!row)
        return;
    saveCurrentList();
}

void DailyTaskWidget::onRowEditCancelled(const QString &id)
{
    auto *row = findRowById(id);
    if (row && row->text().isEmpty()) {
        for (int i = 0; i < m_taskList->count() - 1; ++i) {
            auto *r = qobject_cast<DailyTaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
            if (r && r->id() == id) {
                delete m_taskList->takeItem(i);
                saveCurrentList();
                return;
            }
        }
    }
}

void DailyTaskWidget::onRowDelete(const QString &id)
{
    for (int i = 0; i < m_taskList->count() - 1; ++i) {
        auto *row = qobject_cast<DailyTaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row && row->id() == id) {
            delete m_taskList->takeItem(i);
            saveCurrentList();
            return;
        }
    }
}

void DailyTaskWidget::onRowDurationChanged(const QString &id, int newMinutes)
{
    Q_UNUSED(id);
    Q_UNUSED(newMinutes);
    saveCurrentList();
}

void DailyTaskWidget::onPrevDay()
{
    m_weekday--;
    if (m_weekday < 1) m_weekday = 7;
    m_weekdayCombo->setCurrentIndex(m_weekday - 1);
    populateTaskList();
}

void DailyTaskWidget::onNextDay()
{
    m_weekday++;
    if (m_weekday > 7) m_weekday = 1;
    m_weekdayCombo->setCurrentIndex(m_weekday - 1);
    populateTaskList();
}

void DailyTaskWidget::onWeekdayChanged(int index)
{
    m_weekday = index + 1; // 0=Monday → 1
    populateTaskList();
}

void DailyTaskWidget::onThisWeek()
{
    m_weekday = currentWeekday();
    m_weekdayCombo->setCurrentIndex(m_weekday - 1);
    populateTaskList();
}

// ---------------------------------------------------------------------------
// Language / translation refresh
// ---------------------------------------------------------------------------
void DailyTaskWidget::refreshTexts()
{
    // Re-label the add button
    populateTaskList();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
DailyTaskRow *DailyTaskWidget::findRowById(const QString &id) const
{
    for (int i = 0; i < m_taskList->count() - 1; ++i) {
        auto *row = qobject_cast<DailyTaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row && row->id() == id)
            return row;
    }
    return nullptr;
}

void DailyTaskWidget::applyFontSize()
{
    int textSize = qMax(8, 14 + m_fontOffset);
    int btnSize = qMax(8, 13 + m_fontOffset);
    int comboSize = qMax(8, 13 + m_fontOffset);
    int navSize = qMax(8, 14 + m_fontOffset);

    const QString btnStyle = QStringLiteral("font-size:%1px;").arg(btnSize)
                             + QString::fromLatin1(Theme::kBtnStyleSheet);
    const QString comboStyle = QStringLiteral("font-size:%1px;").arg(comboSize)
                               + QString::fromLatin1(Theme::kComboStyleSheet);
    const QString navBtnStyle = QStringLiteral("font-size:%1px;").arg(navSize)
                                + QString::fromLatin1(Theme::kNavBtnStyleSheet);

    for (int i = 0; i < m_taskList->count() - 1; ++i) {
        auto *row = qobject_cast<DailyTaskRow *>(m_taskList->itemWidget(m_taskList->item(i)));
        if (row) {
            row->setTaskFontSize(textSize);
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

    m_prevBtn->setStyleSheet(navBtnStyle);
    m_nextBtn->setStyleSheet(navBtnStyle);
    m_weekdayCombo->setStyleSheet(comboStyle);
    m_thisWeekBtn->setStyleSheet(btnStyle);
}

void DailyTaskWidget::onFontOffsetChanged(int offset)
{
    m_fontOffset = offset;
    applyFontSize();
}

#include "daily_task_widget.moc"
