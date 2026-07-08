#pragma once
#include <QWidget>

class DailyTaskData;
class DailyTaskRow;
class QListWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QSpinBox;

class DailyTaskWidget : public QWidget {
    Q_OBJECT
public:
    explicit DailyTaskWidget(DailyTaskData *data, QWidget *parent = nullptr);
public slots:
    void onTimedSessionFinished(const QString &taskId);
    void refreshTexts();
signals:
    void startTimerForTask(const QString &taskId, int minutes);
private slots:
    void onAddTimedTask();
    void onRowDelete(const QString &id);
    void onRowEditFinished(const QString &id, const QString &newText);
    void onRowEditCancelled(const QString &id);
    void onRowPlay(const QString &id, int minutes);
    void onRowDurationChanged(const QString &id, int newMinutes);
    void onWeekdayChanged(int index);
    void onThisWeek();
    void onPrevDay();
    void onNextDay();
    void onFontOffsetChanged(int offset);
private:
    void buildUi();
    void populateTaskList();
    void saveCurrentList();
    void applyFontSize();
    int currentWeekday() const;

    DailyTaskData *m_data;
    int m_weekday;
    int m_fontOffset = 0;

    QComboBox *m_weekdayCombo;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_thisWeekBtn;
    QListWidget *m_taskList;

    DailyTaskRow *findRowById(const QString &id) const;
};
