/****************************************************************
*                                                              *
*                         Apache 2.0                           *
*     Copyright Zynomon aelius <zynomon@proton.me>  2026       *
*               Project         :        Vex                   *
*               Version         :        4.2 (Cytoplasm)       *
****************************************************************/
#include <QObject>
#include <QtPlugin>
#include <QMainWindow>
#include <QFileIconProvider>
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QApplication>
#include <QTextStream>
#include <QCompleter>
#include <QFileSystemModel>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QPainter>
#include <QComboBox>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QProcess>
#include <QSaveFile>
#include <QFileSystemWatcher>
#include <QInputDialog>
#include <QDesktopServices>
#include <QTemporaryFile>
#include <QStringConverter>
#include <QPushButton>
#include <QTimer>
#include <QClipboard>
#include <QSize>
#include <QStackedWidget>
#include <QActionGroup>
#include <functional>
#include "Plugvex.H"
#include "Settings.H"


class VexEditor;
class LineNumberArea;
class VexWidget;

class Mode {
public:
    enum ModeEnum { MODE_INS, MODE_Vi, MODE_CMD };

    ModeEnum current() const { return m_mode; }

    void init(QPushButton *btn,
              std::function<void()>                      cbSaveReq,
              std::function<void(const QString &)>       cbVimKey,
              std::function<void(const QString &)>       cbCmdChanged,
              std::function<void(const QString &, bool)> cbCmdExecuted,
              std::function<void(ModeEnum)>              cbModeChanged,
              std::function<void(QKeyEvent *)>           cbDefaultKey)
    {
        m_btn           = btn;
        m_cbSaveReq     = cbSaveReq;
        m_cbVimKey      = cbVimKey;
        m_cbCmdChanged  = cbCmdChanged;
        m_cbCmdExecuted = cbCmdExecuted;
        m_cbModeChanged = cbModeChanged;
        m_cbDefaultKey  = cbDefaultKey;
    }

    void setupButton() {
        m_btn->setStyleSheet("");  // back to default
        loadMODE();
        switch (m_mode) {
        case MODE_INS: INS(); break;
        case MODE_Vi:  Vi();  break;
        case MODE_CMD: CMD(); break;
        }
    }

    void loadMODE() {
        QString saved = Settings::instance().get<QString>("mode", "INS");
        if      (saved == "Vi")  m_mode = MODE_Vi;
        else if (saved == "CMD") m_mode = MODE_CMD;
        else                     m_mode = MODE_INS;
    }

    void saveMODE() {
        switch (m_mode) {
        case MODE_INS: Settings::instance().setValue("mode", QString("INS")); break;
        case MODE_Vi:  Settings::instance().setValue("mode", QString("Vi"));  break;
        case MODE_CMD: Settings::instance().setValue("mode", QString("CMD")); break;
        }
    }

    void changeMODE() {
        switch (m_mode) {
        case MODE_INS: Vi();  break;
        case MODE_Vi:  CMD(); break;
        case MODE_CMD: INS(); break;
        }
    }

    void Vi() {
        m_mode = ModeEnum::MODE_Vi;
        saveMODE();
        m_btn->setText("VI");
        m_btn->setStyleSheet(
            "QPushButton { padding: 2px 10px; background-color: #2196F3;"
            " color: white; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #42a5f5; }");
        QObject::disconnect(m_btn, &QPushButton::clicked, nullptr, nullptr);
        QObject::connect(m_btn, &QPushButton::clicked, m_btn, [this]() { CMD(); });
        m_cbModeChanged(ModeEnum::MODE_Vi);
    }

    void INS() {
        m_mode = ModeEnum::MODE_INS;
        saveMODE();
        m_btn->setText("INSERT");
        m_btn->setStyleSheet(
            "QPushButton { padding: 2px 10px; background-color: #2a5a2a;"
            " color: #d0f0d0; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #3a7a3a; }");
        QObject::disconnect(m_btn, &QPushButton::clicked, nullptr, nullptr);
        QObject::connect(m_btn, &QPushButton::clicked, m_btn, [this]() { Vi(); });
        m_cbModeChanged(ModeEnum::MODE_INS);
    }

    void CMD() {
        m_mode = ModeEnum::MODE_CMD;
        m_cmdLine = ":";
        m_cbCmdChanged(m_cmdLine);
        m_btn->setText("COMMAND");
        m_btn->setStyleSheet(
            "QPushButton { padding: 2px 10px; background-color: #FF9800;"
            " color: white; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #ffb74d; }");
        QObject::disconnect(m_btn, &QPushButton::clicked, nullptr, nullptr);
        QObject::connect(m_btn, &QPushButton::clicked, m_btn, [this]() { INS(); });
        m_cbModeChanged(ModeEnum::MODE_CMD);
    }

    void handleKey(QPlainTextEdit *ed, QKeyEvent *e) {
        if (e->key() == Qt::Key_Escape) {
            changeMODE();
            return;
        }
        switch (m_mode) {
        case ModeEnum::MODE_INS:
            m_cbDefaultKey(e);
            break;
        case ModeEnum::MODE_Vi:
            handleViKey(ed, e);
            break;
        case ModeEnum::MODE_CMD:
            handleCmdKey(e);
            break;
        }
    }

private:
    ModeEnum    m_mode  = MODE_INS;
    QPushButton *m_btn  = nullptr;
    QString     m_cmdLine;

    std::function<void()>                      m_cbSaveReq;
    std::function<void(const QString &)>       m_cbVimKey;
    std::function<void(const QString &)>       m_cbCmdChanged;
    std::function<void(const QString &, bool)> m_cbCmdExecuted;
    std::function<void(ModeEnum)>              m_cbModeChanged;
    std::function<void(QKeyEvent *)>           m_cbDefaultKey;

    void handleViKey(QPlainTextEdit *ed, QKeyEvent *e) {
        QTextCursor cursor = ed->textCursor();

        if (e->key() == Qt::Key_I) {
            INS();
            m_cbVimKey("i");
            return;
        }
        if (e->key() == Qt::Key_A) {
            INS();
            cursor.movePosition(QTextCursor::Right);
            ed->setTextCursor(cursor);
            m_cbVimKey("a");
            return;
        }
        if (e->key() == Qt::Key_Colon) {
            CMD();
            return;
        }
        if (e->key() == Qt::Key_H) {
            cursor.movePosition(QTextCursor::Left);
            ed->setTextCursor(cursor);
            m_cbVimKey("h");
            return;
        }
        if (e->key() == Qt::Key_J) {
            cursor.movePosition(QTextCursor::Down);
            ed->setTextCursor(cursor);
            m_cbVimKey("j");
            return;
        }
        if (e->key() == Qt::Key_K) {
            cursor.movePosition(QTextCursor::Up);
            ed->setTextCursor(cursor);
            m_cbVimKey("k");
            return;
        }
        if (e->key() == Qt::Key_L) {
            cursor.movePosition(QTextCursor::Right);
            ed->setTextCursor(cursor);
            m_cbVimKey("l");
            return;
        }
        if (e->key() == Qt::Key_W && !(e->modifiers() & Qt::ControlModifier)) {
            cursor.movePosition(QTextCursor::NextWord);
            ed->setTextCursor(cursor);
            m_cbVimKey("w");
            return;
        }
        if (e->key() == Qt::Key_B) {
            cursor.movePosition(QTextCursor::PreviousWord);
            ed->setTextCursor(cursor);
            m_cbVimKey("b");
            return;
        }
        if (e->key() == Qt::Key_X) {
            cursor.deleteChar();
            m_cbVimKey("x");
            return;
        }
        if (e->key() == Qt::Key_D) {
            if (e->modifiers() & Qt::ShiftModifier) {
                cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                m_cbVimKey("D");
            } else {
                cursor.select(QTextCursor::LineUnderCursor);
                cursor.removeSelectedText();
                m_cbVimKey("dd");
            }
            return;
        }
        if (e->key() == Qt::Key_Y) {
            cursor.select(QTextCursor::LineUnderCursor);
            QApplication::clipboard()->setText(cursor.selectedText());
            cursor.clearSelection();
            m_cbVimKey("yy");
            return;
        }
        if (e->key() == Qt::Key_O) {
            cursor.movePosition(QTextCursor::EndOfLine);
            ed->setTextCursor(cursor);
            ed->insertPlainText("\n");
            INS();
            m_cbVimKey("o");
            return;
        }
        if (e->key() == Qt::Key_W && (e->modifiers() & Qt::ControlModifier)) {
            m_cbSaveReq();
            m_cbVimKey("<C-w>");
            return;
        }
    }

    void handleCmdKey(QKeyEvent *e) {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            QString cmd = m_cmdLine.mid(1).trimmed();
            bool success = false;
            if (cmd == "w" || cmd == "write") {
                m_cbSaveReq();
                success = true;
            } else if (cmd == "q" || cmd == "quit") {
                success = true;
            } else if (cmd == "wq") {
                m_cbSaveReq();
                success = true;
            } else if (cmd == "q!") {
                success = true;
            }
            m_cbCmdExecuted(cmd, success);
            m_cmdLine.clear();
            Vi();
            m_cbCmdChanged("");
            return;
        }
        if (e->key() == Qt::Key_Backspace && !m_cmdLine.isEmpty()) {
            m_cmdLine.chop(1);
            m_cbCmdChanged(m_cmdLine);
            return;
        }
        if (!e->text().isEmpty()) {
            m_cmdLine += e->text();
            m_cbCmdChanged(m_cmdLine);
            return;
        }
    }
};

