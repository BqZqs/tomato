#include "lang.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

static LocaleManager *s_instance = nullptr;

LocaleManager *LocaleManager::instance()
{
    return s_instance;
}

void LocaleManager::initialize(const QString &dataDir)
{
    if (!s_instance)
        s_instance = new LocaleManager(dataDir);
}

void LocaleManager::destroy()
{
    delete s_instance;
    s_instance = nullptr;
}

LocaleManager::LocaleManager(const QString &dataDir)
    : QObject(QCoreApplication::instance())
    , m_settingsPath(dataDir + QStringLiteral("/settings.json"))
    , m_currentLang(QStringLiteral("zh"))
{
    m_langs[QStringLiteral("zh")] = {QStringLiteral("\u4e2d\u6587")};       // 中文
    m_langs[QStringLiteral("en")] = {QStringLiteral("English")};

    initTranslations();
    loadSettings();
}

// ── Settings persistence ─────────────────────────────────────────────────

void LocaleManager::loadSettings()
{
    QFile f(m_settingsPath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    if (doc.isNull() || !doc.isObject())
        return;

    QString lang = doc.object().value(QStringLiteral("language")).toString();
    if (m_langs.contains(lang))
        m_currentLang = lang;

    m_taskFontSize = doc.object().value(QStringLiteral("taskFontSize")).toInt(16);
    if (m_taskFontSize < 8) m_taskFontSize = 8;
    if (m_taskFontSize > 48) m_taskFontSize = 48;

    m_defaultDuration = doc.object().value(QStringLiteral("defaultDuration")).toInt(25);
    if (m_defaultDuration < 1) m_defaultDuration = 1;
    if (m_defaultDuration > 120) m_defaultDuration = 120;
}

void LocaleManager::saveSettings()
{
    QDir().mkpath(QFileInfo(m_settingsPath).absolutePath());

    QJsonObject obj;
    obj[QStringLiteral("language")] = m_currentLang;
    obj[QStringLiteral("taskFontSize")] = m_taskFontSize;
    obj[QStringLiteral("defaultDuration")] = m_defaultDuration;

    QSaveFile f(m_settingsPath);
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(obj).toJson());
    f.commit();
}

// ── Public API ────────────────────────────────────────────────────────────

QString LocaleManager::currentLang() const
{
    return m_currentLang;
}

void LocaleManager::setLanguage(const QString &lang)
{
    if (!m_langs.contains(lang) || m_currentLang == lang)
        return;
    m_currentLang = lang;
    saveSettings();
    emit languageChanged();
}

int LocaleManager::taskFontSize() const
{
    return m_taskFontSize;
}

void LocaleManager::setTaskFontSize(int size)
{
    if (size < 8 || size > 48 || m_taskFontSize == size)
        return;
    m_taskFontSize = size;
    saveSettings();
    emit taskFontSizeChanged(size);
}

int LocaleManager::defaultDuration() const
{
    return m_defaultDuration;
}

void LocaleManager::setDefaultDuration(int min)
{
    if (min < 1 || min > 120 || m_defaultDuration == min) return;
    m_defaultDuration = min;
    saveSettings();
}

QStringList LocaleManager::languages() const
{
    return m_langs.keys();
}

QString LocaleManager::langName(const QString &lang) const
{
    return m_langs.value(lang).name;
}

QString LocaleManager::tr(const QString &key) const
{
    auto it = m_data.find(key);
    if (it == m_data.end())
        return key;
    auto t = it.value().find(m_currentLang);
    if (t == it.value().end())
        return key;
    return t.value();
}

// ── Translation data ──────────────────────────────────────────────────────

