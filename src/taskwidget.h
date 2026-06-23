#pragma once

#include <QDate>
#include <QWidget>

class TaskData;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;
class QDateEdit;
class QStackedWidget;
class QVBoxLayout;
class QHBoxLayout;

class TaskWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaskWidget(TaskData *data, QWidget *parent = nullptr);

private slots:
    // Date navigation
    void onPrevDay();
    void onNextDay();
    void onToday();
    void onDateEditChanged(const QDate &date);

    // Task list actions
    void onAddTask();
    void onEditTask();
    void onDeleteTask();
    void onToggleTask();
    void onCleanup();
    void onItemCheckChanged(QListWidgetItem *item);

    // Translation refresh
    void refreshTexts();

    // Create actions
    void onCreateEmpty();
    void onInheritAll();
    void onInheritIncomplete();

private:
    void buildUi();
    QHBoxLayout *buildDateNav();
    QWidget *buildTaskListView();
    QWidget *buildCreateView();

    void refreshAll();
    void refreshDateNav();
    void populateTaskList();
    void saveCurrentList();
    void updateStatus();
    bool isAtWindowStart() const;
    bool isAtWindowEnd() const;
    QPair<QDate, QDate> navWindow() const;

    TaskData *m_data = nullptr;
    QDate m_currentDate;

    // Date navigation widgets
    QPushButton *m_btnPrev = nullptr;
    QPushButton *m_btnNext = nullptr;
    QPushButton *m_btnToday = nullptr;
    QDateEdit *m_dateEdit = nullptr;

    // Status label (between date nav and content)
    QLabel *m_statusLabel = nullptr;

    // Switches between list view and create view
    QStackedWidget *m_stack = nullptr;

    // Page 0 — task list view
    QListWidget *m_taskList = nullptr;
    QPushButton *m_btnAdd = nullptr;
    QPushButton *m_btnEdit = nullptr;
    QPushButton *m_btnDelete = nullptr;
    QPushButton *m_btnToggle = nullptr;
    QPushButton *m_btnCleanup = nullptr;

    // Page 1 — create view
    QPushButton *m_btnCreateEmpty = nullptr;
    QPushButton *m_btnInheritAll = nullptr;
    QPushButton *m_btnInheritIncomplete = nullptr;
};