class VColors {
public:
    static void initDefaults() {
        Settings &s = Settings::instance();
        if (!s.contains("theme/lineNumberBg"))
            s.setValue("theme/lineNumberBg", QColor(30, 30, 30));
        if (!s.contains("theme/lineNumberFg"))
            s.setValue("theme/lineNumberFg", QColor(100, 180, 100));
        if (!s.contains("theme/lineHighlightColor"))
            s.setValue("theme/lineHighlightColor", QColor(0, 60, 30, 102));
        if (!s.contains("theme/lineNumberWidth"))
            s.setValue("theme/lineNumberWidth", 3);
    }

    static QColor getLineNumBg(QWidget*) {
        QColor c = Settings::instance().get<QColor>("theme/lineNumberBg");
        return c.isValid() ? c : QColor(30, 30, 30);
    }
    static QColor getLineNumFg(QWidget*) {
        QColor c = Settings::instance().get<QColor>("theme/lineNumberFg");
        return c.isValid() ? c : QColor(100, 180, 100);
    }
    static QColor getHighlightColor(QWidget*) {
        QColor c = Settings::instance().get<QColor>("theme/lineHighlightColor");
        return c.isValid() ? c : QColor(0, 60, 30, 102);
    }
    static int getLineNumberWidth() {
        int w = Settings::instance().get<int>("theme/lineNumberWidth");
        return w > 0 ? w : 3;
    }
};

class VexEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    VexEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setupMode(QPushButton *btn);
    Mode &mode() { return m_mode; }
    QTextDocument::FindFlags getFindFlags(bool caseSensitive, bool wholeWords) const;
    void setLineWrapping(bool wrap);
    bool isLineWrapping() const { return lineWrapEnabled; }

public slots:
    void highlightCurrentLine();

signals:
    void modeChanged(Mode::ModeEnum mode);
    void saveRequested();
    void vimKeyPressed(const QString &keyDesc);
    void commandLineChanged(const QString &command);
    void commandExecuted(const QString &command, bool success);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    LineNumberArea *lineNumberArea;
    Mode            m_mode;
    bool            lineWrapEnabled;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(VexEditor *editor) : QWidget(editor), codeEditor(editor) {
        setObjectName("lineNumberArea");
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    VexEditor *codeEditor;
};

VexEditor::VexEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , lineNumberArea(new LineNumberArea(this))
    , lineWrapEnabled(false)
{
    setObjectName("VexEditor");
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(40);
    connect(this, &QPlainTextEdit::blockCountChanged,     this, &VexEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,         this, &VexEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &VexEditor::highlightCurrentLine);
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void VexEditor::setupMode(QPushButton *btn) {
    m_mode.init(
        btn,
        [this]()                         { emit saveRequested(); },
        [this](const QString &k)         { emit vimKeyPressed(k); },
        [this](const QString &c)         { emit commandLineChanged(c); },
        [this](const QString &c, bool s) { emit commandExecuted(c, s); },
        [this](Mode::ModeEnum m)         { emit modeChanged(m); },
        [this](QKeyEvent *e)             { QPlainTextEdit::keyPressEvent(e); }
        );
    m_mode.setupButton();
}

int VexEditor::lineNumberAreaWidth() {
    int digits = 1;
    int maxLines = qMax(1, document()->blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    return VColors::getLineNumberWidth() + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}
void VexEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), VColors::getLineNumBg(this));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    painter.setPen(VColors::getLineNumFg(this));

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top, lineNumberArea->width() - 4, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
QTextDocument::FindFlags VexEditor::getFindFlags(bool caseSensitive, bool wholeWords) const {
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWords)    flags |= QTextDocument::FindWholeWords;
    return flags;
}

void VexEditor::setLineWrapping(bool wrap) {
    lineWrapEnabled = wrap;
    setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    viewport()->update();
}

void VexEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void VexEditor::keyPressEvent(QKeyEvent *e) {
    m_mode.handleKey(this, e);
}

void VexEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void VexEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void VexEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = VColors::getHighlightColor(this);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

class FindReplaceDialog : public QDialog {
    Q_OBJECT
public:
    explicit FindReplaceDialog(QWidget *parent = nullptr);
    QString findText() const { return findEdit->text(); }
    QString replaceText() const { return replaceEdit->text(); }
    bool isCaseSensitive() const { return caseCheckBox->isChecked(); }
    bool isWholeWords() const { return wholeWordsCheckBox->isChecked(); }
    void setFindText(const QString &text);

signals:
    void findNextRequested();
    void findPreviousRequested();
    void replaceRequested();
    void replaceAllRequested();

private slots:
    void onFindTextChanged(const QString &text);

private:
    QLineEdit *findEdit;
    QLineEdit *replaceEdit;
    QCheckBox *caseCheckBox;
    QCheckBox *wholeWordsCheckBox;
    QPushButton *findNextButton;
    QPushButton *findPrevButton;
    QPushButton *replaceButton;
    QPushButton *replaceAllButton;
};

FindReplaceDialog::FindReplaceDialog(QWidget *parent)
    : QDialog(parent)
    , findEdit(new QLineEdit(this))
    , replaceEdit(new QLineEdit(this))
    , caseCheckBox(new QCheckBox("Match &case", this))
    , wholeWordsCheckBox(new QCheckBox("&Whole words", this))
{
    setWindowTitle("Find and Replace");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(Settings::instance().resolveIcon("edit-find"));

    auto *formLayout = new QFormLayout;
    formLayout->addRow("Find:", findEdit);
    formLayout->addRow("Replace:", replaceEdit);

    auto *optionsLayout = new QHBoxLayout;
    optionsLayout->addWidget(caseCheckBox);
    optionsLayout->addWidget(wholeWordsCheckBox);
    optionsLayout->addStretch();

    auto *buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    findNextButton   = buttonBox->addButton("Find &Next",     QDialogButtonBox::ActionRole);
    findPrevButton   = buttonBox->addButton("Find &Previous", QDialogButtonBox::ActionRole);
    replaceButton    = buttonBox->addButton("&Replace",       QDialogButtonBox::ActionRole);
    replaceAllButton = buttonBox->addButton("Replace &All",   QDialogButtonBox::ActionRole);
    QPushButton *closeButton = buttonBox->addButton(QDialogButtonBox::Close);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(optionsLayout);
    mainLayout->addWidget(buttonBox);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    replaceButton->setEnabled(false);

    connect(findEdit,         &QLineEdit::textChanged, this, &FindReplaceDialog::onFindTextChanged);
    connect(findNextButton,   &QPushButton::clicked,   this, &FindReplaceDialog::findNextRequested);
    connect(findPrevButton,   &QPushButton::clicked,   this, &FindReplaceDialog::findPreviousRequested);
    connect(replaceButton,    &QPushButton::clicked,   this, &FindReplaceDialog::replaceRequested);
    connect(replaceAllButton, &QPushButton::clicked,   this, &FindReplaceDialog::replaceAllRequested);
    connect(closeButton,      &QPushButton::clicked,   this, &QDialog::reject);
}

void FindReplaceDialog::setFindText(const QString &text) {
    findEdit->setText(text);
    findEdit->selectAll();
    findEdit->setFocus();
}

void FindReplaceDialog::onFindTextChanged(const QString &text) {
    replaceButton->setEnabled(!text.isEmpty());
}

class AdminFileHandler : public QObject {
    Q_OBJECT
public:
    bool saveWithAdmin(const QString &filePath, const QString &content);
    bool openWithAdmin(const QString &filePath);

private:
    QString findTerminal();
};

QString AdminFileHandler::findTerminal() {
#ifdef Q_OS_WIN
    return "cmd.exe";
#elif defined(Q_OS_MAC)
    return "open";
#else
    const QStringList absolutePaths = {
        "/usr/bin/konsole",
        "/usr/bin/gnome-terminal",
        "/usr/bin/xfce4-terminal",
        "/usr/bin/mate-terminal",
        "/usr/bin/terminator",
        "/usr/bin/alacritty",
        "/usr/bin/xterm",
        "/usr/bin/x-terminal-emulator"
    };
    for (const QString &path : std::as_const(absolutePaths)) {
        if (QFile::exists(path)) {
            return QFileInfo(path).fileName();
        }
    }
    const QStringList names = {
        "konsole", "gnome-terminal", "xfce4-terminal",
        "mate-terminal", "terminator", "alacritty", "xterm"
    };
    for (const QString &name : std::as_const(names)) {
        if (!QStandardPaths::findExecutable(name).isEmpty()) {
            return name;
        }
    }
#endif
    return QString();
}

