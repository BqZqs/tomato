#include "notewidget.h"
#include "notedata.h"
#include "lang.h"
#include "theme.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QIntValidator>

// ============================================================================
// NotebookManageDialog — local helper dialog (not exposed in header)
// ============================================================================

namespace {

class NotebookManageDialog : public QDialog
{
public:
    explicit NotebookManageDialog(NoteData *data, QWidget *parent = nullptr)
        : QDialog(parent), m_data(data)
    {
        setWindowTitle(loc("Manage Notebooks"));
        setMinimumSize(420, 320);

        auto *root = new QVBoxLayout(this);
        root->setSpacing(12);
        root->setContentsMargins(20, 16, 20, 16);

        // --- Title ---
        auto *title = new QLabel(loc("Manage Notebooks"));
        title->setStyleSheet("font-size:16px;font-weight:bold;color:#1A1A2E");
        root->addWidget(title);

        // --- Notebook list (scrollable) ---
        m_listLayout = new QVBoxLayout;
        m_listLayout->setSpacing(6);
        m_listContainer = new QWidget;
        m_listContainer->setLayout(m_listLayout);

        auto *scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setWidget(m_listContainer);
        scroll->setStyleSheet("QScrollArea { border: 1px solid #D1D5DB; border-radius: 6px; background: #F0F2F5; }");
        root->addWidget(scroll, 1);

        // --- Separator ---
        auto *sep = new QLabel;
        sep->setFixedHeight(1);
        sep->setStyleSheet("background:#D1D5DB");
        root->addWidget(sep);

        // --- Create section ---
        auto *createTitle = new QLabel(loc("Create New Notebook"));
        createTitle->setStyleSheet("font-weight:bold;color:#1A1A2E");
        root->addWidget(createTitle);

        auto *createRow = new QHBoxLayout;
        createRow->setSpacing(8);
        m_nameEdit = new QLineEdit;
        m_nameEdit->setPlaceholderText("Notebook name...");
        m_nameEdit->setMinimumHeight(32);
        m_createBtn = new QPushButton(loc("Create"));
        m_createBtn->setMinimumHeight(32);
        m_createBtn->setStyleSheet(
            "QPushButton { background:#22C55E;color:white;border:none;border-radius:4px;padding:0 16px;font-weight:bold; }"
            "QPushButton:hover { background:#16A34A; }"
            "QPushButton:disabled { background:#B0B3BF; }");
        createRow->addWidget(m_nameEdit, 1);
        createRow->addWidget(m_createBtn);
        root->addLayout(createRow);

        m_limitLabel = new QLabel;
        m_limitLabel->setStyleSheet("color:#EF4444;font-size:12px");
        m_limitLabel->setVisible(false);
        root->addWidget(m_limitLabel);

        m_errorLabel = new QLabel;
        m_errorLabel->setStyleSheet("color:#EF4444;font-size:12px");
        m_errorLabel->setVisible(false);
        root->addWidget(m_errorLabel);

        // --- Close button ---
        auto *closeBtn = new QPushButton(loc("Close"));
        closeBtn->setMinimumHeight(36);
        closeBtn->setStyleSheet(
            "QPushButton { background:#9CA3AF;color:white;border:none;border-radius:4px;padding:0 16px; }"
            "QPushButton:hover { background:#7B7D8C; }");
        root->addWidget(closeBtn);

        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(m_createBtn, &QPushButton::clicked, this, &NotebookManageDialog::onCreate);
        connect(m_nameEdit, &QLineEdit::returnPressed, this, &NotebookManageDialog::onCreate);

        rebuildList();
        updateCreateState();
    }

private slots:
    void onDelete(const QString &name)
    {
        auto answer = QMessageBox::question(
            this, loc("Confirm Delete"),
            loc("Delete notebook '%1' and ALL its notes?").arg(name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (answer == QMessageBox::Yes) {
            m_data->deleteNotebook(name);
            rebuildList();
            updateCreateState();
        }
    }

    void onCreate()
    {
        m_errorLabel->setVisible(false);
        const QString name = m_nameEdit->text().trimmed();

        if (name.isEmpty()) return;

        if (!NoteData::isValidNotebookName(name)) {
            m_errorLabel->setText(loc("Invalid notebook name. Use letters, numbers, spaces, hyphens, or underscores."));
            m_errorLabel->setVisible(true);
            return;
        }

        if (m_data->hasNotebook(name)) {
            m_errorLabel->setText(loc("A notebook with this name already exists."));
            m_errorLabel->setVisible(true);
            return;
        }

        if (!m_data->createNotebook(name)) {
            m_errorLabel->setText(loc("Failed to create notebook."));
            m_errorLabel->setVisible(true);
            return;
        }

        m_nameEdit->clear();
        rebuildList();
        updateCreateState();
    }

private:
    void rebuildList()
    {
        // Clear existing items
        QLayoutItem *child;
        while ((child = m_listLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        const QStringList notebooks = m_data->listNotebooks();

        if (notebooks.isEmpty()) {
            auto *emptyLabel = new QLabel(loc("No notebooks."));
            emptyLabel->setStyleSheet("color:#B0B3BF;font-style:italic;padding:8px");
            m_listLayout->addWidget(emptyLabel);
        } else {
            for (const auto &name : notebooks) {
                auto *row = new QHBoxLayout;
                row->setSpacing(8);

                auto *nameLabel = new QLabel(name);
                nameLabel->setStyleSheet("font-size:13px;color:#1A1A2E;padding:4px 0");
                row->addWidget(nameLabel, 1);

                auto *delBtn = new QPushButton(loc("Delete"));
                delBtn->setFixedSize(64, 28);
                delBtn->setStyleSheet(
                    "QPushButton { background:#EF4444;color:white;border:none;border-radius:3px;font-size:11px; }"
                    "QPushButton:hover { background:#DC2626; }");
                connect(delBtn, &QPushButton::clicked, this, [this, name]() { onDelete(name); });
                row->addWidget(delBtn);

                m_listLayout->addLayout(row);
            }
        }

        m_listLayout->addStretch();
    }

    void updateCreateState()
    {
        const int count = m_data->listNotebooks().size();
        const bool atLimit = count >= NoteData::MAX_NOTEBOOKS;

        m_nameEdit->setEnabled(!atLimit);
        m_createBtn->setEnabled(!atLimit);

        if (atLimit) {
            m_nameEdit->setPlaceholderText("Max notebooks reached");
            m_limitLabel->setText(loc("Max %1 notebooks").arg(NoteData::MAX_NOTEBOOKS));
            m_limitLabel->setVisible(true);
        } else {
            m_nameEdit->setPlaceholderText("Notebook name...");
            m_limitLabel->setVisible(false);
        }
    }

    NoteData *m_data;
    QVBoxLayout *m_listLayout = nullptr;
    QWidget *m_listContainer = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QPushButton *m_createBtn = nullptr;
    QLabel *m_limitLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
};

} // anonymous namespace

// ============================================================================
// NoteWidget
// ============================================================================

NoteWidget::NoteWidget(NoteData *data, QWidget *parent)
    : QWidget(parent), m_data(data), m_currentDate(QDate::currentDate())
{
    setupUi();
    refreshNotebooks();

    if (!m_currentNotebook.isEmpty())
        openMostRecentNote();

    connect(LocaleManager::instance(), &LocaleManager::languageChanged,
            this, &NoteWidget::refreshTexts);
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void NoteWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(24, 16, 24, 16);

    // ================================================================
    // Stacked widget: page 0 = no-notebooks, page 1 = editor
    // ================================================================
    m_stack = new QStackedWidget;
    root->addWidget(m_stack);

    // --- Page 0: No notebooks ---
    m_noNotebooksPage = new QWidget;
    auto *noNBLay = new QVBoxLayout(m_noNotebooksPage);
    noNBLay->setSpacing(16);
    noNBLay->setContentsMargins(0, 0, 0, 0);
    noNBLay->addStretch();

    m_noNBLabel = new QLabel(loc("No notebooks yet.\nCreate one to start."));
    m_noNBLabel->setAlignment(Qt::AlignCenter);
    m_noNBLabel->setStyleSheet("font-size:15px;color:#7B7D8C;padding:16px");
    m_noNBLabel->setWordWrap(true);
    noNBLay->addWidget(m_noNBLabel);

    m_createFirstBtn = new QPushButton(loc("Create First Notebook"));
    m_createFirstBtn->setMinimumHeight(40);
    m_createFirstBtn->setStyleSheet(
        "QPushButton { background:#22C55E;color:white;border:none;border-radius:6px;"
        "font-size:14px;font-weight:bold;padding:0 24px; }"
        "QPushButton:hover { background:#16A34A; }");
    m_createFirstBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    noNBLay->addWidget(m_createFirstBtn, 0, Qt::AlignCenter);
    noNBLay->addStretch();

    connect(m_createFirstBtn, &QPushButton::clicked, this, &NoteWidget::onManageNotebooks);
    m_stack->addWidget(m_noNotebooksPage); // index 0

    // --- Page 1: Editor ---
    m_editorPage = new QWidget;
    auto *editorLay = new QVBoxLayout(m_editorPage);
    editorLay->setSpacing(12);
    editorLay->setContentsMargins(0, 0, 0, 0);

    // Row 1: Notebook selector
    auto *nbBar = new QHBoxLayout;
    nbBar->setSpacing(8);

    m_nbLabel = new QLabel(loc("Notebook:"));
    m_nbLabel->setStyleSheet("font-size:13px;color:#1A1A2E;font-weight:bold");
    nbBar->addWidget(m_nbLabel);

    m_notebookCombo = new QComboBox;
    m_notebookCombo->setMinimumHeight(30);
    m_notebookCombo->setStyleSheet(
        "QComboBox { border:1px solid #D1D5DB;border-radius:4px;padding:4px 8px;background:#FFFFFF;color:#1A1A2E; }"
        "QComboBox:hover { border-color:#9CA3AF; }"
        "QComboBox QAbstractItemView { background:#FFFFFF;color:#1A1A2E;selection-background-color:#F3F4F6; }");
    nbBar->addWidget(m_notebookCombo, 1);

    m_manageBtn = new QPushButton(loc("Manage..."));
    m_manageBtn->setMinimumHeight(30);
    m_manageBtn->setStyleSheet(
        "QPushButton { background:transparent;color:#7B7D8C;border:1px solid #D1D5DB;"
        "border-radius:4px;padding:0 12px;font-size:12px; }"
        "QPushButton:hover { color:#1A1A2E;border-color:#9CA3AF; }");
    nbBar->addWidget(m_manageBtn);

    m_saveBtn = new QPushButton(QStringLiteral("\U0001F4BE"));
    m_saveBtn->setFixedSize(26, 26);
    m_saveBtn->setStyleSheet(Theme::kIconBtnStyleSheet);
    m_saveBtn->setToolTip(loc("Save"));
    nbBar->addWidget(m_saveBtn);

    m_deleteBtn = new QPushButton(QStringLiteral("\U0001F5D1"));
    m_deleteBtn->setFixedSize(26, 26);
    m_deleteBtn->setStyleSheet(Theme::kIconBtnStyleSheet);
    m_deleteBtn->setToolTip(loc("Delete This Note"));
    nbBar->addWidget(m_deleteBtn);

    auto *noteFontCombo = new QComboBox;
    noteFontCombo->setEditable(true);
    noteFontCombo->setInsertPolicy(QComboBox::NoInsert);
    noteFontCombo->setValidator(new QIntValidator(-4, 16, noteFontCombo));
    noteFontCombo->addItems({QStringLiteral("-4"), QStringLiteral("-2"),
                              QStringLiteral("-1"), QStringLiteral("0"),
                              QStringLiteral("+1"), QStringLiteral("+2"),
                              QStringLiteral("+4"), QStringLiteral("+8"),
                              QStringLiteral("+12"), QStringLiteral("+16")});
    noteFontCombo->setCurrentText(
        QString::number(LocaleManager::instance()->noteFontOffset()));
    noteFontCombo->setMaximumWidth(72);
    noteFontCombo->setStyleSheet(QStringLiteral(
        "QComboBox { font-size:12px; padding:2px 4px; "
        "border:1px solid #D1D5DB; border-radius:6px; "
        "color:#7B7D8C; background:#F8F9FB; }"
        "QComboBox:hover { border-color:#9CA3AF; }"
        "QComboBox:focus { border-color:#6C63FF; }"
        "QComboBox::drop-down { width:20px; border:none; }"
        "QComboBox::down-arrow { image:none; border-left:4px solid transparent; border-right:4px solid transparent; border-top:5px solid #7B7D8C; margin-right:4px; }"));
    nbBar->addWidget(noteFontCombo);

    connect(noteFontCombo, &QComboBox::currentTextChanged, this, [](const QString &text) {
        bool ok;
        int val = text.toInt(&ok);
        if (ok) LocaleManager::instance()->setNoteFontOffset(val);
    });

    editorLay->addLayout(nbBar);

    // Row 2: Date navigation
    auto *dateBar = new QHBoxLayout;
    dateBar->setSpacing(6);

    m_prevBtn = new QPushButton(QString::fromUtf8("\xe2\x97\x80"));  // ◀
    m_prevBtn->setFixedSize(32, 30);
    m_prevBtn->setStyleSheet(
        "QPushButton { background:transparent;color:#7B7D8C;border:1px solid #D1D5DB;"
        "border-radius:4px;font-size:14px; }"
        "QPushButton:hover { color:#1A1A2E;border-color:#9CA3AF; }");
    dateBar->addWidget(m_prevBtn);

    m_dateEdit = new QDateEdit(m_currentDate);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setMinimumHeight(30);
    m_dateEdit->setStyleSheet(
        "QDateEdit { border:1px solid #D1D5DB;border-radius:4px;padding:4px 8px;"
        "background:#FFFFFF;color:#1A1A2E;font-size:13px;font-weight:bold; }"
        "QDateEdit:hover { border-color:#9CA3AF; }"
        "QDateEdit:focus { border-color:#6C63FF; }");
    dateBar->addWidget(m_dateEdit, 1);

    m_nextBtn = new QPushButton(QString::fromUtf8("\xe2\x96\xb6"));  // ▶
    m_nextBtn->setFixedSize(32, 30);
    m_nextBtn->setStyleSheet(m_prevBtn->styleSheet());
    dateBar->addWidget(m_nextBtn);

    m_todayBtn = new QPushButton(loc("Today"));
    m_todayBtn->setMinimumHeight(30);
    m_todayBtn->setStyleSheet(
        "QPushButton { background:transparent;color:#6C63FF;border:1px solid #6C63FF;"
        "border-radius:4px;padding:0 10px;font-size:12px;font-weight:bold; }"
        "QPushButton:hover { background:#6C63FF;color:white; }");
    dateBar->addWidget(m_todayBtn);

    m_jumpPrevBtn = new QPushButton(QStringLiteral("|\u25C0"));
    m_jumpPrevBtn->setFixedSize(32, 30);
    m_jumpPrevBtn->setToolTip(loc("Jump to previous note"));
    m_jumpPrevBtn->setStyleSheet(
        "QPushButton { background:transparent;color:#7B7D8C;border:1px solid #D1D5DB;"
        "border-radius:4px;font-size:12px; }"
        "QPushButton:hover { color:#6C63FF;border-color:#6C63FF; }");
    dateBar->addWidget(m_jumpPrevBtn);

    m_jumpNextBtn = new QPushButton(QStringLiteral("\u25B6|"));
    m_jumpNextBtn->setFixedSize(32, 30);
    m_jumpNextBtn->setToolTip(loc("Jump to next note"));
    m_jumpNextBtn->setStyleSheet(m_jumpPrevBtn->styleSheet());
    dateBar->addWidget(m_jumpNextBtn);

    m_pageLabel = new QLabel;
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_pageLabel->setMinimumWidth(40);
    m_pageLabel->setStyleSheet("font-size:11px; color:#7B7D8C;");
    dateBar->addWidget(m_pageLabel);

    m_firstBtn = new QPushButton(QStringLiteral("\u23EE"));
    m_firstBtn->setFixedSize(30, 30);
    m_firstBtn->setToolTip(loc("First page"));
    m_firstBtn->setStyleSheet(
        "QPushButton { background:transparent;color:#7B7D8C;border:1px solid #D1D5DB;"
        "border-radius:4px;font-size:12px; }"
        "QPushButton:hover { color:#6C63FF;border-color:#6C63FF; }");
    dateBar->addWidget(m_firstBtn);

    m_lastBtn = new QPushButton(QStringLiteral("\u23ED"));
    m_lastBtn->setFixedSize(30, 30);
    m_lastBtn->setStyleSheet(
        "QPushButton { background:transparent;color:#7B7D8C;border:1px solid #D1D5DB;"
        "border-radius:4px;font-size:12px; }"
        "QPushButton:hover { color:#6C63FF;border-color:#6C63FF; }");
    dateBar->addWidget(m_lastBtn);

    editorLay->addLayout(dateBar);

    // Row 3: Markdown editor
    m_editor = new QPlainTextEdit;
    m_editor->setPlaceholderText(loc("Start writing..."));
    m_editor->setStyleSheet(
        "QPlainTextEdit { border:1px solid #D1D5DB;border-radius:6px;padding:12px;"
        "background:#FFFFFF;color:#1A1A2E;font-family:Consolas,monospace;font-size:13px;"
        "selection-background-color:#EEF2FF; }"
        "QPlainTextEdit:focus { border-color:#6C63FF; }");
    m_editor->setTabStopDistance(32);

    auto updateEditorFont = [this](int offset) {
        int px = qMax(8, 13 + offset);
        m_editor->setStyleSheet(
            QString("QPlainTextEdit { border:1px solid #D1D5DB;border-radius:6px;padding:12px;"
                    "background:#FFFFFF;color:#1A1A2E;font-family:Consolas,monospace;"
                    "font-size:%1px;selection-background-color:#EEF2FF; }"
                    "QPlainTextEdit:focus { border-color:#6C63FF; }").arg(px));
    };
    updateEditorFont(LocaleManager::instance()->noteFontOffset());
    connect(LocaleManager::instance(), &LocaleManager::noteFontOffsetChanged,
            this, updateEditorFont);

    editorLay->addWidget(m_editor, 1);

    m_stack->addWidget(m_editorPage); // index 1

    // ================================================================
    // Connections
    // ================================================================
    connect(m_notebookCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NoteWidget::onNotebookChanged);
    connect(m_manageBtn, &QPushButton::clicked, this, &NoteWidget::onManageNotebooks);

    connect(m_prevBtn, &QPushButton::clicked, this, &NoteWidget::onPrevDay);
    connect(m_nextBtn, &QPushButton::clicked, this, &NoteWidget::onNextDay);
    connect(m_todayBtn, &QPushButton::clicked, this, &NoteWidget::onToday);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &NoteWidget::onDateEditChanged);

    connect(m_editor, &QPlainTextEdit::textChanged, this, &NoteWidget::onTextChanged);
    connect(m_saveBtn, &QPushButton::clicked, this, &NoteWidget::onSave);
    connect(m_deleteBtn, &QPushButton::clicked, this, &NoteWidget::onDeleteNote);

    connect(m_jumpPrevBtn, &QPushButton::clicked, this, &NoteWidget::onJumpPrev);
    connect(m_jumpNextBtn, &QPushButton::clicked, this, &NoteWidget::onJumpNext);
    connect(m_firstBtn, &QPushButton::clicked, this, &NoteWidget::onFirst);
    connect(m_lastBtn, &QPushButton::clicked, this, &NoteWidget::onLast);
}

// ---------------------------------------------------------------------------
// Notebook management
// ---------------------------------------------------------------------------

void NoteWidget::refreshNotebooks()
{
    const QStringList notebooks = m_data->listNotebooks();

    if (notebooks.isEmpty()) {
        m_stack->setCurrentIndex(0);
        m_currentNotebook.clear();
        return;
    }

    // Remember current selection
    const QString previous = m_currentNotebook;

    // Block signals while repopulating
    m_notebookCombo->blockSignals(true);
    m_notebookCombo->clear();

    int selectIndex = 0;
    for (int i = 0; i < notebooks.size(); ++i) {
        m_notebookCombo->addItem(notebooks.at(i));
        if (notebooks.at(i) == previous)
            selectIndex = i;
    }

    m_notebookCombo->setCurrentIndex(selectIndex);
    m_notebookCombo->blockSignals(false);

    m_currentNotebook = m_notebookCombo->currentText();
    m_stack->setCurrentIndex(1);

    // If the combo index didn't change (e.g. same notebook still selected),
    // the currentIndexChanged signal didn't fire, so load explicitly.
    // But if it did change (e.g. different index), the signal handles it.
    // We only need to force-load when the notebook didn't actually change
    // and we rebuilt from scratch (e.g. after dialog close).
    if (m_currentNotebook == previous && !previous.isEmpty()) {
        loadNote();
    }
}

// ---------------------------------------------------------------------------
// Date navigation
// ---------------------------------------------------------------------------

void NoteWidget::setDate(const QDate &date)
{
    if (!date.isValid()) return;

    saveCurrentNoteIfDirty();
    m_currentDate = date;
    m_dateEdit->blockSignals(true);
    m_dateEdit->setDate(date);
    m_dateEdit->blockSignals(false);
    loadNote();
}

void NoteWidget::onPrevDay()
{
    setDate(m_currentDate.addDays(-1));
}

void NoteWidget::onNextDay()
{
    setDate(m_currentDate.addDays(1));
}

void NoteWidget::onToday()
{
    setDate(QDate::currentDate());
}

void NoteWidget::onDateEditChanged(const QDate &date)
{
    if (date == m_currentDate) return;
    saveCurrentNoteIfDirty();
    m_currentDate = date;
    loadNote();
}

// ---------------------------------------------------------------------------
// Notebook selector slot
// ---------------------------------------------------------------------------

void NoteWidget::onNotebookChanged(int /*index*/)
{
    const QString newNotebook = m_notebookCombo->currentText();
    if (newNotebook.isEmpty() || newNotebook == m_currentNotebook) return;

    saveCurrentNoteIfDirty();
    m_currentNotebook = newNotebook;
    loadNote();
}

// ---------------------------------------------------------------------------
// Note loading / saving
// ---------------------------------------------------------------------------

void NoteWidget::loadNote()
{
    if (m_currentNotebook.isEmpty()) return;

    const QString content = m_data->loadNote(m_currentNotebook, m_currentDate);

    m_editor->blockSignals(true);
    m_editor->setPlainText(content);
    m_editor->blockSignals(false);

    m_lastSavedText = content;
    m_dirty = false;
    updateDirtyState();

    // Placeholder if empty
    m_editor->setPlaceholderText(content.isEmpty() ? loc("Start writing...") : "");
    updatePageLabel();
}

void NoteWidget::saveCurrentNoteIfDirty()
{
    if (!m_dirty || m_currentNotebook.isEmpty()) return;

    const QString content = m_editor->toPlainText();
    if (content.trimmed().isEmpty()) {
        if (m_data->hasNote(m_currentNotebook, m_currentDate))
            m_data->deleteNote(m_currentNotebook, m_currentDate);
    } else {
        m_data->saveNote(m_currentNotebook, m_currentDate, content);
    }
    m_lastSavedText = content;
    m_dirty = false;
    updateDirtyState();
}

void NoteWidget::onSave()
{
    if (m_currentNotebook.isEmpty()) return;

    const QString content = m_editor->toPlainText();
    if (content.trimmed().isEmpty()) {
        if (m_data->hasNote(m_currentNotebook, m_currentDate))
            m_data->deleteNote(m_currentNotebook, m_currentDate);
    } else {
        m_data->saveNote(m_currentNotebook, m_currentDate, content);
    }
    m_lastSavedText = content;
    m_dirty = false;
    updateDirtyState();
}

void NoteWidget::onDeleteNote()
{
    if (m_currentNotebook.isEmpty()) return;

    auto answer = QMessageBox::question(
        this, loc("Delete Note"),
        loc("Delete note for %1?").arg(m_currentDate.toString("yyyy-MM-dd")),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer != QMessageBox::Yes) return;

    m_data->deleteNote(m_currentNotebook, m_currentDate);

    m_editor->blockSignals(true);
    m_editor->clear();
    m_editor->blockSignals(false);

    m_editor->setPlaceholderText(loc("Start writing..."));
    m_lastSavedText.clear();
    m_dirty = false;
    updateDirtyState();
}

// ---------------------------------------------------------------------------
// Dirty tracking
// ---------------------------------------------------------------------------

void NoteWidget::onTextChanged()
{
    const QString current = m_editor->toPlainText();
    const bool wasDirty = m_dirty;
    m_dirty = (current != m_lastSavedText);

    if (m_dirty != wasDirty)
        updateDirtyState();
}

void NoteWidget::updateDirtyState()
{
    m_saveBtn->setEnabled(m_dirty);
}

// ---------------------------------------------------------------------------
// Manage button → opens dialog, then refreshes
// ---------------------------------------------------------------------------

void NoteWidget::onManageNotebooks()
{
    NotebookManageDialog dlg(m_data, this);
    dlg.exec();
    refreshNotebooks();
}

// ---------------------------------------------------------------------------
// Language / translation refresh
// ---------------------------------------------------------------------------

void NoteWidget::refreshTexts()
{
    m_nbLabel->setText(loc("Notebook:"));
    m_manageBtn->setText(loc("Manage..."));
    m_todayBtn->setText(loc("Today"));
    m_saveBtn->setToolTip(loc("Save"));
    m_deleteBtn->setToolTip(loc("Delete This Note"));
    m_jumpPrevBtn->setToolTip(loc("Jump to previous note"));
    m_jumpNextBtn->setToolTip(loc("Jump to next note"));
    m_firstBtn->setToolTip(loc("First page"));
    m_noNBLabel->setText(loc("No notebooks yet.\nCreate one to start."));
    m_createFirstBtn->setText(loc("Create First Notebook"));
    updateDirtyState();
    loadNote();
}

void NoteWidget::onJumpPrev() {
    if (m_currentNotebook.isEmpty()) return;
    const QList<QDate> dates = m_data->noteDates(m_currentNotebook);
    if (dates.isEmpty()) return;
    for (int i = dates.size() - 1; i >= 0; --i) {
        if (dates[i] < m_currentDate) { setDate(dates[i]); return; }
    }
}
void NoteWidget::onJumpNext() {
    if (m_currentNotebook.isEmpty()) return;
    const QList<QDate> dates = m_data->noteDates(m_currentNotebook);
    if (dates.isEmpty()) return;
    for (const QDate &d : dates) {
        if (d > m_currentDate) { setDate(d); return; }
    }
}
void NoteWidget::onFirst() {
    if (m_currentNotebook.isEmpty()) return;
    const QList<QDate> dates = m_data->noteDates(m_currentNotebook);
    if (dates.isEmpty()) return;
    setDate(dates.first());
}
void NoteWidget::onLast() {
    if (m_currentNotebook.isEmpty()) return;
    const QList<QDate> dates = m_data->noteDates(m_currentNotebook);
    if (dates.isEmpty()) return;
    setDate(dates.last());
}
void NoteWidget::updatePageLabel() {
    if (m_currentNotebook.isEmpty()) { m_pageLabel->clear(); return; }
    const QList<QDate> dates = m_data->noteDates(m_currentNotebook);
    int idx = dates.indexOf(m_currentDate);
    if (idx >= 0)
        m_pageLabel->setText(QStringLiteral("%1 / %2").arg(idx + 1).arg(dates.size()));
    else
        m_pageLabel->setText(QStringLiteral("- / %1").arg(dates.size()));
}

void NoteWidget::openMostRecentNote() {
    if (m_currentNotebook.isEmpty()) return;
    const QList<QDate> dates = m_data->noteDates(m_currentNotebook);
    if (!dates.isEmpty()) {
        m_currentDate = dates.last();
        m_dateEdit->blockSignals(true);
        m_dateEdit->setDate(m_currentDate);
        m_dateEdit->blockSignals(false);
    }
    loadNote();
}
