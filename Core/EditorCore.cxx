
/****************************************************************
*                                                              *
*                         Apache 2.0                           *
*     Copyright Zynomon aelius <zynomon@proton.me>  2026       *
*               Project         :        Vex                   *
*               Version         :        4.3 (Cytoplasm)       *
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
#include <QScrollArea>
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
        m_btn->setStyleSheet("");
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
        m_btn->setText("INS");
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
        m_btn->setText("CMD");
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
        if (!s.contains("theme/lineNumberWidth"))
            s.setValue("theme/lineNumberWidth", 3);
    }

    static QColor getLineNumBg(QWidget* widget) {
        QColor c = Settings::instance().get<QColor>("theme/lineNumberBg");
        if (c.isValid()) return c;

        QPalette pal = widget ? widget->palette() : QApplication::palette();
        QColor baseColor = pal.color(QPalette::Base);
        QColor bgColor(0x1e, 0x1e, 0x1e, 153);

        return bgColor;
    }

    static QColor getLineNumFg(QWidget* widget) {
        QColor c = Settings::instance().get<QColor>("theme/lineNumberFg");
        if (c.isValid()) return c;

        return QColor(255, 255, 255);
    }

    static QColor getHighlightColor(QWidget* widget) {
        QColor c = Settings::instance().get<QColor>("theme/lineHighlightColor");
        if (c.isValid()) return c;

        QPalette pal = widget ? widget->palette() : QApplication::palette();
        QColor highlightColor = pal.color(QPalette::Highlight);
        highlightColor.setAlpha(60);

        return highlightColor;
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
    QTextDocument::FindFlags getFindFlags(bool caseSensitive, bool wholeWords) const;
    void setLineWrapping(bool wrap);
    bool isLineWrapping() const { return lineWrapEnabled; }
    void setModeHandler(std::function<void(QPlainTextEdit*, QKeyEvent*)> handler) {
        m_modeHandler = handler;
    }
    void processInsertModeKey(QKeyEvent *e) {
        QPlainTextEdit::keyPressEvent(e);
    }

public slots:
    void highlightCurrentLine();

protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    LineNumberArea *lineNumberArea;
    bool            lineWrapEnabled;
    std::function<void(QPlainTextEdit*, QKeyEvent*)> m_modeHandler;
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

int VexEditor::lineNumberAreaWidth() {
    int digits = 1;
    int maxLines = qMax(1, document()->blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    int charWidth = fontMetrics().horizontalAdvance(QLatin1Char('9'));
    int numberWidth = charWidth * digits;
    int padding = charWidth;
    return VColors::getLineNumberWidth() + numberWidth + padding;
}
void VexEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), VColors::getLineNumBg(this));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    int areaWidth = lineNumberArea->width();
    int fmHeight = fontMetrics().height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(VColors::getLineNumFg(this));
            painter.drawText(0, top, areaWidth - 4, fmHeight,
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(blockNumber + 1));

            if (block.lineCount() > 1) {
                QColor wrapColor = VColors::getLineNumFg(this);
                wrapColor.setAlpha(100);
                painter.setPen(wrapColor);

                QTextLayout *layout = block.layout();
                for (int i = 1; i < block.lineCount(); ++i) {
                    QTextLine line = layout->lineAt(i);
                    int wrapTop = top + qRound(line.y());
                    painter.drawText(0, wrapTop, areaWidth - 4, fmHeight,
                                     Qt::AlignRight | Qt::AlignVCenter, "⏎");
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void VexEditor::setLineWrapping(bool wrap) {
    lineWrapEnabled = wrap;
    setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    viewport()->update();
}

QTextDocument::FindFlags VexEditor::getFindFlags(bool caseSensitive, bool wholeWords) const {
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWords)    flags |= QTextDocument::FindWholeWords;
    return flags;
}


void VexEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void VexEditor::keyPressEvent(QKeyEvent *e) {
    if (m_modeHandler)
        m_modeHandler(this, e);
    else
        QPlainTextEdit::keyPressEvent(e);
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
    friend class EditorCorePlugin;

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
    void showTabContextMenu(const QPoint &pos);
    void renameFile(int index);
    void showInFileManager();
    void openFileInNewWindow();
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
    void attachEditor(VexEditor *editor);

    QStackedWidget *stackedWidget;
    QTabWidget     *tabWidget;
    QPushButton    *modeLabel;
    QLabel         *positionLabel;
    QLabel         *vimHintLabel;
    QAction        *lineWrapAction;
    LineEnding     *m_lineEnding;
    Mode            m_mode;
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
        m_mainWindow->statusBar()->showMessage("saved.", 100);
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
                attachEditor(editor);
                editor->setPlainText(content);
                editor->document()->setModified(true);

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
void VexWidget::attachEditor(VexEditor *editor) {
    editor->setLineWrapping(lineWrapAction->isChecked());
    editor->setModeHandler([this](QPlainTextEdit *ed, QKeyEvent *e) {
        if (m_mode.current() == Mode::MODE_INS) {
            VexEditor *vexEditor = qobject_cast<VexEditor*>(ed);
            if (vexEditor) {
                vexEditor->processInsertModeKey(e);
            }
        } else {
            m_mode.handleKey(ed, e);
        }
    });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexWidget::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool) {
        int index = tabWidget->indexOf(editor);
        if (index != -1) {
            updateTabAppearance(index);
        }
    });
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
    tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabWidget, &QTabWidget::customContextMenuRequested, this, &VexWidget::showTabContextMenu);

    stackedWidget->addWidget(tabWidget);
    stackedWidget->setObjectName("VexStack");
    tabWidget->setObjectName("VexTab");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stackedWidget);

    modeLabel = new QPushButton(mainWin);
    modeLabel->setCursor(Qt::PointingHandCursor);
    mainWin->statusBar()->addPermanentWidget(modeLabel);

    m_mode.init(
        modeLabel,
        [this]() { saveFile(); },
        [this](const QString &k) {
            vimHintLabel->setText(k);
            QTimer::singleShot(800, this, [this]() { vimHintLabel->clear(); });
        },
        [this](const QString &c) { vimHintLabel->setText(c); },
        [this](const QString &c, bool s) {
            QString msg = s ? "Success: " + c : "Failed: " + c;
            if (m_mainWindow) m_mainWindow->statusBar()->showMessage(msg, 2000);
            vimHintLabel->clear();
        },
        [this](Mode::ModeEnum) { updateCursorPosition(); },
        [](QKeyEvent *e) { Q_UNUSED(e); }
        );
    m_mode.setupButton();

    m_lineEnding = new LineEnding(this);
    m_lineEnding->setupUi(mainWin->statusBar());
    connect(m_lineEnding, &LineEnding::lineEndingChanged, this, &VexWidget::onLineEndingChanged);

    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &VexWidget::closeTab);
    connect(tabWidget, &QTabWidget::currentChanged, [this, mainWin](int) {
        updateWindowTitle(mainWin);
        onTabCountChanged(tabWidget->count());
        VexEditor *editor = getCurrentEditor();
        if (editor) {
            LineEnding::Type type = editorLineEndings.value(editor, LineEnding::LF);
            m_lineEnding->setType(type);
            updateCursorPosition();
        } else {
            positionLabel->setText("Line: 0, Col: 0");
        }
    });
    onTabCountChanged(0);

    positionLabel = new QLabel("Line: 0, Col: 0", mainWin);
    mainWin->statusBar()->addPermanentWidget(positionLabel);

    vimHintLabel = new QLabel("", mainWin);
    vimHintLabel->setStyleSheet("QLabel { font-family: monospace; }");
    mainWin->statusBar()->addPermanentWidget(vimHintLabel);

    QTimer::singleShot(0, this, &VexWidget::restoreToolbarState);
}

void VexWidget::setupMenus(QMainWindow *mainWin) {
    QMenuBar* mb = mainWin->menuBar();

    QMenu* fileMenu = nullptr;
    QMenu* editMenu = nullptr;
    QMenu* viewMenu = nullptr;
    QMenu* helpMenu = nullptr;

    const QList<QAction*> acts = mb->actions();
    for (QAction* a : acts) {
        if (a->text() == "&File" || a->text() == "File") fileMenu = a->menu();
        else if (a->text() == "&Edit" || a->text() == "Edit") editMenu = a->menu();
        else if (a->text() == "&View" || a->text() == "View") viewMenu = a->menu();
        else if (a->text() == "&Help" || a->text() == "Help") helpMenu = a->menu();
    }

    if (!fileMenu) fileMenu = mb->addMenu("&File");
    if (!editMenu) editMenu = mb->addMenu("&Edit");
    if (!viewMenu) viewMenu = mb->addMenu("&View");
    if (!helpMenu) helpMenu = mb->addMenu("&Help");

    QAction *newAction = fileMenu->addAction(Settings::instance().resolveIcon("document-new"), "&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &VexWidget::newFile);

    QAction *openAction = fileMenu->addAction(Settings::instance().resolveIcon("folder-open"), "&Open");
    openAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(openAction, &QAction::triggered, this, &VexWidget::openFile);

    QAction *openByNameAction = fileMenu->addAction(Settings::instance().resolveIcon("document-open"), "Open &By Name...");
    openByNameAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(openByNameAction, &QAction::triggered, this, &VexWidget::openFileByName);

    fileMenu->addSeparator();

    recentMenu = fileMenu->addMenu("Open &Recent");
    recentMenu->setIcon(Settings::instance().resolveIcon("document-open-recent"));
    updateRecentMenu();

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction(Settings::instance().resolveIcon("document-save"), "&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &VexWidget::saveFile);

    QAction *saveAsAction = fileMenu->addAction(Settings::instance().resolveIcon("document-save-as"), "Save &As");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &VexWidget::saveFileAs);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(Settings::instance().resolveIcon("application-exit"), "E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, mainWin, &QWidget::close);

    QAction *restartAction = fileMenu->addAction(Settings::instance().resolveIcon("view-refresh"), "&Restart");
    restartAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(restartAction, &QAction::triggered, this, [this]() {
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
        qApp->quit();
    });

    QAction *undoAction = editMenu->addAction(Settings::instance().resolveIcon("edit-undo"), "&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &VexWidget::undo);

    QAction *redoAction = editMenu->addAction(Settings::instance().resolveIcon("edit-redo"), "&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &VexWidget::redo);

    editMenu->addSeparator();

    QAction *findAction = editMenu->addAction(Settings::instance().resolveIcon("edit-find"), "&Find and Replace");
    findAction->setShortcut(QKeySequence("Ctrl+F"));
    connect(findAction, &QAction::triggered, this, &VexWidget::showFindReplaceDialog);

    lineWrapAction = viewMenu->addAction("&Line Wrapping");
    lineWrapAction->setCheckable(true);
    lineWrapAction->setChecked(false);
    connect(lineWrapAction, &QAction::toggled, this, &VexWidget::toggleLineWrapping);

    QAction *aboutAction = helpMenu->addAction(Settings::instance().resolveIcon("vex"), "&About");
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
    addAction("Open",         "folder-open",     SLOT(openFile()));
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
    attachEditor(editor);

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
    attachEditor(editor);
    editor->setPlainText(content);

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
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            m_mode.changeMODE();
            return true;
        }
    }
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

    bool success = false;

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
    aboutDialog.setMinimumSize(550, 400);
    aboutDialog.setWindowIcon(Settings::instance().resolveIcon("vex"));

    QVBoxLayout *mainLayout = new QVBoxLayout(&aboutDialog);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout *contentLayout = new QHBoxLayout;

    QWidget *infoWidget = new QWidget;
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setAlignment(Qt::AlignCenter);

    QLabel *iconLabel = new QLabel;
    QPixmap iconPixmap = Settings::instance().resolveIcon("vex").pixmap(160, 160);
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *aboutLabel = new QLabel;
    aboutLabel->setTextFormat(Qt::RichText);
    aboutLabel->setWordWrap(true);
    aboutLabel->setAlignment(Qt::AlignCenter);
    aboutLabel->setText(R"(
<h3>Vex Editor v4.<sub>3</sub></h3>
<p><i>Cytoplasm (Revision)</i></p>
<p>Extensive Text Editor.</p>
)");

    infoLayout->addWidget(iconLabel);
    infoLayout->addWidget(aboutLabel);
    infoLayout->addStretch();

    QTabWidget *tabs = new QTabWidget;
    tabs->setTabPosition(QTabWidget::North);

    auto makeScrollingTab = [](const QString &html) {
        QScrollArea *scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        QLabel *label = new QLabel;
        label->setTextFormat(Qt::RichText);
        label->setText(html);
        label->setOpenExternalLinks(true);
        label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        label->setWordWrap(true);
        label->setContentsMargins(10, 10, 10, 10);
        scrollArea->setWidget(label);
        return scrollArea;
    };

    QMap<QString, QString> tabData = {
        { "About", R"(
<h3>Version</h3>
<ul>
<li><b>Version:</b> 4.3</li>
<li><b>Status:</b> Cytoplasm (STABLE)</li>
<li><b>Release Date:</b> March 2026</li>
<li><b>Security Support Until:</b> 01/5/2027</li>
<li><b>Warranty:</b> Report bugs for issues</li>
</ul>
)"},
        { "Vi", R"(
<h3>Vi Mode Keybindings</h3>
<ul>
<li><b>h</b> → Move cursor left</li>
<li><b>j</b> → Move cursor down</li>
<li><b>k</b> → Move cursor up</li>
<li><b>l</b> → Move cursor right</li>
<li><b>i</b> → Enter INSERT mode (before cursor)</li>
<li><b>a</b> → Enter INSERT mode (after cursor)</li>
<li><b>x</b> → Delete character under cursor</li>
<li><b>o</b> → New line below, enter INSERT mode</li>
<li><b>w</b> → Move to next word</li>
<li><b>b</b> → Move to previous word</li>
<li><b>dd</b> → Delete current line</li>
<li><b>D</b> → Delete to end of line (Shift+D)</li>
<li><b>yy</b> → Yank (copy) current line</li>
<li><b>:</b> → Enter COMMAND mode</li>
<li><b>Ctrl+W</b> → Save file</li>
</ul>
)"},
        { "CMD", R"(
<h3>Command Mode Commands</h3>
<ul>
<li><b>:w</b> or <b>:write</b> → Save file</li>
<li><b>:q</b> or <b>:quit</b> → Close tab</li>
<li><b>:wq</b> → Save and close tab</li>
<li><b>:q!</b> → Force close tab without saving</li>
<li><b>Backspace</b> → Delete last character</li>
<li><b>Enter</b> → Execute command</li>
</ul>
)"},
        { "INS", R"(
<h3>Insert Mode Keybindings</h3>
<ul>
<li><b>Ctrl+S</b> → Save file</li>
<li><b>Ctrl+Z</b> → Undo</li>
<li><b>Ctrl+Y</b> → Redo</li>
<li><b>Ctrl+A</b> → Select all</li>
<li><b>Ctrl+C</b> → Copy</li>
<li><b>Ctrl+V</b> → Paste</li>
<li><b>Ctrl+X</b> → Cut</li>
<li><b>Ctrl+F</b> → Find and Replace</li>
<li><b>Home/End</b> → Line start/end</li>
<li><b>Ctrl+Home/End</b> → Document start/end</li>
<li><b>Page Up/Down</b> → Scroll page</li>
</ul>
)"},
        { "Global", R"(
<h3>Application Shortcuts</h3>
<ul>
<li><b>Esc</b> → Cycle through mode (INS → VI → CMD → INS)</li>
<li><b>Ctrl+N</b> → New file</li>
<li><b>Ctrl+Shift+O</b> → Open file dialog</li>
<li><b>Ctrl+O</b> → Open file by name</li>
<li><b>Ctrl+S</b> → Save file</li>
<li><b>Ctrl+Shift+S</b> → Save file as</li>
<li><b>Ctrl+F</b> → Find and Replace</li>
<li><b>F3</b> → Find next</li>
<li><b>Shift+F3</b> → Find previous</li>
<li><b>Ctrl+Shift+R</b> → Restart</li>
<li><b>Ctrl+Q</b> → Quit</li>
<li><b>Ctrl+Z</b> → Undo</li>
<li><b>Ctrl+Y</b> → Redo</li>
<li><b>Drag & Drop</b> → Open files</li>
</ul>
)"},
        { "License", R"(
<h3>License</h3>
<hr>
<p>Vex Editor is licensed under the <strong>Apache License 2.0</strong>.</p>
<p><a href='https://github.com/zynomon/vex/'>GitHub Repository</a></p>
<h3>Author</h3>
<p><b>Zynomon aelius</b> [zynomon@proton.me]</p>
<h3>Copyright</h3>
<p>Copyright Zynomon aelius 2026</p>
)"}
    };

    for (auto it = tabData.constBegin(); it != tabData.constEnd(); ++it)
        tabs->addTab(makeScrollingTab(it.value()), it.key());

    contentLayout->addWidget(infoWidget);
    contentLayout->addWidget(tabs, 1);

    mainLayout->addLayout(contentLayout);

    QPushButton *okBtn = new QPushButton("Close");
    QObject::connect(okBtn, &QPushButton::clicked, &aboutDialog, &QDialog::accept);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    mainLayout->addLayout(btnLayout);

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
    QString tempDir = Settings::basePath() + "/.temp/";
    QString requestFile = tempDir + QString::number(QCoreApplication::applicationPid()) + ".Req";
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
    QFileIconProvider iconProvider;

    for (int i = 0; i < recentFiles.size() && i < MAX_RECENT_FILES; ++i) {
        QString filePath = recentFiles[i];
        QFileInfo info(filePath);
        QString text = QString("%2").arg(info.fileName());
        QAction *action = recentMenu->addAction(iconProvider.icon(info), text);
        action->setToolTip(filePath);
        connect(action, &QAction::triggered, this, [this, filePath]() {
            openFileAtPath(filePath);
        });
    }

    if (recentFiles.isEmpty()) {
        QAction *empty = recentMenu->addAction(Settings::instance().resolveIcon("dialog-warning"), "No recent files");
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
        QPixmap pixmap = fileIcon.pixmap(16, 16);
        QPixmap rotated(16, 16);
        rotated.fill(Qt::transparent);
        QPainter painter(&rotated);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(8, 8);
        painter.rotate(-15);
        painter.translate(-8, -8);
        painter.drawPixmap(0, 0, pixmap);
        painter.end();

        QIcon tiltedIcon(rotated);
        tabWidget->setTabIcon(tabIndex, tiltedIcon.pixmap(16, 16, QIcon::Disabled));
    } else {
        tabWidget->setTabIcon(tabIndex, fileIcon);
    }
}
void VexWidget::showTabContextMenu(const QPoint &pos)
{
    int tabIndex = tabWidget->tabBar()->tabAt(pos);
    if (tabIndex == -1) return;

    VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(tabIndex));
    if (!editor) return;

    QString filePath = filePaths.value(editor);
    bool fileExistsOnDisk = !filePath.isEmpty() && QFileInfo::exists(filePath);

    QMenu contextMenu(this);

    if (fileExistsOnDisk && editor->document()->isModified()) {
        QAction *saveAction = contextMenu.addAction(Settings::instance().resolveIcon("document-save"), "Save");
        connect(saveAction, &QAction::triggered, this, [this]() {
            saveFile();
        });
    }

    QAction *saveAsAction = contextMenu.addAction(Settings::instance().resolveIcon("document-save-as"), "Save As...");
    connect(saveAsAction, &QAction::triggered, this, [this]() {
        saveFileAs();
    });

    contextMenu.addSeparator();

    QAction *renameAction = contextMenu.addAction(Settings::instance().resolveIcon("edit-rename"), "Rename File");
    connect(renameAction, &QAction::triggered, this, [this, tabIndex]() {
        renameFile(tabIndex);
    });

    contextMenu.addSeparator();

    if (fileExistsOnDisk) {
        QAction *showAction = contextMenu.addAction(Settings::instance().resolveIcon("system-file-manager"), "Show in File Manager");
        connect(showAction, &QAction::triggered, this, [this]() {
            showInFileManager();
        });

        QAction *newWindowAction = contextMenu.addAction(Settings::instance().resolveIcon("window-new"), "Open in New Window");
        connect(newWindowAction, &QAction::triggered, this, [this]() {
            openFileInNewWindow();
        });
    }

    contextMenu.addSeparator();

    QAction *closeAction = contextMenu.addAction(Settings::instance().resolveIcon("window-close"), "Close Tab");
    connect(closeAction, &QAction::triggered, this, [this, tabIndex]() {
        closeTab(tabIndex);
    });

    contextMenu.exec(tabWidget->tabBar()->mapToGlobal(pos));
}

void VexWidget::renameFile(int index)
{
    VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(index));
    if (!editor) return;

    QString oldPath = filePaths.value(editor);
    QFileInfo oldInfo(oldPath);

    bool isNewFile = oldPath.isEmpty();
    QString currentName = isNewFile ? tabWidget->tabText(index) : oldInfo.fileName();

    if (currentName.endsWith(" *")) {
        currentName.chop(2);
    }

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename File", "Enter new file name:", QLineEdit::Normal, currentName, &ok);

    if (!ok || newName.isEmpty() || newName == currentName) return;

    if (isNewFile) {
        tabWidget->setTabText(index, newName);
        filePaths[editor] = newName;
        updateTabAppearance(index);
        updateWindowTitle(m_mainWindow);
        return;
    }

    QString newPath = oldInfo.absolutePath() + "/" + newName;
    QFile file(oldPath);

    if (file.rename(newPath)) {
        fileWatcher->removePath(oldPath);
        filePaths[editor] = newPath;
        fileWatcher->addPath(newPath);
        tabWidget->setTabText(index, QFileInfo(newPath).fileName());
        updateTabAppearance(index);
        updateWindowTitle(m_mainWindow);

        Settings &settings = Settings::instance();
        QStringList recentFiles = settings.get<QStringList>("recentFiles");
        recentFiles.replaceInStrings(oldPath, newPath);
        settings.setValue("recentFiles", recentFiles);
        updateRecentMenu();

        if (m_mainWindow) {
            m_mainWindow->statusBar()->showMessage("File renamed to: " + newPath, 3000);
        }
    } else {
        QMessageBox::warning(this, "Error", "Could not rename file:\n" + file.errorString());
    }
}

void VexWidget::showInFileManager()
{
    VexEditor *editor = getCurrentEditor();
    if (!editor) return;

    QString path = filePaths.value(editor);
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
    }
}

void VexWidget::openFileInNewWindow()
{
    VexEditor *editor = getCurrentEditor();
    if (!editor) return;

    QString path = filePaths.value(editor);
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                QStringList() << "-p" << "new" << "-f" << path);
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
class EditorCorePlugin : public QObject, public CorePlugin {
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
        Q_UNUSED(settings)
        m_mainWin = reinterpret_cast<QMainWindow*>(window);
        cmdLine.addCommand({{"f", "file"}, "Open file(s) at startup", ""});
        cmdLine.addCommand({{"p", "pid"}, "Send files to specific process ID, or 'new' for new window", "pid"});
        return true;
    }

    void afterParse(CmdLine& cmdLine) override {
        QStringList filesToOpen = cmdLine.flagArgs("f");
        QString pidArg = cmdLine.flagArgs("p").value(0, "");

        qint64 currentPid = QCoreApplication::applicationPid();
        QString tempDir = Settings::basePath() + "/.temp";
        QDir().mkpath(tempDir);
        m_requestFilePath = tempDir + "/" + QString::number(currentPid) + ".Req";

        if (pidArg == "new") {
            setupEditor();
            openFiles(filesToOpen);
            return;
        }

        if (!pidArg.isEmpty()) {
            bool ok;
            qint64 targetPid = pidArg.toLongLong(&ok);

            if (!ok || targetPid <= 0) {
                setupEditor();
                openFiles(filesToOpen);
                return;
            }

            if (isProcessAlive(targetPid)) {
                writeToRequestFile(targetPid, filesToOpen, tempDir);
                qApp->quit();
                return;
            }

            QFile::remove(tempDir + "/" + QString::number(targetPid) + ".Req");
            setupEditor();
            openFiles(filesToOpen);
            m_deadPidMessage = "PID " + pidArg + " is dead. Opening files in current window.";
            return;
        }

        if (trySendToExistingInstance(filesToOpen, tempDir, currentPid)) {
            qApp->quit();
            return;
        }

        setupEditor();
        openFiles(filesToOpen);
    }

private:
    QMainWindow* m_mainWin = nullptr;
    VexWidget* m_editor = nullptr;
    QString m_requestFilePath;
    QString m_deadPidMessage;

    void setupEditor() {
        QFile requestFile(m_requestFilePath);
        if (requestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            requestFile.close();
        }

        m_editor = new VexWidget(m_mainWin);
        m_mainWin->installEventFilter(m_editor);
        m_mainWin->setCentralWidget(m_editor);
        m_editor->setupUI(m_mainWin);
        m_editor->setupMenus(m_mainWin);
        m_editor->setupToolbar(m_mainWin);
        m_editor->loadSettings();
        m_editor->setAcceptDrops(true);

        QFileSystemWatcher *watcher = new QFileSystemWatcher(m_editor);
        watcher->addPath(m_requestFilePath);
        QObject::connect(watcher, &QFileSystemWatcher::fileChanged, m_editor,
                         [this]() { m_editor->handleInstanceRequest(m_requestFilePath); });

        connect(qApp, &QCoreApplication::aboutToQuit, [this]() {
            QFile::remove(m_requestFilePath);
        });

        if (!m_deadPidMessage.isEmpty()) {
            m_mainWin->statusBar()->showMessage(m_deadPidMessage, 5000);
            m_deadPidMessage.clear();
        }
    }

    void openFiles(const QStringList& files) {
        for (const QString& filePath : files) {
            m_editor->openFileAtPath(filePath);
        }
    }

    bool trySendToExistingInstance(const QStringList& filesToOpen, const QString& tempDir, qint64 currentPid) {
        if (filesToOpen.isEmpty()) return false;

        QDir dir(tempDir);
        const QStringList reqFiles = dir.entryList(QStringList() << "*.Req", QDir::Files);

        for (const QString& file : reqFiles) {
            qint64 pid = file.section('.', 0, 0).toLongLong();
            if (pid == currentPid) continue;

            if (!isProcessAlive(pid)) {
                QFile::remove(tempDir + "/" + file);
                continue;
            }

            writeToRequestFile(pid, filesToOpen, tempDir);
            return true;
        }
        return false;
    }
    bool isProcessAlive(qint64 pid) {
#ifdef Q_OS_WIN
        QProcess p;
        p.start("tasklist", QStringList() << "/FI" << QString("PID eq %1").arg(pid));
        p.waitForFinished();
        QString output = QString::fromLocal8Bit(p.readAllStandardOutput());
        return output.contains(QString::number(pid));
#else
        QProcess p;
        p.start("ps", QStringList() << "-p" << QString::number(pid));
        p.waitForFinished();
        return p.exitCode() == 0;
#endif
    }
    void writeToRequestFile(qint64 targetPid, const QStringList& filesToOpen, const QString& tempDir) {
        QString targetFile = tempDir + "/" + QString::number(targetPid) + ".Req";
        QFile requestFile(targetFile);
        if (requestFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&requestFile);
            out.setEncoding(QStringConverter::Utf8);
            for (const QString& path : filesToOpen) {
                out << QDir::current().absoluteFilePath(path) << "\n";
            }
            requestFile.close();
        }
    }


};


#include "EditorCore.moc"