bool AdminFileHandler::saveWithAdmin(const QString &filePath, const QString &content) {
#ifdef Q_OS_WIN
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QMessageBox::warning(nullptr, "Error", "Cannot create temporary file.");
        return false;
    }
    QTextStream out(&tempFile);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    tempFile.close();

    QTemporaryFile batchFile(QDir::temp().absoluteFilePath("XXXXXX.bat"));
    if (!batchFile.open()) {
        QMessageBox::warning(nullptr, "Error", "Cannot create batch file.");
        return false;
    }

    QTextStream batchOut(&batchFile);
    batchOut << "@echo off\n";
    batchOut << "copy /Y \"" << QDir::toNativeSeparators(tempFile.fileName())
             << "\" \"" << QDir::toNativeSeparators(filePath) << "\"\n";
    batchOut << "if %errorlevel% equ 0 (\n";
    batchOut << "    echo File saved successfully!\n";
    batchOut << "    timeout /t 2 /nobreak >nul\n";
    batchOut << ") else (\n";
    batchOut << "    echo Save failed!\n";
    batchOut << "    pause\n";
    batchOut << ")\n";
    batchOut.flush();
    batchFile.close();

    HINSTANCE result = ShellExecuteA(
        NULL,
        "runas",
        batchFile.fileName().toUtf8().constData(),
        NULL,
        NULL,
        SW_SHOWNORMAL
        );

    return reinterpret_cast<intptr_t>(result) > 32;
#else
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QMessageBox::warning(nullptr, "Error", "Cannot create temporary file.");
        return false;
    }
    QTextStream out(&tempFile);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    tempFile.close();

    QString terminal = findTerminal();
    if (terminal.isEmpty()) {
        QMessageBox::warning(nullptr, "Error", "No terminal emulator found.");
        return false;
    }

    QString cmd = QString("sudo cp \"%1\" \"%2\" && echo 'File saved successfully.' || echo 'Save failed.'")
                      .arg(tempFile.fileName(), filePath);
    QStringList terminalArgs;
    if (terminal == "konsole") {
        terminalArgs = {"-e", "sh", "-c", cmd + "; read -p 'Press Enter to close...'"};
    } else if (terminal == "gnome-terminal") {
        terminalArgs = {"--", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    } else {
        terminalArgs = {"-e", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    }
    return QProcess::startDetached(terminal, terminalArgs);
#endif
}

bool AdminFileHandler::openWithAdmin(const QString &filePath) {
#ifdef Q_OS_WIN
    QString appPath = QCoreApplication::applicationFilePath();
    QProcess::startDetached("cmd.exe", QStringList() << "/c" << "start" << appPath << "-f" << filePath);
    return true;
#else
    QString terminal = findTerminal();
    if (terminal.isEmpty()) {
        QMessageBox::warning(nullptr, "Error", "No terminal emulator found.");
        return false;
    }

    QString appPath = QCoreApplication::applicationFilePath();
    QString cmd = QString("sudo \"%1\" -f \"%2\"").arg(appPath, filePath);
    QStringList terminalArgs;
    if (terminal == "konsole") {
        terminalArgs = {"-e", "sh", "-c", cmd + "; read -p 'Press Enter to close...'"};
    } else if (terminal == "gnome-terminal") {
        terminalArgs = {"--", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    } else {
        terminalArgs = {"-e", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    }
    return QProcess::startDetached(terminal, terminalArgs);
#endif
}

class EmptyStateView : public QWidget {
    Q_OBJECT
public:
    explicit EmptyStateView(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(12);

        QLabel *iconLabel = new QLabel(this);
        QIcon vexIcon = Settings::instance().resolveIcon("vex");
        if (!vexIcon.isNull()) {
            iconLabel->setPixmap(vexIcon.pixmap(130, 130));
        }
        layout->addWidget(iconLabel);

        QLabel *welcomeLabel = new QLabel("Welcome to Vex", this);
        QFont welcomeFont;
        welcomeFont.setFamilies({"Nimbus Mono PS", "Nimbus Mono", "monospace"});
        welcomeFont.setPointSize(16);
        welcomeLabel->setFont(welcomeFont);
        layout->addWidget(welcomeLabel);

        QLabel *infoLabel = new QLabel(
            "Open a file to start editing\n"
            "______________________________________\n\n"
            "Ctrl+N                        New file\n"
            "Ctrl+O                    Open by name\n"
            "Ctrl+Shift+O               Open dialog\n"
            "Ctrl+S                            Save\n"
            "Ctrl+F                            Find\n"
            "Esc                        Change mode\n\n"
            "! Tip: Drag and drop files here to open up",
            this);
        QFont infoFont;
        infoFont.setItalic(true);
        infoFont.setFamilies({"Nimbus Mono PS", "Nimbus Mono", "monospace"});
        infoFont.setPointSize(11);
        infoLabel->setFont(infoFont);
        layout->addWidget(infoLabel);
    }
};

class LineEnding : public QObject {
    Q_OBJECT
public:
    enum Type { LF, CRLF, CR };

    explicit LineEnding(QObject *parent = nullptr) : QObject(parent), m_type(LF) {}
    explicit LineEnding(Type type, QObject *parent = nullptr) : QObject(parent), m_type(type) {}
    Type type() const { return m_type; }
    QString label() const {
        switch (m_type) {
        case CRLF: return "CRLF";
        case CR:   return "CR";
        default:   return "LF";
        }
    }

    static LineEnding::Type detect(const QByteArray &data) {
        int crlfCount = 0, crCount = 0, lfCount = 0;
        int limit = qMin(data.size(), 8192);
        for (int i = 0; i < limit; ++i) {
            if (data[i] == '\r') {
                if (i + 1 < limit && data[i + 1] == '\n') { ++crlfCount; ++i; }
                else ++crCount;
            } else if (data[i] == '\n') {
                ++lfCount;
            }
        }
        if (crlfCount > lfCount && crlfCount > crCount) return CRLF;
        if (crCount   > lfCount && crCount   > crlfCount) return CR;
        return LF;
    }

    QString decode(const QByteArray &data) const {
        QStringDecoder dec(QStringConverter::Utf8);
        QString text = dec(data);
        text.replace("\r\n", "\n");
        text.replace('\r', '\n');
        return text;
    }

    QByteArray encode(const QString &text) const {
        QString s = text;
        s.replace("\r\n", "\n");
        s.replace('\r', '\n');
        switch (m_type) {
        case CRLF: s.replace('\n', "\r\n"); break;
        case CR:   s.replace('\n', '\r');   break;
        default:   break;
        }
        QStringEncoder enc(QStringConverter::Utf8);
        return enc(s);
    }

    void setupUi(QStatusBar *statusBar) {
        m_button = new QPushButton(label(), statusBar);
        m_button->setToolTip("Line ending");
        m_button->setFlat(true);
        m_button->setCursor(Qt::PointingHandCursor);

        QMenu *menu = new QMenu(m_button);
        QActionGroup *group = new QActionGroup(menu);
        group->setExclusive(true);

        auto addAction = [&](const QString &text, Type t) {
            QAction *action = menu->addAction(text);
            action->setData(t);
            action->setCheckable(true);
            group->addAction(action);
            if (t == m_type) action->setChecked(true);
        };

        addAction("Unix (LF)", LF);
        addAction("Windows (CRLF)", CRLF);
        addAction("Classic Mac (CR)", CR);

        m_button->setMenu(menu);
        statusBar->addPermanentWidget(m_button);

        connect(group, &QActionGroup::triggered, this, [this](QAction *action) {
            Type newType = static_cast<Type>(action->data().toInt());
            if (m_type != newType) {
                m_type = newType;
                m_button->setText(label());
                emit lineEndingChanged();
            }
        });
    }

    void setType(Type t) {
        if (m_type != t) {
            m_type = t;
            if (m_button) {
                m_button->setText(label());
                for (QAction *action : m_button->menu()->actions()) {
                    if (action->data().toInt() == t) {
                        action->setChecked(true);
                        break;
                    }
                }
            }
        }
    }

signals:
    void lineEndingChanged();

private:
    Type m_type;
    QPushButton *m_button = nullptr;
};

class VexWidget : public QWidget {
    Q_OBJECT
    friend class VexCorePlugin;

public:
    explicit VexWidget(QWidget *parent = nullptr);
    ~VexWidget();
    void openFileAtPath(const QString &path);
    QTabWidget* getTabWidget() { return tabWidget; }
    QString getFilePath(VexEditor *editor) { return filePaths.value(editor); }
    void setupUI(QMainWindow *mainWin);
    void setupMenus(QMainWindow *mainWin);
    void setupToolbar(QMainWindow *mainWin);
    bool eventFilter(QObject *obj, QEvent *event) override;
    void loadSettings();

private slots:
    void newFile();
    void openFile();
    void openFileByName();
    void saveFile();
    void saveFileAs();
    void closeTab(int index);
    void toggleLineWrapping(bool enabled);
    void updateCursorPosition();
    void showFindReplaceDialog();
    void findNext();
    void findPrevious();
    void replace();
    void replaceAll();
    void showAbout();
    void undo();
    void redo();
    void openTerminal();
    void onTabCountChanged(int count);
    void saveToolbarState();
    void restoreToolbarState();
    void saveSessionAndQuit();
    void loadSavedSession();
    void handleInstanceRequest(const QString &requestFilePath);
    void onSettingsFileChanged(const QString &path);
    void onLineEndingChanged();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void saveSettings();
    void updateRecentMenu();
    void updateTabAppearance(int tabIndex);
    void updateWindowTitle(QMainWindow *mainWin);
    bool hasBinaryContent(const QByteArray &data) const;
    VexEditor* getCurrentEditor();
    QString getCurrentWorkingDirectory() const;

    QStackedWidget *stackedWidget;
    QTabWidget     *tabWidget;
    QPushButton    *modeLabel;
    QLabel         *positionLabel;
    QLabel         *vimHintLabel;
    QAction        *lineWrapAction;
    LineEnding     *m_lineEnding;
    QMap<VexEditor*, QString> filePaths;
    QMap<VexEditor*, LineEnding::Type> editorLineEndings;
    FindReplaceDialog *findDialog;
    QString currentFindText;
    QString currentReplaceText;
    bool currentCaseSensitive;
    bool currentWholeWords;
    QFileSystemWatcher *fileWatcher;
    QFileSystemWatcher *m_settingsWatcher;
    AdminFileHandler    adminHandler;
    QMenu          *recentMenu;
    EmptyStateView *emptyView;
    QMainWindow    *m_mainWindow;
    bool            m_sessionRestored;
    const int MAX_RECENT_FILES = 10;
};

VexWidget::VexWidget(QWidget *parent)
    : QWidget(parent)
    , findDialog(nullptr)
    , currentCaseSensitive(false)
    , currentWholeWords(false)
    , m_mainWindow(nullptr)
    , stackedWidget(nullptr)
    , tabWidget(nullptr)
    , emptyView(nullptr)
    , m_sessionRestored(false)
    , modeLabel(nullptr)

    , m_lineEnding(nullptr)
    , m_settingsWatcher(nullptr)
{
    setAcceptDrops(true);
    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (m_mainWindow) {
            m_mainWindow->statusBar()->showMessage("File modified externally: " + QFileInfo(path).fileName(), 3000);
        }
    });
    m_settingsWatcher = new QFileSystemWatcher(this);
    QString configPath = Settings::instance().configFilePath();
    if (QFile::exists(configPath)) {
        m_settingsWatcher->addPath(configPath);
    }
    connect(m_settingsWatcher, &QFileSystemWatcher::fileChanged, this, &VexWidget::onSettingsFileChanged);
}

VexWidget::~VexWidget() {
    saveSettings();
}

void VexWidget::onSettingsFileChanged(const QString &path) {
    if (!m_settingsWatcher->files().contains(path)) {
        m_settingsWatcher->addPath(path);
    }
    for (int i = 0; i < tabWidget->count(); ++i) {
        VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(i));
        if (editor) {
            editor->viewport()->update();
            editor->highlightCurrentLine();
        }
    }
    if (m_mainWindow) {
        m_mainWindow->statusBar()->showMessage("Settings updated", 500);
    }
}



