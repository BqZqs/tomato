#pragma once

#include <QDate>
#include <QWidget>

class TaskData;
class QListWidget;
class QPushButton;
class QLabel;
class QDateEdit;
class QStackedWidget;
class QVBoxLayout;
class QHBoxLayout;

// Forward declaration for TaskRow defined in taskwidget.cpp
class TaskRow;

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
    void onCleanup();
    void onRowChecked(const QString &id, bool checked);
    void onRowEditFinished(const QString &id, const QString &newText);
    void onRowEditCancelled(const QString &id);
    void onRowDelete(const QString &id);

    // Translation refresh
    void refreshTexts();

    // Font size
    void onFontOffsetChanged(int offset);

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
    void applyFontSize();
    bool isAtWindowStart() const;
    bool isAtWindowEnd() const;
    QPair<QDate, QDate> navWindow() const;

    TaskRow *findRowById(const QString &id) const;

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

    // Page 1 — create view
    QPushButton *m_btnCreateEmpty = nullptr;
    QPushButton *m_btnInheritAll = nullptr;
    QPushButton *m_btnInheritIncomplete = nullptr;

    int m_taskFontSize = 16;
};