void LocaleManager::initTranslations()
{
    // Helper: adds a key with zh and en translations
    auto add = [this](const QString &key, const QString &zh, const QString &en) {
        m_data[key][QStringLiteral("zh")] = zh;
        m_data[key][QStringLiteral("en")] = en;
    };

    // ── mainwindow.cpp ────────────────────────────────────────────────
    add(QStringLiteral("Pomodoro"),       QStringLiteral("\u756A\u8304\u949F"),       QStringLiteral("Pomodoro"));        // 番茄钟
    add(QStringLiteral("Ready"),          QStringLiteral("\u51C6\u5907\u5C31\u7EEA"),  QStringLiteral("Ready"));           // 准备就绪
    add(QStringLiteral("min"),            QStringLiteral("\u5206\u949F"),              QStringLiteral("min"));             // 分钟
    add(QStringLiteral("Start"),          QStringLiteral("\u5F00\u59CB"),              QStringLiteral("Start"));           // 开始
    add(QStringLiteral("Pause"),          QStringLiteral("\u6682\u505C"),              QStringLiteral("Pause"));           // 暂停
    add(QStringLiteral("Resume"),         QStringLiteral("\u7EE7\u7EED"),              QStringLiteral("Resume"));          // 继续
    add(QStringLiteral("Reset"),          QStringLiteral("\u91CD\u7F6E"),              QStringLiteral("Reset"));           // 重置
    add(QStringLiteral("Sessions: %1"),   QStringLiteral("\u5B8C\u6210\u6B21\u6570: %1"), QStringLiteral("Sessions: %1")); // 完成次数
    add(QStringLiteral("Running..."),     QStringLiteral("\u8FD0\u884C\u4E2D..."),     QStringLiteral("Running..."));      // 运行中
    add(QStringLiteral("Paused"),         QStringLiteral("\u5DF2\u6682\u505C"),        QStringLiteral("Paused"));          // 已暂停
    add(QStringLiteral("Done!"),          QStringLiteral("\u5B8C\u6210\uFF01"),        QStringLiteral("Done!"));           // 完成！

    // ── taskwidget.cpp ────────────────────────────────────────────────
    add(QStringLiteral("Today"),          QStringLiteral("\u4ECA\u5929"),              QStringLiteral("Today"));           // 今天
    add(QStringLiteral("No task list for this date."), QStringLiteral("\u8BE5\u65E5\u671F\u6CA1\u6709\u4EFB\u52A1\u6E05\u5355\u3002"), QStringLiteral("No task list for this date.")); // 该日期没有任务清单。
    add(QStringLiteral("Create Empty List"), QStringLiteral("\u521B\u5EFA\u7A7A\u6E05\u5355"), QStringLiteral("Create Empty List")); // 创建空清单
    add(QStringLiteral("Inherit All from..."), QStringLiteral("\u4ECE...\u7EE7\u627F\u5168\u90E8"), QStringLiteral("Inherit All from...")); // 从...继承全部
    add(QStringLiteral("Inherit Incomplete from..."), QStringLiteral("\u4ECE...\u7EE7\u627F\u672A\u5B8C\u6210\u9879"), QStringLiteral("Inherit Incomplete from...")); // 从...继承未完成项
    add(QStringLiteral("%1 tasks, %2 done"), QStringLiteral("%1 \u9879\u4EFB\u52A1, %2 \u9879\u5DF2\u5B8C\u6210"), QStringLiteral("%1 tasks, %2 done")); // 项任务, 项已完成
    add(QStringLiteral("Add Task"),       QStringLiteral("\u6DFB\u52A0\u4EFB\u52A1"),  QStringLiteral("Add Task"));        // 添加任务
    add(QStringLiteral("Add Timed Task"), QStringLiteral("\u6DFB\u52A0\u8BA1\u65F6\u4EFB\u52A1"), QStringLiteral("Add Timed Task")); // 添加计时任务
    add(QStringLiteral("Timed Task"),     QStringLiteral("\u8BA1\u65F6\u4EFB\u52A1"),  QStringLiteral("Timed Task"));      // 计时任务
    add(QStringLiteral("Duration (minutes):"), QStringLiteral("\u65F6\u957F (\u5206\u949F):"), QStringLiteral("Duration (minutes):")); // 时长 (分钟)
    add(QStringLiteral("Edit Duration"), QStringLiteral("\u7F16\u8F91\u65F6\u957F"), QStringLiteral("Edit Duration")); // 编辑时长
    add(QStringLiteral("Click to change duration"), QStringLiteral("\u70B9\u51FB\u4FEE\u6539\u65F6\u957F"), QStringLiteral("Click to change duration")); // 点击修改时长
    add(QStringLiteral("Start timer for this task"), QStringLiteral("\u4E3A\u6B64\u4EFB\u52A1\u542F\u52A8\u8BA1\u65F6\u5668"), QStringLiteral("Start timer for this task")); // 为此任务启动计时器
    add(QStringLiteral("Edit Task"),      QStringLiteral("\u7F16\u8F91\u4EFB\u52A1"),  QStringLiteral("Edit Task"));       // 编辑任务
    add(QStringLiteral("Task Text:"),     QStringLiteral("\u4EFB\u52A1\u5185\u5BB9:"), QStringLiteral("Task Text:"));      // 任务内容
    add(QStringLiteral("Select date to inherit from:"), QStringLiteral("\u9009\u62E9\u8981\u7EE7\u627F\u7684\u65E5\u671F:"), QStringLiteral("Select date to inherit from:")); // 选择要继承的日期
    add(QStringLiteral("No other dates available to inherit from."), QStringLiteral("\u6CA1\u6709\u5176\u4ED6\u53EF\u7EE7\u627F\u7684\u65E5\u671F\u3002"), QStringLiteral("No other dates available to inherit from.")); // 没有其他可继承的日期。
    add(QStringLiteral("Cleanup Complete"), QStringLiteral("\u6E05\u7406\u5B8C\u6210"), QStringLiteral("Cleanup Complete")); // 清理完成
    add(QStringLiteral("Deleted %1 file(s) outside the 15-day window."), QStringLiteral("\u5DF2\u5220\u9664 %1 \u4E2A\u8D85\u51FA 15 \u5929\u7A97\u53E3\u7684\u6587\u4EF6\u3002"), QStringLiteral("Deleted %1 file(s) outside the 15-day window.")); // 已删除...文件
    add(QStringLiteral("Previous day"),   QStringLiteral("\u524D\u4E00\u5929"),        QStringLiteral("Previous day"));
    add(QStringLiteral("Next day"),       QStringLiteral("\u540E\u4E00\u5929"),        QStringLiteral("Next day"));
    add(QStringLiteral("Go to today"),    QStringLiteral("\u56DE\u5230\u4ECA\u5929"),  QStringLiteral("Go to today"));

    // ── notewidget.cpp ────────────────────────────────────────────────
    add(QStringLiteral("Notebook:"),      QStringLiteral("\u7B14\u8BB0\u672C\uFF1A"),  QStringLiteral("Notebook:"));       // 笔记本：
    add(QStringLiteral("Manage..."),      QStringLiteral("\u7BA1\u7406..."),            QStringLiteral("Manage..."));       // 管理...
    add(QStringLiteral("Save"),           QStringLiteral("\u4FDD\u5B58"),              QStringLiteral("Save"));            // 保存
    add(QStringLiteral("Delete This Note"), QStringLiteral("\u5220\u9664\u6B64\u7B14\u8BB0"), QStringLiteral("Delete This Note")); // 删除此笔记
    add(QStringLiteral("\u25CF Unsaved changes"), QStringLiteral("\u25CF \u672A\u4FDD\u5B58\u7684\u66F4\u6539"), QStringLiteral("\u25CF Unsaved changes")); // ● 未保存的更改
    add(QStringLiteral("No notebooks yet.\nCreate one to start."), QStringLiteral("\u6682\u65E0\u7B14\u8BB0\u672C\u3002\n\u521B\u5EFA\u4E00\u4E2A\u5F00\u59CB\u5427\u3002"), QStringLiteral("No notebooks yet.\nCreate one to start.")); // 暂无笔记本...
    add(QStringLiteral("Create First Notebook"), QStringLiteral("\u521B\u5EFA\u7B2C\u4E00\u4E2A\u7B14\u8BB0\u672C"), QStringLiteral("Create First Notebook")); // 创建第一个笔记本
    add(QStringLiteral("Manage Notebooks"), QStringLiteral("\u7BA1\u7406\u7B14\u8BB0\u672C"), QStringLiteral("Manage Notebooks")); // 管理笔记本
    add(QStringLiteral("Create New Notebook"), QStringLiteral("\u521B\u5EFA\u65B0\u7B14\u8BB0\u672C"), QStringLiteral("Create New Notebook")); // 创建新笔记本
    add(QStringLiteral("Create"),         QStringLiteral("\u521B\u5EFA"),              QStringLiteral("Create"));          // 创建
    add(QStringLiteral("Close"),          QStringLiteral("\u5173\u95ED"),              QStringLiteral("Close"));           // 关闭
    add(QStringLiteral("Delete"),         QStringLiteral("\u5220\u9664"),              QStringLiteral("Delete"));          // 删除
    add(QStringLiteral("No notebooks."),  QStringLiteral("\u6682\u65E0\u7B14\u8BB0\u672C\u3002"), QStringLiteral("No notebooks.")); // 暂无笔记本。
    add(QStringLiteral("Max %1 notebooks"), QStringLiteral("\u6700\u591A %1 \u672C\u7B14\u8BB0\u672C"), QStringLiteral("Max %1 notebooks")); // 最多...笔记本
    add(QStringLiteral("Delete notebook '%1' and ALL its notes?"), QStringLiteral("\u5220\u9664\u7B14\u8BB0\u672C '%1' \u53CA\u5176\u6240\u6709\u7B14\u8BB0\uFF1F"), QStringLiteral("Delete notebook '%1' and ALL its notes?")); // 删除笔记本...
    add(QStringLiteral("Invalid notebook name. Use letters, numbers, spaces, hyphens, or underscores."), QStringLiteral("\u65E0\u6548\u7684\u7B14\u8BB0\u672C\u540D\u79F0\u3002\u8BF7\u4F7F\u7528\u5B57\u6BCD\u3001\u6570\u5B57\u3001\u7A7A\u683C\u3001\u8FDE\u5B57\u7B26\u6216\u4E0B\u5212\u7EBF\u3002"), QStringLiteral("Invalid notebook name. Use letters, numbers, spaces, hyphens, or underscores.")); // 无效的笔记本名称...
    add(QStringLiteral("A notebook with this name already exists."), QStringLiteral("\u540C\u540D\u7B14\u8BB0\u672C\u5DF2\u5B58\u5728\u3002"), QStringLiteral("A notebook with this name already exists.")); // 同名笔记本已存在。
    add(QStringLiteral("Failed to create notebook."), QStringLiteral("\u521B\u5EFA\u7B14\u8BB0\u672C\u5931\u8D25\u3002"), QStringLiteral("Failed to create notebook.")); // 创建笔记本失败。
    add(QStringLiteral("Confirm Delete"), QStringLiteral("\u786E\u8BA4\u5220\u9664"),  QStringLiteral("Confirm Delete"));   // 确认删除
    add(QStringLiteral("Delete note for %1?"), QStringLiteral("\u5220\u9664 %1 \u7684\u7B14\u8BB0\uFF1F"), QStringLiteral("Delete note for %1?")); // 删除...的笔记？
    add(QStringLiteral("Start writing..."), QStringLiteral("\u5F00\u59CB\u5199\u4F5C..."), QStringLiteral("Start writing...")); // 开始写作...
    add(QStringLiteral("Language"),       QStringLiteral("\u8BED\u8A00"),              QStringLiteral("Language"));        // 语言
    add(QStringLiteral("Font Size:"),     QStringLiteral("\u5B57\u4F53\u5927\u5C0F:"),  QStringLiteral("Font Size:"));       // 字体大小
    add(QStringLiteral("Adjust task list font size"), QStringLiteral("\u8C03\u6574\u4EFB\u52A1\u5217\u8868\u5B57\u4F53\u5927\u5C0F"), QStringLiteral("Adjust task list font size"));
    add(QStringLiteral("Cleanup"),        QStringLiteral("\u6E05\u7406"),              QStringLiteral("Cleanup"));
    add(QStringLiteral("Inherit All"),    QStringLiteral("\u7EE7\u627F\u5168\u90E8"),  QStringLiteral("Inherit All"));
    add(QStringLiteral("Inherit Incomplete"), QStringLiteral("\u7EE7\u627F\u672A\u5B8C\u6210"), QStringLiteral("Inherit Incomplete"));
}