void VexWidget::saveToolbarState() {
    if (m_mainWindow) {
        Settings::instance().setValue("toolbarState", m_mainWindow->saveState());
    }
}

void VexWidget::restoreToolbarState() {
    if (m_mainWindow) {
        QByteArray state = Settings::instance().get<QByteArray>("toolbarState");
        if (!state.isEmpty()) {
            m_mainWindow->restoreState(state);
        }
    }
}

void VexWidget::saveSessionAndQuit() {
    QString tempDir = Settings::basePath() + "/.temp";
    QDir().mkpath(tempDir);

    QStringList sessionFiles;

    for (int i = 0; i < tabWidget->count(); ++i) {
        VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(i));
        if (editor && editor->document()->isModified()) {
            QString originalPath = filePaths.value(editor);
            QString sessionPath = tempDir + QString("/session_%1.bak").arg(i);

            QFile file(sessionPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out.setEncoding(QStringConverter::Utf8);
                out << (originalPath.isEmpty() ? "" : originalPath) << "\n";
                out << editor->toPlainText();
                file.close();

                sessionFiles.append(sessionPath);
            }
        }
    }

    if (!sessionFiles.isEmpty()) {
        Settings::instance().setValue("sessionFiles", sessionFiles);
        Settings::instance().setValue("hasSavedSession", true);
    }

    saveToolbarState();
    saveSettings();
    qApp->quit();
}

void VexWidget::loadSavedSession() {
    Settings &settings = Settings::instance();
    bool hasSession = settings.get<bool>("hasSavedSession", false);
    if (!hasSession) return;

    QStringList sessionFiles = settings.get<QStringList>("sessionFiles");
    if (sessionFiles.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Restore Previous Session",
        "A previous session with unsaved changes was found.\n\n"
        "Would you like to restore these files?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
        );

    if (reply == QMessageBox::Yes) {
        for (const QString &sessionPath : std::as_const(sessionFiles)) {
            QFile file(sessionPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                in.setEncoding(QStringConverter::Utf8);

                QString originalPath = in.readLine();
                QString content = in.readAll();
                file.close();

                VexEditor *editor = new VexEditor(this);
                editor->setupMode(modeLabel);
                editor->setLineWrapping(lineWrapAction->isChecked());
                editor->setPlainText(content);
                editor->document()->setModified(true);

                connect(editor, &VexEditor::modeChanged, this, [this](Mode::ModeEnum) {
                    updateCursorPosition();
                });
                connect(editor, &VexEditor::saveRequested, this, &VexWidget::saveFile);
                connect(editor, &VexEditor::vimKeyPressed, this, [this](const QString &key) {
                    vimHintLabel->setText(key);
                    QTimer::singleShot(800, this, [this]() { vimHintLabel->clear(); });
                });
                connect(editor, &VexEditor::commandLineChanged, this, [this](const QString &cmd) {
                    vimHintLabel->setText(cmd);
                });
                connect(editor, &VexEditor::commandExecuted, this, [this](const QString &cmd, bool success) {
                    QString msg = success ? "Success: " + cmd : "Failed: " + cmd;
                    if (m_mainWindow) {
                        m_mainWindow->statusBar()->showMessage(msg, 2000);
                    }
                    vimHintLabel->clear();
                });
                connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexWidget::updateCursorPosition);
                connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool) {
                    int index = tabWidget->indexOf(editor);
                    if (index != -1) {
                        updateTabAppearance(index);
                    }
                });

                QString tabName = originalPath.isEmpty() ? "Restored (Unsaved)" :
                                      QFileInfo(originalPath).fileName() + " (Recovered)";
                int index = tabWidget->addTab(editor, tabName);
                tabWidget->setCurrentIndex(index);
                filePaths[editor] = originalPath;
                editorLineEndings[editor] = LineEnding::LF;
                updateTabAppearance(index);
            }
            QFile::remove(sessionPath);
        }
    } else {
        for (const QString &sessionPath : std::as_const(sessionFiles)) {
            QFile::remove(sessionPath);
        }
    }

    settings.remove("sessionFiles");
    settings.remove("hasSavedSession");

    updateWindowTitle(m_mainWindow);
    onTabCountChanged(tabWidget->count());
}

void VexWidget::handleInstanceRequest(const QString &requestFilePath) {
    QFile file(requestFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QStringList pathsToOpen;

    while (!in.atEnd()) {
        QString path = in.readLine().trimmed();
        if (!path.isEmpty()) {
            pathsToOpen.append(path);
        }
    }
    file.close();

    QFile clearFile(requestFilePath);
    if (clearFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        clearFile.close();
    }

    for (const QString &path : pathsToOpen) {
        if (QFileInfo::exists(path)) {
            openFileAtPath(path);
        } else if (m_mainWindow) {
            m_mainWindow->statusBar()->showMessage(tr("Invalid file path: %1").arg(path), 2000);
        }
    }

    if (m_mainWindow && !pathsToOpen.isEmpty()) {
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
}

void VexWidget::setupUI(QMainWindow *mainWin) {
    m_mainWindow = mainWin;
    stackedWidget = new QStackedWidget(this);
    emptyView = new EmptyStateView(stackedWidget);
    emptyView->setObjectName("VexEmpty");
    stackedWidget->addWidget(emptyView);

    tabWidget = new QTabWidget(stackedWidget);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    stackedWidget->addWidget(tabWidget);
    stackedWidget->setObjectName("VexStack");
    tabWidget->setObjectName("VexTab");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stackedWidget);

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &VexWidget::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, [this, mainWin](int) {
        updateWindowTitle(mainWin);
        onTabCountChanged(tabWidget->count());
        VexEditor *editor = getCurrentEditor();
        if (editor) {
            LineEnding::Type type = editorLineEndings.value(editor, LineEnding::LF);
            m_lineEnding->setType(type);
        }
    });
    onTabCountChanged(0);

    modeLabel = new QPushButton(mainWin);
    modeLabel->setCursor(Qt::PointingHandCursor);
    modeLabel->setStyleSheet("QPushButton { background-color: transparent; color: transparent; border: none; } QPushButton:hover { background-color: transparent; color: transparent; } QPushButton:pressed { background-color: transparent; color: transparent; }");
    mainWin->statusBar()->addPermanentWidget(modeLabel);
    m_lineEnding = new LineEnding(this);
    m_lineEnding->setupUi(mainWin->statusBar());
    connect(m_lineEnding, &LineEnding::lineEndingChanged, this, &VexWidget::onLineEndingChanged);


    positionLabel = new QLabel("Line: 1, Col: 1", mainWin);
    mainWin->statusBar()->addPermanentWidget(positionLabel);

    vimHintLabel = new QLabel("", mainWin);
    vimHintLabel->setStyleSheet("QLabel { font-family: monospace; }");
    mainWin->statusBar()->addPermanentWidget(vimHintLabel);

    QTimer::singleShot(0, this, &VexWidget::restoreToolbarState);
}

