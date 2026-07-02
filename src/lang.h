#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

class LocaleManager : public QObject
{
    Q_OBJECT

public:
    static LocaleManager *instance();
    static void initialize(const QString &dataDir);
    static void destroy();

    QString currentLang() const;
    void setLanguage(const QString &lang);
    int taskFontSize() const;
    void setTaskFontSize(int size);
    QStringList languages() const;
    QString langName(const QString &lang) const;

    // Translate key. Falls back to key if no translation found.
    QString tr(const QString &key) const;

signals:
    void languageChanged();
    void taskFontSizeChanged(int size);

private:
    explicit LocaleManager(const QString &dataDir);
    void loadSettings();
    void saveSettings();
    void initTranslations();

    QString m_settingsPath;
    QString m_currentLang;
    int m_taskFontSize = 16;

    // lang -> {displayName, nativeName}
    struct LangMeta {
        QString name;       // "English", "中文"
    };
    QMap<QString, LangMeta> m_langs;

    // key -> {lang -> translation}
    QMap<QString, QMap<QString, QString>> m_data;
};

// Convenience function
inline QString loc(const char *key)
{
    return LocaleManager::instance()->tr(QString::fromUtf8(key));
}
