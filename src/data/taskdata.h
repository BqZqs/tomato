#pragma once

#include <QDate>
#include <QList>
#include <QPair>
#include <QString>

struct TaskItem {
    QString id;
    QString text;
    bool completed = false;
    int order = 0;
    int durationMinutes = 0;
};

class TaskData {
public:
    explicit TaskData(const QString &dataDir);

    // Core CRUD
    QList<TaskItem> loadTasks(QDate date) const;
    bool saveTasks(QDate date, const QList<TaskItem> &tasks) const;
    bool hasTaskFile(QDate date) const;
    bool deleteTaskFile(QDate date) const;

    // Creation modes
    bool createEmptyList(QDate date) const;
    bool inheritAll(QDate source, QDate target) const;
    bool inheritIncomplete(QDate source, QDate target) const;

    // Date window (today ± 7 days)
    QPair<QDate, QDate> validWindow() const;
    bool isDateInWindow(QDate date) const;
    QList<QDate> existingDates() const;

    // Manual cleanup (user-triggered)
    int cleanupOutsideWindow();

    // Path helpers
    QString tasksDir() const;
    QString filePath(QDate date) const;

private:
    static QString fmt(QDate d);
    QString m_dataDir;
};
