#include "notedata.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

NoteData::NoteData(const QString &dataDir)
    : m_dataDir(dataDir)
{
    QDir().mkpath(notesDir());
}

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

QString NoteData::notesDir() const
{
    return m_dataDir + QStringLiteral("/notes");
}

QString NoteData::notebookDir(const QString &notebook) const
{
    return notesDir() + QStringLiteral("/") + notebook;
}

QString NoteData::notePath(const QString &notebook, QDate date) const
{
    return notebookDir(notebook) + QStringLiteral("/") + fmt(date) + QStringLiteral(".md");
}

QString NoteData::fmt(QDate d)
{
    return d.toString(QStringLiteral("yyyy-MM-dd"));
}

// ---------------------------------------------------------------------------
// Notebook management
// ---------------------------------------------------------------------------

QStringList NoteData::listNotebooks() const
{
    QDir dir(notesDir());
    QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    return names;
}

bool NoteData::createNotebook(const QString &name) const
{
    if (!isValidNotebookName(name))
        return false;

    if (listNotebooks().size() >= MAX_NOTEBOOKS)
        return false;

    // If notebook already exists, refuse to overwrite
    if (hasNotebook(name))
        return false;

    return QDir().mkpath(notebookDir(name));
}

bool NoteData::deleteNotebook(const QString &name) const
{
    return QDir(notebookDir(name)).removeRecursively();
}

bool NoteData::hasNotebook(const QString &name) const
{
    return QDir(notebookDir(name)).exists();
}

bool NoteData::isValidNotebookName(const QString &name)
{
    if (name.trimmed().isEmpty())
        return false;

    // Reject characters that break filesystems
    for (const QChar &c : name) {
        if (c == QLatin1Char('/')  || c == QLatin1Char('\\') ||
            c == QLatin1Char(':')  || c == QLatin1Char('*')  ||
            c == QLatin1Char('?')  || c == QLatin1Char('"')  ||
            c == QLatin1Char('<')  || c == QLatin1Char('>')  ||
            c == QLatin1Char('|')  || c == QLatin1Char('.')) {
            return false;
        }
    }

    // Must contain at least one non-whitespace character (already checked by trimmed above)
    return true;
}

// ---------------------------------------------------------------------------
// Note CRUD
// ---------------------------------------------------------------------------

QString NoteData::loadNote(const QString &notebook, QDate date) const
{
    QFile file(notePath(notebook, date));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    return QString::fromUtf8(file.readAll());
}

bool NoteData::saveNote(const QString &notebook, QDate date, const QString &content) const
{
    // Ensure notebook directory exists
    QDir().mkpath(notebookDir(notebook));

    QSaveFile file(notePath(notebook, date));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(content.toUtf8());

    if (!file.commit())
        return false;

    return true;
}

bool NoteData::hasNote(const QString &notebook, QDate date) const
{
    return QFileInfo::exists(notePath(notebook, date));
}

bool NoteData::deleteNote(const QString &notebook, QDate date) const
{
    return QFile::remove(notePath(notebook, date));
}

QList<QDate> NoteData::noteDates(const QString &notebook) const
{
    QDir dir(notebookDir(notebook));
    QStringList files = dir.entryList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);

    QList<QDate> dates;
    dates.reserve(files.size());

    for (const QString &f : files) {
        // Strip ".md" suffix to get "yyyy-MM-dd"
        QString dateStr = f.chopped(3);
        QDate d = QDate::fromString(dateStr, QStringLiteral("yyyy-MM-dd"));
        if (d.isValid())
            dates.append(d);
    }

    return dates;
}