void VexWidget::setupMenus(QMainWindow *mainWin) {
    QMenu *fileMenu = mainWin->menuBar()->addMenu("&File");

    QAction *newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &VexWidget::newFile);

    QAction *openAction = fileMenu->addAction("&Open");
    openAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(openAction, &QAction::triggered, this, &VexWidget::openFile);

    QAction *openByNameAction = fileMenu->addAction("Open &By Name...");
    openByNameAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(openByNameAction, &QAction::triggered, this, &VexWidget::openFileByName);

    fileMenu->addSeparator();

    recentMenu = fileMenu->addMenu("Open &Recent");
    updateRecentMenu();

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &VexWidget::saveFile);

    QAction *saveAsAction = fileMenu->addAction("Save &As");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &VexWidget::saveFileAs);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, mainWin, &QWidget::close);

    QMenu *editMenu = mainWin->menuBar()->addMenu("&Edit");

    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &VexWidget::undo);

    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &VexWidget::redo);

    editMenu->addSeparator();

    QAction *findAction = editMenu->addAction("&Find and Replace");
    findAction->setShortcut(QKeySequence("Ctrl+F"));
    connect(findAction, &QAction::triggered, this, &VexWidget::showFindReplaceDialog);

    QMenu *viewMenu = mainWin->menuBar()->addMenu("&View");

    lineWrapAction = viewMenu->addAction("&Line Wrapping");
    lineWrapAction->setCheckable(true);
    lineWrapAction->setChecked(false);
    connect(lineWrapAction, &QAction::toggled, this, &VexWidget::toggleLineWrapping);

    QMenu *helpMenu = mainWin->menuBar()->addMenu("&Help");

    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &VexWidget::showAbout);
}

void VexWidget::setupToolbar(QMainWindow *mainWin) {
    QToolBar *toolbar = mainWin->addToolBar("Main");
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setObjectName("Vtoolbar");
    auto addAction = [&](const QString &label, const QString &iconName, const char *slot) {
        QIcon icon = Settings::instance().resolveIcon(iconName);
        QAction *action = new QAction(icon, label, this);
        connect(action, SIGNAL(triggered()), this, slot);
        toolbar->addAction(action);
        return action;
    };
    connect(toolbar, &QToolBar::orientationChanged, this, [toolbar](Qt::Orientation orientation) {
        if (orientation == Qt::Vertical) {
            toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        } else {
            toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        }
    });

    addAction("New",          "document-new",      SLOT(newFile()));
    addAction("Open",         "document-open",     SLOT(openFile()));
    addAction("Open by Name", "document-open",     SLOT(openFileByName()));
    addAction("Save",         "document-save",     SLOT(saveFile()));
    toolbar->addSeparator();
    addAction("Undo",         "edit-undo",          SLOT(undo()));
    addAction("Redo",         "edit-redo",          SLOT(redo()));
    toolbar->addSeparator();
    addAction("Find",         "edit-find",          SLOT(showFindReplaceDialog()));
    addAction("Terminal",     "utilities-terminal", SLOT(openTerminal()));
}

void VexWidget::newFile() {
    VexEditor *editor = new VexEditor(this);
    editor->setupMode(modeLabel);
    editor->setLineWrapping(lineWrapAction->isChecked());

    connect(editor, &VexEditor::modeChanged, this, [this](Mode::ModeEnum) {
        updateCursorPosition();
    });
    connect(editor, &VexEditor::saveRequested, this, &VexWidget::saveFile);
    connect(editor, &VexEditor::vimKeyPressed, this, [this](const QString &key) {
        vimHintLabel->setText(key);
        QTimer::singleShot(800, this, [this]() { vimHintLabel->clear(); });
    });
    connect(editor, &VexEditor::commandLineChanged, this, [this](const QString &cmd) {
        vimHintLabel->setText(cmd);
    });
    connect(editor, &VexEditor::commandExecuted, this, [this](const QString &cmd, bool success) {
        QString msg = success ? "Success: " + cmd : "Failed: " + cmd;
        if (m_mainWindow) {
            m_mainWindow->statusBar()->showMessage(msg, 2000);
        }
        vimHintLabel->clear();
    });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexWidget::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool) {
        int index = tabWidget->indexOf(editor);
        if (index != -1) {
            updateTabAppearance(index);
        }
    });

    int index = tabWidget->addTab(editor, "No Name");
    tabWidget->setCurrentIndex(index);
    filePaths[editor] = QString();
    editorLineEndings[editor] = LineEnding::LF;
    updateTabAppearance(index);
    updateWindowTitle(m_mainWindow);
    onTabCountChanged(tabWidget->count());
}

void VexWidget::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", QString(), "All Files (*)");
    if (!fileName.isEmpty()) {
        openFileAtPath(fileName);
    }
}
void VexWidget::openFileAtPath(const QString &filePath) {
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, "Error", "File does not exist: " + filePath);
        return;
    }

    if (!info.isReadable()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Permission Denied",
            QString("You don't have permission to read:<br><b>%1</b><br><br>"
                    "Open with administrator privileges?").arg(filePath),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
            adminHandler.openWithAdmin(filePath);
            return;
        }
        QMessageBox::warning(this, "Error", "File is not readable: " + filePath);
        return;
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Cannot open file: " + filePath);
        return;
    }

    Settings &settings = Settings::instance();
    QStringList whitelistedFiles = settings.get<QStringList>("binaryWhitelist", QStringList());
    bool isWhitelisted = whitelistedFiles.contains(filePath);

    QByteArray data = file.readAll();
    file.close();

    bool isBinary = hasBinaryContent(data);

    if (isBinary && !isWhitelisted) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Binary File Detected");
        msgBox.setText("This file appears to be binary. Vex Editor primarily supports text files.");
        msgBox.setInformativeText("Opening binary files may display unreadable content.");
        msgBox.setIcon(QMessageBox::Warning);

        QCheckBox *whitelistCheckBox = new QCheckBox("Mark this file as not binary");
        msgBox.setCheckBox(whitelistCheckBox);

        QPushButton *openButton   = msgBox.addButton("Open Anyway", QMessageBox::AcceptRole);
        QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == cancelButton) {
            return;
        }

        if (msgBox.clickedButton() == openButton && whitelistCheckBox->isChecked()) {
            whitelistedFiles.append(filePath);
            settings.setValue("binaryWhitelist", whitelistedFiles);
        }
    }

    LineEnding::Type detectedType = LineEnding::detect(data);
    LineEnding converter(detectedType);
    QString content = converter.decode(data);

    VexEditor *editor = new VexEditor(this);
    editor->setupMode(modeLabel);
    editor->setLineWrapping(lineWrapAction->isChecked());
    editor->setPlainText(content);

    connect(editor, &VexEditor::modeChanged, this, [this](Mode::ModeEnum) {
        updateCursorPosition();
    });
    connect(editor, &VexEditor::saveRequested, this, &VexWidget::saveFile);
    connect(editor, &VexEditor::vimKeyPressed, this, [this](const QString &key) {
        vimHintLabel->setText(key);
        QTimer::singleShot(800, this, [this]() { vimHintLabel->clear(); });
    });
    connect(editor, &VexEditor::commandLineChanged, this, [this](const QString &cmd) {
        vimHintLabel->setText(cmd);
    });
    connect(editor, &VexEditor::commandExecuted, this, [this](const QString &cmd, bool success) {
        QString msg = success ? "Success: " + cmd : "Failed: " + cmd;
        if (m_mainWindow) {
            m_mainWindow->statusBar()->showMessage(msg, 2000);
        }
        vimHintLabel->clear();
    });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexWidget::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool) {
        int index = tabWidget->indexOf(editor);
        if (index != -1) {
            updateTabAppearance(index);
        }
    });

    int index = tabWidget->addTab(editor, QFileInfo(filePath).fileName());
    tabWidget->setCurrentIndex(index);
    filePaths[editor] = filePath;
    editorLineEndings[editor] = detectedType;
    if (tabWidget->currentWidget() == editor) {
        m_lineEnding->setType(detectedType);
    }
    updateTabAppearance(index);
    updateWindowTitle(m_mainWindow);
    fileWatcher->addPath(filePath);

    QStringList recentFiles = settings.get<QStringList>("recentFiles");
    recentFiles.removeAll(filePath);
    recentFiles.prepend(filePath);
    while (recentFiles.size() > MAX_RECENT_FILES) {
        recentFiles.removeLast();
    }
    settings.setValue("recentFiles", recentFiles);
    updateRecentMenu();
    if (m_mainWindow) {
        m_mainWindow->statusBar()->showMessage("Opened: " + filePath, 2000);
    }
    onTabCountChanged(tabWidget->count());
}
void VexWidget::openFileByName() {
    QDialog dialog(this);
    dialog.setWindowTitle("Open File by Path");
    dialog.setWindowIcon(Settings::instance().resolveIcon("document-open"));
    dialog.setMinimumSize(500, 350);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *iconArea = new QLabel(&dialog);
    iconArea->setFixedSize(100, 100);
    iconArea->setAlignment(Qt::AlignCenter);
    iconArea->setStyleSheet("border: 1px solid #aaa; border-radius: 8px; background: transparent;");

    QLabel *iconLabel = new QLabel(iconArea);
    iconLabel->setFixedSize(64, 64);
    iconLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *iconLayout = new QVBoxLayout(iconArea);
    iconLayout->setContentsMargins(18, 18, 18, 18);
    iconLayout->addWidget(iconLabel);
    mainLayout->addWidget(iconArea, 0, Qt::AlignHCenter);

    QLabel *label = new QLabel("Enter file path (use ~/ for home):", &dialog);
    label->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(label);

    QLineEdit *pathEdit = new QLineEdit(&dialog);
    pathEdit->setPlaceholderText("e.g. ~/Documents/file.txt");

    QCompleter *completer = new QCompleter(&dialog);
    QFileSystemModel *fsModel = new QFileSystemModel(completer);
    fsModel->setRootPath("");
    fsModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Drives);
    completer->setModel(fsModel);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    pathEdit->setCompleter(completer);

    QFileIconProvider iconProvider;
    QIcon defaultIcon = iconProvider.icon(QFileIconProvider::File);
    iconLabel->setPixmap(defaultIcon.pixmap(64, 64));

    connect(pathEdit, &QLineEdit::textChanged, [=, &iconProvider](const QString &text) {
        QString newText = text;
#ifdef Q_OS_WIN
        if (text.startsWith("~\\") || text.startsWith("~/")) {
            newText = QDir::homePath() + text.mid(1);
        }
#else
        if (text.startsWith("~/")) {
            newText = QDir::homePath() + text.mid(1);
        }
#endif
        if (newText != text) {
            pathEdit->blockSignals(true);
            pathEdit->setText(newText);
            pathEdit->setCursorPosition(newText.length());
            pathEdit->blockSignals(false);
            return;
        }

        if (text.isEmpty()) {
            iconLabel->setPixmap(defaultIcon.pixmap(64, 64));
            fsModel->setRootPath("");
            return;
        }

        QFileInfo info(text);
        QIcon icon = defaultIcon;
        if (info.exists()) {
            icon = info.isDir() ?
                       iconProvider.icon(QFileIconProvider::Folder) :
                       iconProvider.icon(info);
        }
        iconLabel->setPixmap(icon.pixmap(64, 64));

        QString rootPath = info.exists() && info.isDir() ? text : info.absolutePath();
        fsModel->setRootPath(rootPath);
    });

    mainLayout->addWidget(pathEdit);

    QString hintText;
