#pragma once

#include <QMainWindow>

class NoteData;
class TaskData;
class QComboBox;
class QKeyEvent;
class QLabel;
class QSplitter;
class NoteWidget;
class TaskWidget;
class PomodoroWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onLanguageChanged(int index);
    void refreshTexts();

private:
    void setupUi();
    void toggleFullscreen();
    QString dataDirPath() const;

    TaskData *m_taskData;
    NoteData *m_noteData;

    // Top bar
    QLabel *m_titleLabel;
    QComboBox *m_langCombo;

    // Layout splitters
    QSplitter *m_mainSplitter;
    QSplitter *m_leftSplitter;

    // Feature widgets
    PomodoroWidget *m_pomodoroWidget;
    TaskWidget *m_taskWidget;
    NoteWidget *m_noteWidget;

    bool m_fullscreen = false;
};
