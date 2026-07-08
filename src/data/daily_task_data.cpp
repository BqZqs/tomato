#include "daily_task_data.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <algorithm>

DailyTaskData::DailyTaskData(const QString &dataDir)
    : m_dataDir(dataDir)
{
    QDir().mkpath(dailyDir());
}

QString DailyTaskData::dailyDir() const
{
    return m_dataDir + QStringLiteral("/daily");
}

QString DailyTaskData::filePath(int weekday) const
{
    return dailyDir() + QStringLiteral("/") + weekdayName(weekday) + QStringLiteral(".json");
}

QString DailyTaskData::weekdayName(int weekday)
{
    switch (weekday) {
    case 1: return QStringLiteral("Monday");
    case 2: return QStringLiteral("Tuesday");
    case 3: return QStringLiteral("Wednesday");
    case 4: return QStringLiteral("Thursday");
    case 5: return QStringLiteral("Friday");
    case 6: return QStringLiteral("Saturday");
    case 7: return QStringLiteral("Sunday");
    default: return QString();
    }
}

QList<DailyTaskItem> DailyTaskData::loadTasks(int weekday) const
{
    QFile file(filePath(weekday));
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};

    const QJsonObject root = doc.object();
    const QJsonValue tasksVal = root.value(QStringLiteral("tasks"));
    if (!tasksVal.isArray())
        return {};

    QList<DailyTaskItem> tasks;
    const QJsonArray arr = tasksVal.toArray();
    for (const QJsonValue &v : arr) {
        if (!v.isObject())
            continue;
        const QJsonObject obj = v.toObject();
        DailyTaskItem item;
        item.id        = obj.value(QStringLiteral("id")).toString();
        item.text      = obj.value(QStringLiteral("text")).toString();
        item.durationMinutes = obj.value(QStringLiteral("duration")).toInt(25);
        item.completed = obj.value(QStringLiteral("completed")).toBool();
        item.order     = obj.value(QStringLiteral("order")).toInt(0);
        tasks.append(item);
    }

    std::sort(tasks.begin(), tasks.end(),
              [](const DailyTaskItem &a, const DailyTaskItem &b) { return a.order < b.order; });

    return tasks;
}

bool DailyTaskData::saveTasks(int weekday, const QList<DailyTaskItem> &tasks) const
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("day")]     = weekdayName(weekday);

    QJsonArray arr;
    for (const DailyTaskItem &item : tasks) {
        QJsonObject obj;
        obj[QStringLiteral("id")]        = item.id;
        obj[QStringLiteral("text")]      = item.text;
        obj[QStringLiteral("duration")]  = item.durationMinutes;
        obj[QStringLiteral("completed")] = item.completed;
        obj[QStringLiteral("order")]     = item.order;
        arr.append(obj);
    }
    root[QStringLiteral("tasks")] = arr;

    const QJsonDocument doc(root);

    QDir().mkpath(dailyDir());

    QSaveFile file(filePath(weekday));
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(doc.toJson());
    return file.commit();
}

bool DailyTaskData::hasTasks(int weekday) const
{
    return QFileInfo::exists(filePath(weekday));
}