#ifdef Q_OS_WIN
    hintText = "Hint: Type ~/ or ~\\ and it will auto-expand to your home directory";
#else
    hintText = "Hint: Type ~/ and it will auto-expand to your home directory";
#endif
    QLabel *hintLabel = new QLabel(hintText, &dialog);
    hintLabel->setStyleSheet("color: gray; font-size: 10px;");
    hintLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(hintLabel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Open | QDialogButtonBox::Cancel, &dialog);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    pathEdit->setFocus();

    if (dialog.exec() == QDialog::Accepted) {
        QString filePath = pathEdit->text().trimmed();
        if (filePath.isEmpty()) {
            newFile();
            return;
        }

        QFileInfo fileInfo(filePath);
        if (fileInfo.isRelative()) {
            filePath = QDir::current().absoluteFilePath(filePath);
            fileInfo = QFileInfo(filePath);
        }

        if (fileInfo.exists() && fileInfo.isDir()) {
            if (m_mainWindow) {
                m_mainWindow->statusBar()->showMessage("Cannot open directory: " + filePath, 3000);
            }
            return;
        }

        QString dirPath = fileInfo.absolutePath();
        QDir targetDir(dirPath);
        if (!targetDir.exists()) {
            if (!targetDir.mkpath(".")) {
                if (m_mainWindow) {
                    m_mainWindow->statusBar()->showMessage("Cannot create directory: " + dirPath, 3000);
                }
                return;
            }
        }

        if (!fileInfo.exists()) {
            QFile newFile(filePath);
            if (newFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                newFile.close();
            } else {
                if (m_mainWindow) {
                    m_mainWindow->statusBar()->showMessage("Cannot create: " + filePath, 3000);
                }
                return;
            }
        }

        openFileAtPath(filePath);
    }
}

void VexWidget::saveFile() {
    VexEditor *editor = getCurrentEditor();
    if (!editor) return;

    QString fileName = filePaths.value(editor);
    if (fileName.isEmpty()) {
        saveFileAs();
        return;
    }

    if (!editor->document()->isModified()) return;

    QSaveFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        LineEnding::Type type = editorLineEndings.value(editor, LineEnding::LF);
        LineEnding converter(type);
        QByteArray encoded = converter.encode(editor->toPlainText());
        file.write(encoded);
        if (file.commit()) {
            editor->document()->setModified(false);
            updateTabAppearance(tabWidget->currentIndex());
            if (m_mainWindow) {
                m_mainWindow->statusBar()->showMessage("File saved: " + fileName, 3000);
            }

            Settings &settings = Settings::instance();
            QStringList recentFiles = settings.get<QStringList>("recentFiles");
            recentFiles.removeAll(fileName);
            recentFiles.prepend(fileName);
            while (recentFiles.size() > MAX_RECENT_FILES) {
                recentFiles.removeLast();
            }
            settings.setValue("recentFiles", recentFiles);
            updateRecentMenu();
            return;
        }
    }

    QFileInfo info(fileName);
    if (!info.isWritable()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Permission Denied",
            QString("You don't have permission to save:<br><b>%1</b><br><br>"
                    "Save with administrator privileges?").arg(fileName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
            LineEnding::Type type = editorLineEndings.value(editor, LineEnding::LF);
            LineEnding converter(type);
            QByteArray encoded = converter.encode(editor->toPlainText());
            QString content = QString::fromUtf8(encoded);
            if (adminHandler.saveWithAdmin(fileName, content)) {
                editor->document()->setModified(false);
                updateTabAppearance(tabWidget->currentIndex());
                if (m_mainWindow) {
                    m_mainWindow->statusBar()->showMessage("Admin save initiated for: " + fileName, 3000);
                }
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "Could not save file:\n" + fileName);
    }
}

void VexWidget::saveFileAs() {
    VexEditor *editor = getCurrentEditor();
    if (!editor) return;

    QString fileName = QFileDialog::getSaveFileName(this, "Save File", QString(), "All Files (*)");
    if (!fileName.isEmpty()) {
        filePaths[editor] = fileName;
        updateTabAppearance(tabWidget->currentIndex());
        saveFile();
    }
}

bool VexWidget::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::Close) {
        closeEvent(static_cast<QCloseEvent*>(event));
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void VexWidget::closeTab(int index) {
    VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(index));
    if (editor && editor->document()->isModified()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes",
            QString("Tab \"%1\" has unsaved changes. Save before closing?").arg(tabWidget->tabText(index)),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );
        if (reply == QMessageBox::Save) {
            saveFile();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    if (editor) {
        QString path = filePaths.value(editor);
        if (!path.isEmpty()) {
            fileWatcher->removePath(path);
        }
        filePaths.remove(editor);
        editorLineEndings.remove(editor);
    }

    tabWidget->removeTab(index);
    updateWindowTitle(m_mainWindow);
    onTabCountChanged(tabWidget->count());
}

void VexWidget::toggleLineWrapping(bool enabled) {
    Settings::instance().setValue("lineWrapping", enabled);

    for (int i = 0; i < tabWidget->count(); ++i) {
        VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(i));
        if (editor) {
            editor->setLineWrapping(enabled);
        }
    }

    if (m_mainWindow) {
        m_mainWindow->statusBar()->showMessage(
            enabled ? "Line wrapping enabled" : "Line wrapping disabled", 2000);
    }
}

void VexWidget::updateCursorPosition() {
    VexEditor *editor = getCurrentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int col  = cursor.columnNumber() + 1;
        positionLabel->setText(QString("Line: %1, Col: %2").arg(line).arg(col));
    }
}

void VexWidget::showFindReplaceDialog() {
    if (!findDialog) {
        findDialog = new FindReplaceDialog(this);
        connect(findDialog, &FindReplaceDialog::findNextRequested, this, [this]() {
            currentFindText      = findDialog->findText();
            currentReplaceText   = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords    = findDialog->isWholeWords();
            findNext();
        });
        connect(findDialog, &FindReplaceDialog::findPreviousRequested, this, [this]() {
            currentFindText      = findDialog->findText();
            currentReplaceText   = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords    = findDialog->isWholeWords();
            findPrevious();
        });
        connect(findDialog, &FindReplaceDialog::replaceRequested, this, [this]() {
            currentFindText      = findDialog->findText();
            currentReplaceText   = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords    = findDialog->isWholeWords();
            replace();
        });
        connect(findDialog, &FindReplaceDialog::replaceAllRequested, this, [this]() {
            currentFindText      = findDialog->findText();
            currentReplaceText   = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords    = findDialog->isWholeWords();
            replaceAll();
        });
    }
    findDialog->setFindText(currentFindText);
    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

void VexWidget::findNext() {
    VexEditor *editor = getCurrentEditor();
    if (!editor || currentFindText.isEmpty()) return;

    QTextDocument::FindFlags flags = editor->getFindFlags(currentCaseSensitive, currentWholeWords);
    bool found = editor->find(currentFindText, flags);
    if (!found) {
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        editor->setTextCursor(cursor);
        editor->find(currentFindText, flags);
    }
}

void VexWidget::findPrevious() {
    VexEditor *editor = getCurrentEditor();
    if (!editor || currentFindText.isEmpty()) return;

    QTextDocument::FindFlags flags = editor->getFindFlags(currentCaseSensitive, currentWholeWords);
    flags |= QTextDocument::FindBackward;
    bool found = editor->find(currentFindText, flags);
    if (!found) {
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor->setTextCursor(cursor);
        editor->find(currentFindText, flags);
    }
}

void VexWidget::replace() {
    VexEditor *editor = getCurrentEditor();
    if (!editor || currentFindText.isEmpty()) return;

    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == currentFindText) {
        cursor.insertText(currentReplaceText);
    }
    findNext();
}

