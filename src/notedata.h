#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>

class NoteData {
public:
    explicit NoteData(const QString &dataDir);

    // Notebook management
    QStringList listNotebooks() const;
    bool createNotebook(const QString &name) const;
    bool deleteNotebook(const QString &name) const;
    bool hasNotebook(const QString &name) const;

    static bool isValidNotebookName(const QString &name);
    static constexpr int MAX_NOTEBOOKS = 5;

    // Note CRUD
    QString loadNote(const QString &notebook, QDate date) const;
    bool saveNote(const QString &notebook, QDate date, const QString &content) const;
    bool hasNote(const QString &notebook, QDate date) const;
    bool deleteNote(const QString &notebook, QDate date) const;
    QList<QDate> noteDates(const QString &notebook) const;

    // Path helpers
    QString notesDir() const;
    QString notebookDir(const QString &notebook) const;
    QString notePath(const QString &notebook, QDate date) const;

private:
    static QString fmt(QDate d);
    QString m_dataDir;
};
