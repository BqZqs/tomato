#include "taskdata.h"

#include <algorithm>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

TaskData::TaskData(const QString &dataDir)
    : m_dataDir(dataDir)
{
    QDir().mkpath(tasksDir());
}

// ── Path helpers ────────────────────────────────────────────────────

QString TaskData::tasksDir() const
{
    return m_dataDir + QStringLiteral("/tasks");
}

QString TaskData::filePath(QDate date) const
{
    return tasksDir() + QStringLiteral("/") + fmt(date) + QStringLiteral(".json");
}

QString TaskData::fmt(QDate d)
{
    return d.toString(QStringLiteral("yyyy-MM-dd"));
}

// ── Core CRUD ───────────────────────────────────────────────────────

QList<TaskItem> TaskData::loadTasks(QDate date) const
{
    QFile file(filePath(date));
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

    QList<TaskItem> tasks;
    const QJsonArray arr = tasksVal.toArray();
    for (const QJsonValue &v : arr) {
        if (!v.isObject())
            continue;
        const QJsonObject obj = v.toObject();
        TaskItem item;
        item.id        = obj.value(QStringLiteral("id")).toString();
        item.text      = obj.value(QStringLiteral("text")).toString();
        item.completed = obj.value(QStringLiteral("completed")).toBool();
        item.order     = obj.value(QStringLiteral("order")).toInt(0);
        item.durationMinutes = obj.value(QStringLiteral("duration")).toInt(0);
        tasks.append(item);
    }

    std::sort(tasks.begin(), tasks.end(),
              [](const TaskItem &a, const TaskItem &b) { return a.order < b.order; });

    return tasks;
}

bool TaskData::saveTasks(QDate date, const QList<TaskItem> &tasks) const
{
    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("date")]    = fmt(date);

    QJsonArray arr;
    for (const TaskItem &item : tasks) {
        QJsonObject obj;
        obj[QStringLiteral("id")]        = item.id;
        obj[QStringLiteral("text")]      = item.text;
        obj[QStringLiteral("completed")] = item.completed;
        obj[QStringLiteral("order")]     = item.order;
        obj[QStringLiteral("duration")]  = item.durationMinutes;
        arr.append(obj);
    }
    root[QStringLiteral("tasks")] = arr;

    const QJsonDocument doc(root);

    // Ensure parent directory exists
    QDir().mkpath(tasksDir());

    QSaveFile file(filePath(date));
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(doc.toJson());
    return file.commit();
}

bool TaskData::hasTaskFile(QDate date) const
{
    return QFileInfo::exists(filePath(date));
}

bool TaskData::deleteTaskFile(QDate date) const
{
    return QFile::remove(filePath(date));
}

// ── Creation modes ──────────────────────────────────────────────────

bool TaskData::createEmptyList(QDate date) const
{
    return saveTasks(date, {});
}

bool TaskData::inheritAll(QDate source, QDate target) const
{
    if (!hasTaskFile(source))
        return false;

    QList<TaskItem> tasks = loadTasks(source);
    for (TaskItem &item : tasks)
        item.completed = false;

    return saveTasks(target, tasks);
}

bool TaskData::inheritIncomplete(QDate source, QDate target) const
{
    if (!hasTaskFile(source))
        return false;

    const QList<TaskItem> sourceTasks = loadTasks(source);
    QList<TaskItem> incomplete;
    for (const TaskItem &item : sourceTasks) {
        if (!item.completed)
            incomplete.append(item);
    }

    return saveTasks(target, incomplete);
}

// ── Date window ─────────────────────────────────────────────────────

QPair<QDate, QDate> TaskData::validWindow() const
{
    const QDate today = QDate::currentDate();
    return {today.addDays(-7), today.addDays(7)};
}

bool TaskData::isDateInWindow(QDate date) const
{
    const auto [start, end] = validWindow();
    return date >= start && date <= end;
}

QList<QDate> TaskData::existingDates() const
{
    QList<QDate> dates;
    const QDir dir(tasksDir());
    if (!dir.exists())
        return dates;

    const QStringList filters{QStringLiteral("*.json")};
    const auto entries = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo &info : entries) {
        QString name = info.fileName();
        if (!name.endsWith(QStringLiteral(".json")))
            continue;
        name.chop(5); // remove ".json"
        const QDate d = QDate::fromString(name, QStringLiteral("yyyy-MM-dd"));
        if (d.isValid())
            dates.append(d);
    }

    std::sort(dates.begin(), dates.end());
    return dates;
}

// ── Manual cleanup ──────────────────────────────────────────────────

int TaskData::cleanupOutsideWindow()
{
    int count = 0;
    const QDir dir(tasksDir());
    if (!dir.exists())
        return 0;

    const QDate today = QDate::currentDate();
    const auto [start, end] = validWindow();
    const QString todayFileName = fmt(today) + QStringLiteral(".json");

    const QStringList filters{QStringLiteral("*.json")};
    const auto entries = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo &info : entries) {
        // Never delete today's file
        if (info.fileName() == todayFileName)
            continue;

        QString name = info.fileName();
        if (!name.endsWith(QStringLiteral(".json")))
            continue;
        name.chop(5);
        const QDate d = QDate::fromString(name, QStringLiteral("yyyy-MM-dd"));
        if (!d.isValid())
            continue;

        if (d < start || d > end) {
            if (QFile::remove(info.filePath()))
                ++count;
        }
    }

    return count;
}