void VexWidget::replaceAll() {
    VexEditor *editor = getCurrentEditor();
    if (!editor || currentFindText.isEmpty()) return;

    QTextDocument::FindFlags flags = editor->getFindFlags(currentCaseSensitive, currentWholeWords);
    QTextCursor cursor = editor->document()->find(currentFindText, 0, flags);
    int replacements = 0;
    while (!cursor.isNull()) {
        cursor.insertText(currentReplaceText);
        ++replacements;
        cursor = editor->document()->find(currentFindText, cursor, flags);
    }
    if (m_mainWindow) {
        m_mainWindow->statusBar()->showMessage(QString("%1 replacements made").arg(replacements), 2000);
    }
}

void VexWidget::undo() {
    if (VexEditor *editor = getCurrentEditor()) {
        editor->undo();
    }
}

void VexWidget::redo() {
    if (VexEditor *editor = getCurrentEditor()) {
        editor->redo();
    }
}

void VexWidget::openTerminal() {
    QString workingDir = getCurrentWorkingDirectory();

#ifdef Q_OS_WIN
    QString nativePath = QDir::toNativeSeparators(workingDir);

    QString pwshPath = QStandardPaths::findExecutable("pwsh.exe");
    if (!pwshPath.isEmpty()) {
        QProcess::startDetached("cmd.exe",
                                QStringList() << "/c" << "start" << "pwsh.exe"
                                              << "-WorkingDirectory" << nativePath << "-NoExit");

        m_mainWindow->statusBar()->showMessage("Testing: start pwsh.exe", 2000);
        return;
    }

    QProcess::startDetached("cmd.exe",
                            QStringList() << "/c" << "start" << "cmd.exe" << "/k" << "cd" << "/d" << nativePath);

    m_mainWindow->statusBar()->showMessage("Testing: start cmd.exe", 2000);

#elif defined(Q_OS_MAC)

    QString script = QString("tell application \"Terminal\"\n"
                             "    do script \"cd '%1'\"\n"
                             "    activate\n"
                             "end tell").arg(workingDir);
    bool success = QProcess::startDetached("osascript", QStringList() << "-e" << script);

    if (success) {
        m_mainWindow->statusBar()->showMessage("Terminal (Terminal.app) opened in: " + workingDir, 2000);
        return;
    }

    m_mainWindow->statusBar()->showMessage("Failed to open terminal", 3000);

#else

    QString terminalUri = "terminal://" + workingDir;
    bool success = QProcess::startDetached("xdg-open", QStringList() << terminalUri);

    if (success) {
        m_mainWindow->statusBar()->showMessage("Default terminal opened in: " + workingDir, 2000);
        return;
    }

    const QList<QPair<QString, QStringList>> terminals = {
        {"konsole",        {"--workdir", workingDir}},
        {"gnome-terminal", {"--working-directory=" + workingDir}},
        {"xfce4-terminal", {"--working-directory=" + workingDir}},
        {"mate-terminal",  {"--working-directory=" + workingDir}},
        {"terminator",     {"--working-directory=" + workingDir}},
        {"alacritty",      {"--working-directory", workingDir}},
        {"kitty",          {"--directory", workingDir}},
        {"xterm",          {"-e", "bash", "-c", QString("cd '%1' && exec bash").arg(workingDir)}},
        {"urxvt",          {"-cd", workingDir}},
        {"rxvt",           {"-cd", workingDir}}
    };

    for (const auto &term : terminals) {
        QString termPath = QStandardPaths::findExecutable(term.first);
        if (!termPath.isEmpty()) {
            success = QProcess::startDetached(termPath, term.second, workingDir);
            if (success) {
                m_mainWindow->statusBar()->showMessage(
                    QString("Terminal (%1) opened in: %2").arg(term.first).arg(workingDir), 2000);
                return;
            }
        }
    }

    m_mainWindow->statusBar()->showMessage("Failed to open terminal", 3000);
#endif
}

void VexWidget::showAbout() {
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("About Vex");
    aboutDialog.setMinimumSize(500, 400);
    aboutDialog.setWindowIcon(Settings::instance().resolveIcon("vex"));

    QVBoxLayout *layout = new QVBoxLayout(&aboutDialog);

    QLabel *iconLabel = new QLabel;
    QPixmap iconPixmap = windowIcon().pixmap(170, 170);
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel *aboutLabel = new QLabel;
    aboutLabel->setTextFormat(Qt::RichText);
    aboutLabel->setWordWrap(true);
    aboutLabel->setAlignment(Qt::AlignCenter);
    aboutLabel->setStyleSheet("border: none; background: #020a00ff; padding: 12px; font-size: 13px;");
    aboutLabel->setText(R"(
<h2>Vex Editor v4.1</h2>
<p><i>An extensive Qt text editor.</i></p>
)");
    layout->addWidget(aboutLabel);

    QTabWidget *tabs = new QTabWidget;
    tabs->setTabPosition(QTabWidget::North);

    auto makeTab = [](const QString &html) {
        QLabel *label = new QLabel;
        label->setTextFormat(Qt::RichText);
        label->setText(html);
        label->setOpenExternalLinks(true);
        label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        label->setWordWrap(true);
        return label;
    };

    QMap<QString, QString> tabData = {
        { "About", R"(
<h3>⠀⠀ Version</h3>
<ul>
<li><b>⠀⠀Version:</b> 4.2</li>
<li><b>⠀⠀Status:</b> Cytoplasm (STABLE)</li>
<li><b>⠀⠀Release Date:</b> March 2026</li>
<li><b>⠀⠀Security Support Until:</b> 01/5/2027</li>
<li><b>⠀⠀Warranty:</b> Report bugs for issues</li>
</ul>
)"},
        { "Vi", R"(
<h3>⠀⠀ Vi Mode</h3>
<ul>
<li><b>⠀⠀h / j / k / l</b> → Move cursor</li>
<li><b>⠀⠀i / a</b> → Enter INSERT mode</li>
<li><b>⠀⠀x</b> → Delete character</li>
<li><b>⠀⠀o</b> → New line below</li>
<li><b>⠀⠀w / b</b> → Word forward/back</li>
<li><b>⠀⠀Shift + D</b> → Delete to end of line</li>
<li><b>⠀⠀: then, wq [:wq]</b> → Save file</li>
</ul>
)"},
        { "Insert", R"(
<h3>⠀⠀ Insert Mode</h3>
<ul>
<li><b>⠀⠀Ctrl + S</b> → Save file</li>
<li><b>⠀⠀Ctrl + Z / Y</b> → Undo / Redo</li>
<li><b>⠀⠀Home / End</b> → Line start/end</li>
</ul>
)"},
        { "App Shortcuts", R"(
<h3>⠀⠀App Shortcuts</h3>
<ul>
<li><b>  Esc → Change mode </li>
<li><b>⠀⠀Ctrl + N / O / S</b> → New / Open / Save</li>
<li><b>⠀⠀Ctrl + F</b> → Find & Replace</li>
<li><b>⠀⠀F3 / Shift + F3</b> → Find next / previous</li>
<li><b>⠀⠀Ctrl + Q</b> → Quit</li>
</ul>
)"},
        { "⠀⠀License", R"(
<h3>License</h3>
<hr>
<p>⠀⠀Vex Editor is licensed under the <strong>Apache License 2.0</strong>.</p>
<p>⠀⠀<a href='https://github.com/zynomon/vex/'>GitHub</a></p>
<h3>⠀⠀Author</h3>
<p><b>⠀⠀Zynomon aelius</b></p>
)"}
    };

    for (auto it = tabData.constBegin(); it != tabData.constEnd(); ++it)
        tabs->addTab(makeTab(it.value()), it.key());

    layout->addWidget(tabs);

    QPushButton *okBtn = new QPushButton("Close");
    okBtn->setFixedWidth(130);
    QObject::connect(okBtn, &QPushButton::clicked, &aboutDialog, &QDialog::accept);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    layout->addLayout(btnLayout);

    aboutDialog.exec();
}

void VexWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void VexWidget::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void VexWidget::dropEvent(QDropEvent *event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : std::as_const(urls)) {
        QString path = url.toLocalFile();
        if (QFileInfo(path).isFile()) {
            openFileAtPath(path);
        }
    }
    event->acceptProposedAction();
}

