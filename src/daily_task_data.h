#pragma once
#include <QList>
#include <QString>

struct DailyTaskItem {
    QString id;
    QString text;
    int durationMinutes = 25;
    bool completed = false;
    int order = 0;
};

class DailyTaskData {
public:
    explicit DailyTaskData(const QString &dataDir);
    QList<DailyTaskItem> loadTasks(int weekday) const;  // 1=Mon..7=Sun
    bool saveTasks(int weekday, const QList<DailyTaskItem> &tasks) const;
    bool hasTasks(int weekday) const;
    static QString weekdayName(int weekday);  // "Monday", "Tuesday", etc.
    QString dailyDir() const;
    QString filePath(int weekday) const;
private:
    QString m_dataDir;
};
