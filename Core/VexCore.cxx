/****************************************************************
*                                                              *
*                         Apache 2.0                           *
*     Copyright Zynomon aelius <zynomon@proton.me>  2026       *
*               Project         :        Vex                   *
*               Version         :        4.0 (Cytoplasm)       *
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
#include "Plugvex.H"
#include "Settings.H"

class VexEditor;
class LineNumberArea;
class VexWidget;

enum class EditorMode {
    Insert,
    Vi,
    Command
};

class VColors {
public:
    static QColor defaultLineNumBg() { return QColor(30, 30, 30); }
    static QColor defaultLineNumFg() { return QColor(100, 180, 100); }
    static QColor defaultHighlight() { return QColor(0, 60, 30, 102); }
    static int defaultLineWidth() { return 3; }

    static QColor getLineNumBg(QWidget* widget) {
        QVariant val = widget->property("lineNumberBg");
        return val.isValid() && val.canConvert<QColor>() ? val.value<QColor>() : defaultLineNumBg();
    }
    static QColor getLineNumFg(QWidget* widget) {
        QVariant val = widget->property("lineNumberFg");
        return val.isValid() && val.canConvert<QColor>() ? val.value<QColor>() : defaultLineNumFg();
    }
    static QColor getHighlightColor(QWidget* widget) {
        QVariant val = widget->property("lineHighlightColor");
        return val.isValid() && val.canConvert<QColor>() ? val.value<QColor>() : defaultHighlight();
    }
};

class VexEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    VexEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setVimMode(bool enabled);
    bool isVimMode() const;
    EditorMode getCurrentMode() const { return currentMode; }
    QString getModeString() const;
    QTextDocument::FindFlags getFindFlags(bool caseSensitive, bool wholeWords) const;
    void setLineWrapping(bool wrap);
    bool isLineWrapping() const { return lineWrapEnabled; }

signals:
    void modeChanged(const QString &mode);
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
    void highlightCurrentLine();

private:
    void setMode(EditorMode mode);
    void handleViModeKey(QKeyEvent *e);
    void handleInsertModeKey(QKeyEvent *e);
    void handleCommandModeKey(QKeyEvent *e);

    LineNumberArea *lineNumberArea;
    bool vimMode;
    EditorMode currentMode;
    QString currentCommandLine;
    bool lineWrapEnabled;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(VexEditor *editor) : QWidget(editor), codeEditor(editor) {
        setAutoFillBackground(true);
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
    , vimMode(false)
    , currentMode(EditorMode::Insert)
    , lineWrapEnabled(false)
{
    setObjectName("VexEditor");
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(40);
    if (!property("lineNumberBg").isValid()) setProperty("lineNumberBg", VColors::defaultLineNumBg());
    if (!property("lineNumberFg").isValid()) setProperty("lineNumberFg", VColors::defaultLineNumFg());
    if (!property("lineHighlightColor").isValid()) setProperty("lineHighlightColor", VColors::defaultHighlight());
    if (!property("lineNumberWidth").isValid()) setProperty("lineNumberWidth", VColors::defaultLineWidth());

    setStyleSheet(
        "QWidget#lineNumberArea {\n"
        "    background-color: qproperty-lineNumberBg;\n"
        "}\n"
        "QLabel#lineNumberText {\n"
        "    color: qproperty-lineNumberFg;\n"
        "}\n"
        );

    connect(this, &QPlainTextEdit::blockCountChanged, this, &VexEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &VexEditor::updateLineNumberArea);
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
    return property("lineNumberWidth").toInt() + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void VexEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), VColors::getLineNumBg(this));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    QTextCursor cursor = textCursor();
    int currentLine = cursor.blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            bool isCurrentLine = (blockNumber == currentLine);

            if (lineWrapEnabled && !isCurrentLine) {

                QTextLayout *layout = block.layout();
                if (layout && layout->lineCount() > 1) {

                    painter.setPen(VColors::getLineNumFg(this));
                    painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                                     Qt::AlignRight, "⏎");
                } else {
                    QString number = QString::number(blockNumber + 1);
                    painter.setPen(VColors::getLineNumFg(this));
                    painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                                     Qt::AlignRight, number);
                }
            } else {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(VColors::getLineNumFg(this));
                painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                                 Qt::AlignRight, number);
            }
        }
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void VexEditor::setVimMode(bool enabled) {
    vimMode = enabled;
    setMode(enabled ? EditorMode::Vi : EditorMode::Insert);
}

bool VexEditor::isVimMode() const {
    return vimMode;
}

QString VexEditor::getModeString() const {
    switch (currentMode) {
    case EditorMode::Insert: return "INSERT";
    case EditorMode::Vi: return "VI";
    case EditorMode::Command: return "COMMAND";
    default: return "UNKNOWN";
    }
}

void VexEditor::setMode(EditorMode mode) {
    currentMode = mode;
    emit modeChanged(getModeString());
}

void VexEditor::setLineWrapping(bool wrap) {
    lineWrapEnabled = wrap;
    setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    viewport()->update();
}

QTextDocument::FindFlags VexEditor::getFindFlags(bool caseSensitive, bool wholeWords) const {
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWords) flags |= QTextDocument::FindWholeWords;
    return flags;
}

void VexEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void VexEditor::keyPressEvent(QKeyEvent *e) {
    if (!vimMode) {
        QPlainTextEdit::keyPressEvent(e);
        return;
    }

    switch (currentMode) {
    case EditorMode::Insert:
        handleInsertModeKey(e);
        break;
    case EditorMode::Vi:
        handleViModeKey(e);
        break;
    case EditorMode::Command:
        handleCommandModeKey(e);
        break;
    }
}

void VexEditor::handleInsertModeKey(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        setMode(EditorMode::Vi);
        return;
    }
    QPlainTextEdit::keyPressEvent(e);
}

void VexEditor::handleViModeKey(QKeyEvent *e) {
    QTextCursor cursor = textCursor();

    if (e->key() == Qt::Key_I) {
        setMode(EditorMode::Insert);
        emit vimKeyPressed("i");
        return;
    }

    if (e->key() == Qt::Key_A) {
        setMode(EditorMode::Insert);
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        emit vimKeyPressed("a");
        return;
    }

    if (e->key() == Qt::Key_Colon) {
        setMode(EditorMode::Command);
        currentCommandLine = ":";
        emit commandLineChanged(currentCommandLine);
        return;
    }

    if (e->key() == Qt::Key_H) {
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
        emit vimKeyPressed("h");
        return;
    }

    if (e->key() == Qt::Key_J) {
        cursor.movePosition(QTextCursor::Down);
        setTextCursor(cursor);
        emit vimKeyPressed("j");
        return;
    }

    if (e->key() == Qt::Key_K) {
        cursor.movePosition(QTextCursor::Up);
        setTextCursor(cursor);
        emit vimKeyPressed("k");
        return;
    }

    if (e->key() == Qt::Key_L) {
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        emit vimKeyPressed("l");
        return;
    }

    if (e->key() == Qt::Key_W) {
        cursor.movePosition(QTextCursor::NextWord);
        setTextCursor(cursor);
        emit vimKeyPressed("w");
        return;
    }

    if (e->key() == Qt::Key_B) {
        cursor.movePosition(QTextCursor::PreviousWord);
        setTextCursor(cursor);
        emit vimKeyPressed("b");
        return;
    }

    if (e->key() == Qt::Key_X) {
        cursor.deleteChar();
        emit vimKeyPressed("x");
        return;
    }

    if (e->key() == Qt::Key_D) {
        if (e->modifiers() & Qt::ShiftModifier) {
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            emit vimKeyPressed("D");
        } else {
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
            emit vimKeyPressed("dd");
        }
        return;
    }

    if (e->key() == Qt::Key_Y) {
        cursor.select(QTextCursor::LineUnderCursor);
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(cursor.selectedText());
        cursor.clearSelection();
        emit vimKeyPressed("yy");
        return;
    }

    if (e->key() == Qt::Key_O) {
        cursor.movePosition(QTextCursor::EndOfLine);
        setTextCursor(cursor);
        insertPlainText("\n");
        setMode(EditorMode::Insert);
        emit vimKeyPressed("o");
        return;
    }

    if (e->key() == Qt::Key_W && e->modifiers() & Qt::ControlModifier) {
        emit saveRequested();
        emit vimKeyPressed("<C-w>");
        return;
    }

    QPlainTextEdit::keyPressEvent(e);
}

void VexEditor::handleCommandModeKey(QKeyEvent *e) {
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QString cmd = currentCommandLine.mid(1).trimmed();
        bool success = false;

        if (cmd == "w" || cmd == "write") {
            emit saveRequested();
            success = true;
        } else if (cmd == "q" || cmd == "quit") {
            window()->close();
            success = true;
        } else if (cmd == "wq") {
            emit saveRequested();
            window()->close();
            success = true;
        } else if (cmd == "q!") {
            window()->close();
            success = true;
        }

        emit commandExecuted(cmd, success);
        currentCommandLine.clear();
        setMode(EditorMode::Vi);
        emit commandLineChanged("");
        return;
    }

    if (e->key() == Qt::Key_Escape) {
        currentCommandLine.clear();
        setMode(EditorMode::Vi);
        emit commandLineChanged("");
        return;
    }

    if (e->key() == Qt::Key_Backspace && !currentCommandLine.isEmpty()) {
        currentCommandLine.chop(1);
        emit commandLineChanged(currentCommandLine);
        return;
    }

    if (!e->text().isEmpty()) {
        currentCommandLine += e->text();
        emit commandLineChanged(currentCommandLine);
        return;
    }
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
    setWindowIcon(Settings::resolveIcon("edit-find"));

    auto *formLayout = new QFormLayout;
    formLayout->addRow("Find:", findEdit);
    formLayout->addRow("Replace:", replaceEdit);

    auto *optionsLayout = new QHBoxLayout;
    optionsLayout->addWidget(caseCheckBox);
    optionsLayout->addWidget(wholeWordsCheckBox);
    optionsLayout->addStretch();

    auto *buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    findNextButton = buttonBox->addButton("Find &Next", QDialogButtonBox::ActionRole);
    findPrevButton = buttonBox->addButton("Find &Previous", QDialogButtonBox::ActionRole);
    replaceButton = buttonBox->addButton("&Replace", QDialogButtonBox::ActionRole);
    replaceAllButton = buttonBox->addButton("Replace &All", QDialogButtonBox::ActionRole);
    QPushButton *closeButton = buttonBox->addButton(QDialogButtonBox::Close);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(optionsLayout);
    mainLayout->addWidget(buttonBox);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    replaceButton->setEnabled(false);

    connect(findEdit, &QLineEdit::textChanged, this, &FindReplaceDialog::onFindTextChanged);
    connect(findNextButton, &QPushButton::clicked, this, &FindReplaceDialog::findNextRequested);
    connect(findPrevButton, &QPushButton::clicked, this, &FindReplaceDialog::findPreviousRequested);
    connect(replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceRequested);
    connect(replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceAllRequested);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
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
    for (const QString &path : absolutePaths) {
        if (QFile::exists(path)) {
            return QFileInfo(path).fileName();
        }
    }
    const QStringList names = {
        "konsole", "gnome-terminal", "xfce4-terminal",
        "mate-terminal", "terminator", "alacritty", "xterm"
    };
    for (const QString &name : names) {
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
        layout->setSpacing(10);

        QLabel *iconLabel = new QLabel(this);
        QIcon vexIcon = Settings::resolveIcon("vex");
        if (!vexIcon.isNull()) {
            iconLabel->setPixmap(vexIcon.pixmap(128, 128));
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
    void toggleVimMode(bool enabled);
    void toggleLineWrapping(bool enabled);
    void updateModeLabel(const QString &mode);
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
    QTabWidget *tabWidget;
    QLabel *modeLabel;
    QLabel *positionLabel;
    QLabel *vimHintLabel;
    QAction *vimModeAction;
    QAction *lineWrapAction;
    QMap<VexEditor*, QString> filePaths;
    FindReplaceDialog *findDialog;
    QString currentFindText;
    QString currentReplaceText;
    bool currentCaseSensitive;
    bool currentWholeWords;
    QFileSystemWatcher *fileWatcher;
    AdminFileHandler adminHandler;
    QMenu *recentMenu;
    EmptyStateView *emptyView;
    QMainWindow *m_mainWindow;
    bool m_sessionRestored;
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
{
    setAcceptDrops(true);
    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (m_mainWindow) {
            m_mainWindow->statusBar()->showMessage("File modified externally: " + QFileInfo(path).fileName(), 3000);
        }
    });
}

VexWidget::~VexWidget() {
    saveSettings();
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
        for (const QString &sessionPath : sessionFiles) {
            QFile file(sessionPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                in.setEncoding(QStringConverter::Utf8);

                QString originalPath = in.readLine();
                QString content = in.readAll();
                file.close();

                VexEditor *editor = new VexEditor(this);
                editor->setVimMode(vimModeAction->isChecked());
                editor->setLineWrapping(lineWrapAction->isChecked());
                editor->setPlainText(content);
                editor->document()->setModified(true);

                connect(editor, &VexEditor::modeChanged, this, &VexWidget::updateModeLabel);
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
                updateTabAppearance(index);
            }
            QFile::remove(sessionPath);
        }
    } else {
        for (const QString &sessionPath : sessionFiles) {
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
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString path = in.readLine().trimmed();
            if (!path.isEmpty()) {
                if (QFileInfo::exists(path)) {
                    openFileAtPath(path);
                } else if (m_mainWindow) {
                    m_mainWindow->statusBar()->showMessage("File not found: " + path, 3000);
                }
            }
        }
        file.close();

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.close();
        }

        if (m_mainWindow) {
            m_mainWindow->raise();
            m_mainWindow->activateWindow();
        }
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
    });

    onTabCountChanged(0);

    modeLabel = new QLabel("INSERT", mainWin);
    modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #2a5a2a; color: #d0f0d0; font-weight: bold; }");
    mainWin->statusBar()->addPermanentWidget(modeLabel);

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

    vimModeAction = viewMenu->addAction("&Vim Mode");
    vimModeAction->setCheckable(true);
    vimModeAction->setChecked(false);
    connect(vimModeAction, &QAction::toggled, this, &VexWidget::toggleVimMode);

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

    auto addAction = [&](const QString &label, const QString &iconName, const char *slot) {
        QIcon icon = Settings::resolveIcon(iconName);
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

    addAction("New", "document-new", SLOT(newFile()));
    addAction("Open", "document-open", SLOT(openFile()));
    addAction("Open by Name", "document-open", SLOT(openFileByName()));
    addAction("Save", "document-save", SLOT(saveFile()));
    toolbar->addSeparator();
    addAction("Undo", "edit-undo", SLOT(undo()));
    addAction("Redo", "edit-redo", SLOT(redo()));
    toolbar->addSeparator();
    addAction("Find", "edit-find", SLOT(showFindReplaceDialog()));
    addAction("Terminal", "utilities-terminal", SLOT(openTerminal()));
}

void VexWidget::newFile() {
    VexEditor *editor = new VexEditor(this);
    editor->setVimMode(vimModeAction->isChecked());
    editor->setLineWrapping(lineWrapAction->isChecked());

    connect(editor, &VexEditor::modeChanged, this, &VexWidget::updateModeLabel);
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

    QByteArray data = file.peek(8192);
    bool isBinary = hasBinaryContent(data);

    if (isBinary && !isWhitelisted) {
        file.close();

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Binary File Detected");
        msgBox.setText("This file appears to be binary. Vex Editor primarily supports text files.");
        msgBox.setInformativeText("Opening binary files may display unreadable content.");
        msgBox.setIcon(QMessageBox::Warning);

        QCheckBox *whitelistCheckBox = new QCheckBox("Mark this file as not binary");
        msgBox.setCheckBox(whitelistCheckBox);

        QPushButton *openButton = msgBox.addButton("Open Anyway", QMessageBox::AcceptRole);
        QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == cancelButton) {
            return;
        }

        if (msgBox.clickedButton() == openButton && whitelistCheckBox->isChecked()) {
            whitelistedFiles.append(filePath);
            settings.setValue("binaryWhitelist", whitelistedFiles);
        }

        if (!file.open(QFile::ReadOnly)) {
            QMessageBox::warning(this, "Error", "Cannot open file: " + filePath);
            return;
        }
        file.seek(0);
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    VexEditor *editor = new VexEditor(this);
    editor->setVimMode(vimModeAction->isChecked());
    editor->setLineWrapping(lineWrapAction->isChecked());
    editor->setPlainText(content);

    connect(editor, &VexEditor::modeChanged, this, &VexWidget::updateModeLabel);
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
    dialog.setWindowIcon(Settings::resolveIcon("document-open"));
    dialog.setMinimumSize(500, 350);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *iconArea = new QLabel(&dialog);
    iconArea->setFixedSize(100, 100);
    iconArea->setAlignment(Qt::AlignCenter);
    iconArea->setStyleSheet("border: 2px solid #aaa; border-radius: 8px; background: transparent;");

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
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << editor->toPlainText();
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
            if (adminHandler.saveWithAdmin(fileName, editor->toPlainText())) {
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
    }

    tabWidget->removeTab(index);
    updateWindowTitle(m_mainWindow);
    onTabCountChanged(tabWidget->count());
}

void VexWidget::toggleVimMode(bool enabled) {
    Settings::instance().setValue("vimMode", enabled);

    for (int i = 0; i < tabWidget->count(); ++i) {
        VexEditor *editor = qobject_cast<VexEditor*>(tabWidget->widget(i));
        if (editor) {
            editor->setVimMode(enabled);
        }
    }
    updateModeLabel(enabled ? "VI" : "INSERT");
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

void VexWidget::updateModeLabel(const QString &mode) {
    modeLabel->setText(mode);
    if (mode == "VI") {
        modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #2196F3; color: white; font-weight: bold; }");
    } else if (mode == "INSERT") {
        modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #2a5a2a; color: #d0f0d0; font-weight: bold; }");
    } else if (mode == "COMMAND") {
        modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #FF9800; color: white; font-weight: bold; }");
    }
}

void VexWidget::updateCursorPosition() {
    VexEditor *editor = getCurrentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        positionLabel->setText(QString("Line: %1, Col: %2").arg(line).arg(col));
    }
}

void VexWidget::showFindReplaceDialog() {
    if (!findDialog) {
        findDialog = new FindReplaceDialog(this);
        connect(findDialog, &FindReplaceDialog::findNextRequested, this, [this]() {
            currentFindText = findDialog->findText();
            currentReplaceText = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords = findDialog->isWholeWords();
            findNext();
        });
        connect(findDialog, &FindReplaceDialog::findPreviousRequested, this, [this]() {
            currentFindText = findDialog->findText();
            currentReplaceText = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords = findDialog->isWholeWords();
            findPrevious();
        });
        connect(findDialog, &FindReplaceDialog::replaceRequested, this, [this]() {
            currentFindText = findDialog->findText();
            currentReplaceText = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords = findDialog->isWholeWords();
            replace();
        });
        connect(findDialog, &FindReplaceDialog::replaceAllRequested, this, [this]() {
            currentFindText = findDialog->findText();
            currentReplaceText = findDialog->replaceText();
            currentCaseSensitive = findDialog->isCaseSensitive();
            currentWholeWords = findDialog->isWholeWords();
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
    QProcess::startDetached("cmd.exe", QStringList() << "/K" << "cd" << "/D" << workingDir);
#elif defined(Q_OS_MAC)
    QString script = QString("tell application \"Terminal\"\n"
                             "    do script \"cd '%1'\"\n"
                             "    activate\n"
                             "end tell").arg(workingDir);
    QProcess::execute("osascript", QStringList() << "-e" << script);
#else
    const QStringList terminals = {
        "konsole", "gnome-terminal", "xfce4-terminal",
        "mate-terminal", "terminator", "alacritty", "xterm"
    };
    for (const QString &term : terminals) {
        QString termPath = QStandardPaths::findExecutable(term);
        if (!termPath.isEmpty()) {
            if (term == "konsole") {
                QProcess::startDetached(termPath, QStringList() << "--workdir" << workingDir);
            } else if (term == "gnome-terminal") {
                QProcess::startDetached(termPath, QStringList() << "--working-directory=" + workingDir);
            } else if (term == "xfce4-terminal") {
                QProcess::startDetached(termPath, QStringList() << "--working-directory=" + workingDir);
            } else {
                QProcess::startDetached(termPath, QStringList(), workingDir);
            }
            return;
        }
    }
    QMessageBox::warning(this, "Error", "No terminal emulator found.");
#endif
}

void VexWidget::showAbout() {
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("About Vex");
    aboutDialog.setMinimumSize(500, 400);
    aboutDialog.setWindowIcon(Settings::resolveIcon("vex"));

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
<p><i>A fast Vim-inspired Qt text editor.</i></p>
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
<h3> Version</h3>
<ul>
<li><b> Version:</b> 4.0</li>
<li><b> Status:</b> Cytoplasm (STABLE)</li>
<li><b> Release Date:</b> March 2026</li>
<li><b> Security Support Until:</b> 01/5/2027</li>
<li><b> Warranty:</b> Report bugs for issues</li>
</ul>
)"},
        { "Vi", R"(
<h3> Vi Mode</h3>
<ul>
<li><b> h / j / k / l</b> → Move cursor</li>
<li><b> i / a</b> → Enter INSERT mode</li>
<li><b> x</b> → Delete character</li>
<li><b> o</b> → New line below</li>
<li><b> w / b</b> → Word forward/back</li>
<li><b> Shift + D</b> → Delete to end of line</li>
<li><b> : then, wq [:wq]</b> → Save file</li>
</ul>
)"},
        { "Insert", R"(
<h3> Insert Mode</h3>
<ul>
<li><b> Esc</b> → Return to VI mode</li>
<li><b> Ctrl + S</b> → Save file</li>
<li><b> Ctrl + Z / Y</b> → Undo / Redo</li>
<li><b> Home / End</b> → Line start/end</li>
</ul>
)"},
        { "App Shortcuts", R"(
<h3>App Shortcuts</h3>
<ul>
<li><b> Ctrl + N / O / S</b> → New / Open / Save</li>
<li><b> Ctrl + F</b> → Find & Replace</li>
<li><b> F3 / Shift + F3</b> → Find next / previous</li>
<li><b> Ctrl + Q</b> → Quit</li>
</ul>
)"},
        { "License", R"(
<h3> License</h3>
<p> Vex Editor is licensed under the <strong>Apache License 2.0</strong>.</p>
<p> <a href='https://github.com/zynomon/vex/'>GitHub</a></p>
<h3> Author</h3>
<p><b> Zynomon aelius</b></p>
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
    for (const QUrl &url : urls) {
        QString path = url.toLocalFile();
        if (QFileInfo(path).isFile()) {
            openFileAtPath(path);
        }
    }
    event->acceptProposedAction();
}

void VexWidget::closeEvent(QCloseEvent *event) {
    QString tempDir = Settings::basePath() + "/.temp";
    QString requestFile = tempDir + "/request";

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
        QPushButton *dontSaveButton = msgBox.addButton("Quit", QMessageBox::DestructiveRole);
        QPushButton *cancelButton = msgBox.addButton("Close", QMessageBox::RejectRole);

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
            mainWin->setWindowTitle("Vex - New Draft file");
        } else {
            mainWin->setWindowTitle("Vex - " + QFileInfo(fileName).fileName());
        }
    } else {
        mainWin->setWindowTitle("Vex");
    }
}

void VexWidget::loadSettings() {
    Settings &settings = Settings::instance();

    bool vimMode = settings.get<bool>("vimMode", false);
    vimModeAction->setChecked(vimMode);

    bool lineWrapping = settings.get<bool>("lineWrapping", false);
    lineWrapAction->setChecked(lineWrapping);

    updateRecentMenu();

    QTimer::singleShot(100, this, &VexWidget::loadSavedSession);
}

void VexWidget::saveSettings() {
    Settings &settings = Settings::instance();
    settings.setValue("vimMode", vimModeAction->isChecked());
    settings.setValue("lineWrapping", lineWrapAction->isChecked());
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

    int nullCount = 0;
    int controlCount = 0;
    int printableCount = 0;
    int totalBytes = qMin(data.size(), 8192);

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
    bool initialize(MainWindow* window, Settings* settings, CmdLine& cmdLine) {
        QMainWindow *mainWin = reinterpret_cast<QMainWindow*>(window);

        QString vtempdir = Settings::basePath() + "/.temp";
        QDir().mkpath(vtempdir);
        QString requestFilePath = vtempdir + "/request";

        QFile requestFile(requestFilePath);

        if (requestFile.exists()) {
            if (requestFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&requestFile);
                QStringList filesToOpen = cmdLine.flagArgs("file");
                if (filesToOpen.isEmpty()) filesToOpen = cmdLine.positionalArgs();

                for (const QString& path : filesToOpen) {
                    out << QDir::current().absoluteFilePath(path) << "\n";
                }
                requestFile.close();
            }
            return false;
        } else {
            if (requestFile.open(QIODevice::WriteOnly)) requestFile.close();
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
        QObject::connect(watcher, &QFileSystemWatcher::fileChanged, editor, [editor, requestFilePath]() {
            editor->handleInstanceRequest(requestFilePath);
        });

        cmdLine.addCommand({{"f", "file"}, "Open file(s) at startup", ""});

        QTimer::singleShot(0, [editor, &cmdLine]() {
            QStringList files = cmdLine.flagArgs("file");
            if (files.isEmpty()) files = cmdLine.positionalArgs();
            for (const QString& filePath : std::as_const(files)) {
                editor->openFileAtPath(filePath);
            }
        });

        return true;
    }};

#include "VexCore.moc"