void VexWidget::closeEvent(QCloseEvent *event) {
    QString tempDir = Settings::basePath() + "/.temp";
    QString requestFile = tempDir + "/fileReq";

    bool hasUnsavedChanges = false;
    for (int i = 0; i < tabWidget->count(); ++i) {
        VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(i));
        if (editor && editor->document()->isModified()) {
            hasUnsavedChanges = true;
            break;
        }
    }

    if (hasUnsavedChanges) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Unsaved Changes");
        msgBox.setText("Looks like you are trying to exit while you have some progress unsaved.");
        msgBox.setInformativeText("Do you like to save this session for later opening or exit recursively?");
        msgBox.setIcon(QMessageBox::Question);

        QPushButton *saveSessionButton = msgBox.addButton("Save session and Quit", QMessageBox::AcceptRole);
        QPushButton *dontSaveButton    = msgBox.addButton("Quit",                  QMessageBox::DestructiveRole);
        QPushButton *cancelButton      = msgBox.addButton("Close",                 QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == saveSessionButton) {
            saveSessionAndQuit();
            if (QFile::exists(requestFile)) QFile::remove(requestFile);
            QDir dir(tempDir);
            if (dir.exists() && dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
                dir.rmdir(tempDir);
            event->accept();
        } else if (msgBox.clickedButton() == dontSaveButton) {
            saveToolbarState();
            saveSettings();
            if (QFile::exists(requestFile)) QFile::remove(requestFile);
            QDir dir(tempDir);
            if (dir.exists() && dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
                dir.rmdir(tempDir);
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        saveToolbarState();
        saveSettings();
        if (QFile::exists(requestFile)) QFile::remove(requestFile);
        QDir dir(tempDir);
        if (dir.exists() && dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty())
            dir.rmdir(tempDir);
        event->accept();
    }
}

void VexWidget::updateWindowTitle(QMainWindow *mainWin) {
    if (!mainWin) return;

    VexEditor *editor = getCurrentEditor();
    if (editor) {
        QString fileName = filePaths.value(editor);
        if (fileName.isEmpty()) {
            mainWin->setWindowTitle("Vex • New Draft file");
        } else {
            mainWin->setWindowTitle("Vex • " + QFileInfo(fileName).fileName());
        }
    } else {
        mainWin->setWindowTitle("Vex");
    }
}

void VexWidget::loadSettings() {
    VColors::initDefaults();
    Settings &settings = Settings::instance();

    bool lineWrapping = settings.get<bool>("lineWrapping", false);
    lineWrapAction->setChecked(lineWrapping);

    updateRecentMenu();

    QTimer::singleShot(100, this, &VexWidget::loadSavedSession);
}

void VexWidget::saveSettings() {
    Settings::instance().setValue("lineWrapping", lineWrapAction->isChecked());
}

void VexWidget::updateRecentMenu() {
    if (!recentMenu) return;
    recentMenu->clear();

    Settings &settings = Settings::instance();
    QStringList recentFiles = settings.get<QStringList>("recentFiles");

    for (int i = 0; i < recentFiles.size() && i < MAX_RECENT_FILES; ++i) {
        QString filePath = recentFiles[i];
        QFileInfo info(filePath);
        QString text = QString("%1. %2").arg(i + 1).arg(info.fileName());
        QAction *action = recentMenu->addAction(text);
        action->setToolTip(filePath);
        connect(action, &QAction::triggered, this, [this, filePath]() {
            openFileAtPath(filePath);
        });
    }

    if (recentFiles.isEmpty()) {
        QAction *empty = recentMenu->addAction("No recent files");
        empty->setEnabled(false);
    }
}

void VexWidget::updateTabAppearance(int tabIndex) {
    VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(tabIndex));
    if (!editor) return;

    QString filePath = filePaths.value(editor);
    QFileIconProvider iconProvider;
    QIcon fileIcon;

    if (filePath.isEmpty()) {
        fileIcon = iconProvider.icon(QFileIconProvider::File);
        tabWidget->setTabText(tabIndex, "Untitled");
        tabWidget->setTabToolTip(tabIndex, "Untitled");
    } else {
        QFileInfo info(filePath);
        fileIcon = iconProvider.icon(info);
        tabWidget->setTabText(tabIndex, info.fileName());
        tabWidget->setTabToolTip(tabIndex, info.absoluteFilePath());
    }

    bool isModified = editor->document()->isModified();
    if (isModified) {
        tabWidget->setTabIcon(tabIndex, QIcon(fileIcon.pixmap(16, 16, QIcon::Disabled)));
    } else {
        tabWidget->setTabIcon(tabIndex, fileIcon);
    }
}

bool VexWidget::hasBinaryContent(const QByteArray &data) const {
    if (data.isEmpty()) return false;

    int nullCount    = 0;
    int controlCount = 0;
    int printableCount = 0;
    int totalBytes   = qMin(data.size(), 8192);

    for (int i = 0; i < totalBytes; ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        if (c == 0) {
            nullCount++;
            if (nullCount > 5) return true;
        }
        if (c < 32 && c != 9 && c != 10 && c != 13) {
            controlCount++;
            if (controlCount > totalBytes * 0.05) return true;
        }
        if (c >= 32 && c <= 126) {
            printableCount++;
        }
    }

    double printableRatio = static_cast<double>(printableCount) / totalBytes;
    return printableRatio < 0.85;
}

VexEditor* VexWidget::getCurrentEditor() {
    return qobject_cast<VexEditor*>(tabWidget->currentWidget());
}

QString VexWidget::getCurrentWorkingDirectory() const {
    VexEditor *editor = const_cast<VexWidget*>(this)->getCurrentEditor();
    if (editor) {
        QString filePath = filePaths.value(editor);
        if (!filePath.isEmpty()) {
            return QFileInfo(filePath).absolutePath();
        }
    }
    return QDir::currentPath();
}

void VexWidget::onTabCountChanged(int count) {
    if (count == 0) {
        stackedWidget->setCurrentIndex(0);
    } else {
        stackedWidget->setCurrentIndex(1);
    }
}

void VexWidget::onLineEndingChanged() {
    VexEditor *editor = getCurrentEditor();
    if (editor) {
        editorLineEndings[editor] = m_lineEnding->type();
        editor->document()->setModified(true);
        updateTabAppearance(tabWidget->currentIndex());
    }
}

class VexCorePlugin : public QObject, public CorePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "vex.core/4.0")
    Q_INTERFACES(CorePlugin)

public:
    PluginMetadata meta() const override {
        PluginMetadata metadata;
        metadata.importance = PluginMetadata::Xylem;
        return metadata;
    }
    bool initialize(MainWindow* window, Settings* settings, CmdLine& cmdLine) override {
        QMainWindow *mainWin = reinterpret_cast<QMainWindow*>(window);

        cmdLine.addCommand({{"f", "file"}, "Open file(s) at startup", ""});

        QString tempDir = Settings::basePath() + "/.temp";
        QDir().mkpath(tempDir);
        QString requestFilePath = tempDir + "/fileReq";

        QFile requestFile(requestFilePath);

        if (requestFile.exists()) {
            QStringList filesToOpen = cmdLine.flagArgs("f");
            if (!filesToOpen.isEmpty()) {
                QFile requestFileWrite(requestFilePath);
                if (requestFileWrite.open(QIODevice::Append | QIODevice::Text)) {
                    QTextStream out(&requestFileWrite);
                    out.setEncoding(QStringConverter::Utf8);
                    for (const QString& path : std::as_const(filesToOpen)) {
                        QStringList splitPaths = path.split('\n', Qt::SkipEmptyParts);
                        for (const QString& p : std::as_const(splitPaths)) {
                            out << QDir::current().absoluteFilePath(p.trimmed()) << "\n";
                        }
                    }
                    requestFileWrite.close();
                }
            }

            QFileSystemWatcher *fallbackWatcher = new QFileSystemWatcher();
            fallbackWatcher->addPath(requestFilePath);

            bool *handled = new bool(false);

            QObject::connect(fallbackWatcher, &QFileSystemWatcher::fileChanged,
                             [requestFilePath, handled](const QString &path) {
                                 if (*handled) {
                                     return;
                                 }

                                 QFile file(requestFilePath);
                                 if (file.size() == 0) {
                                     *handled = true;
                                     qApp->quit();
                                 }
                             });

            QTimer::singleShot(10000, [requestFilePath, handled]() {
                if (*handled) {
                    return;
                }

                *handled = true;

                QString savedContent;
                QFile file(requestFilePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    in.setEncoding(QStringConverter::Utf8);
                    savedContent = in.readAll();
                    file.close();
                }

                QFile::remove(requestFilePath);

                QString appPath = QCoreApplication::applicationFilePath();
                QProcess::startDetached(appPath, QStringList());

                QTimer::singleShot(500, [requestFilePath, savedContent]() {
                    QFile newRequestFile(requestFilePath);
                    if (newRequestFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&newRequestFile);
                        out.setEncoding(QStringConverter::Utf8);
                        out << savedContent;
                        newRequestFile.close();
                    }
                });

                qApp->quit();
            });

            return false;
        }

        if (requestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            requestFile.close();
        }

        VexWidget *editor = new VexWidget(mainWin);
        mainWin->installEventFilter(editor);
        mainWin->setCentralWidget(editor);
        editor->setupUI(mainWin);
        editor->setupMenus(mainWin);
        editor->setupToolbar(mainWin);
        editor->loadSettings();
        editor->setAcceptDrops(true);

        QFileSystemWatcher *watcher = new QFileSystemWatcher(editor);
        watcher->addPath(requestFilePath);
        QObject::connect(watcher, &QFileSystemWatcher::fileChanged, editor,
                         [editor, requestFilePath]() {
                             QTimer::singleShot(100, [editor, requestFilePath]() {
                                 editor->handleInstanceRequest(requestFilePath);
                             });
                         });

        QTimer::singleShot(0, [editor, &cmdLine]() {
            QStringList files = cmdLine.flagArgs("f");
            for (const QString& filePath : std::as_const(files)) {
                QStringList splitPaths = filePath.split('\n', Qt::SkipEmptyParts);
                for (const QString& path : std::as_const(splitPaths)) {
                    editor->openFileAtPath(path.trimmed());
                }
            }
        });
        return true;
    }
};
#include "VexCore.moc"
