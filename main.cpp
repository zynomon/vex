#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>
#include <QFontDialog>
#include <QSettings>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QTextDocument>
#include <QTextCursor>
#include <QtSvg/QtSvg>
#include <QtSvgWidgets/QtSvgWidgets>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QProcess>
#include <QPluginLoader>
#include <QDirIterator>
#include <QSaveFile>
#include <QFileSystemWatcher>
#include <QInputDialog>
#include <QDesktopServices>

class LineNumberArea;
class VimTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    VimTextEdit(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setVimMode(bool enabled);
    bool isVimMode() const;
    QString getCurrentMode() const;
    QTextDocument::FindFlags getFindFlags(bool caseSensitive, bool wholeWords) const;
signals:
    void modeChanged(const QString &mode);
    void saveRequested();
protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
private:
    LineNumberArea *lineNumberArea;
    bool vimMode;
    bool insertMode;
    QString currentMode;
    QSyntaxHighlighter *highlighter = nullptr;
};

class SyntaxHighlighter : public QSyntaxHighlighter {
public:
    SyntaxHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {
        HighlightingRule rule;
        keywordFormat.setForeground(Qt::darkBlue);
        keywordFormat.setFontWeight(QFont::Bold);
        QStringList keywordPatterns = {
            "\\bclass\\b", "\\bconst\\b", "\\benum\\b", "\\bexplicit\\b",
            "\\bfriend\\b", "\\binline\\b", "\\bint\\b", "\\blong\\b",
            "\\bnamespace\\b", "\\boperator\\b", "\\bprivate\\b", "\\bprotected\\b",
            "\\bpublic\\b", "\\bshort\\b", "\\bsignals\\b", "\\bsigned\\b",
            "\\bslots\\b", "\\bstatic\\b", "\\bstruct\\b", "\\btemplate\\b",
            "\\btypedef\\b", "\\btypename\\b", "\\bunion\\b", "\\bunsigned\\b",
            "\\bvirtual\\b", "\\bvoid\\b", "\\bvolatile\\b", "\\bif\\b",
            "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\breturn\\b"
        };
        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }
        quotationFormat.setForeground(Qt::darkGreen);
        rule.pattern = QRegularExpression("\".*?\""); // Nonigg98iojhjkhbi89h every byte counts my guy
        rule.format = quotationFormat;
        highlightingRules.append(rule);
        singleLineCommentFormat.setForeground(Qt::gray);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = singleLineCommentFormat;
        highlightingRules.append(rule);
    }
protected:
    void highlightBlock(const QString &text) override {
        if (text.size() > 10000) return;
        for (const HighlightingRule &rule : highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat keywordFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat singleLineCommentFormat;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(VimTextEdit *editor) : QWidget(editor), codeEditor(editor) {}
    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }
protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }
private:
    VimTextEdit *codeEditor;
};

VimTextEdit::VimTextEdit(QWidget *parent)
    : QPlainTextEdit(parent), vimMode(false), insertMode(true), currentMode("INSERT") {
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(40);
    highlighter = new SyntaxHighlighter(document());
    highlighter->setParent(this);
    lineNumberArea = new LineNumberArea(this);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &VimTextEdit::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &VimTextEdit::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &VimTextEdit::highlightCurrentLine);
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int VimTextEdit::lineNumberAreaWidth() {
    int digits = 1;
    int maxLines = qMax(1, document()->blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    return 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void VimTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(30, 30, 30));
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(100, 180, 100));
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void VimTextEdit::setVimMode(bool enabled) {
    vimMode = enabled;
    if (enabled) {
        insertMode = false;
        currentMode = "NORMAL";
    } else {
        insertMode = true;
        currentMode = "INSERT";
    }
    emit modeChanged(currentMode);
}

bool VimTextEdit::isVimMode() const { return vimMode; }
QString VimTextEdit::getCurrentMode() const { return currentMode; }

QTextDocument::FindFlags VimTextEdit::getFindFlags(bool caseSensitive, bool wholeWords) const {
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWords) flags |= QTextDocument::FindWholeWords;
    return flags;
}

void VimTextEdit::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void VimTextEdit::keyPressEvent(QKeyEvent *e) {
    if (!vimMode) {
        QPlainTextEdit::keyPressEvent(e);
        return;
    }
    if (insertMode) {
        if (e->key() == Qt::Key_Escape) {
            insertMode = false;
            currentMode = "NORMAL";
            emit modeChanged(currentMode);
            return;
        }
        QPlainTextEdit::keyPressEvent(e);
        return;
    }
    QTextCursor cursor = textCursor();
    if (e->key() == Qt::Key_I) {
        insertMode = true;
        currentMode = "INSERT";
        emit modeChanged(currentMode);
    } else if (e->key() == Qt::Key_A) {
        insertMode = true;
        currentMode = "INSERT";
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        emit modeChanged(currentMode);
    } else if (e->key() == Qt::Key_H) {
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
    } else if (e->key() == Qt::Key_J) {
        cursor.movePosition(QTextCursor::Down);
        setTextCursor(cursor);
    } else if (e->key() == Qt::Key_K) {
        cursor.movePosition(QTextCursor::Up);
        setTextCursor(cursor);
    } else if (e->key() == Qt::Key_L) {
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
    } else if (e->key() == Qt::Key_X) {
        cursor.deleteChar();
    } else if (e->key() == Qt::Key_D && e->modifiers() & Qt::ShiftModifier) {
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    } else if (e->key() == Qt::Key_O) {
        cursor.movePosition(QTextCursor::EndOfLine);
        setTextCursor(cursor);
        insertPlainText("\n");
        insertMode = true;
        currentMode = "INSERT";
        emit modeChanged(currentMode);
    } else if (e->key() == Qt::Key_W) {
        cursor.movePosition(QTextCursor::NextWord);
        setTextCursor(cursor);
    } else if (e->key() == Qt::Key_B) {
        cursor.movePosition(QTextCursor::PreviousWord);
        setTextCursor(cursor);
    } else if (e->key() == Qt::Key_Colon) {
        currentMode = "COMMAND";
        emit modeChanged(currentMode);
    } else if (e->key() == Qt::Key_W && e->modifiers() & Qt::ControlModifier) {
        emit saveRequested();
    } else {
        QPlainTextEdit::keyPressEvent(e);
    }
}

void VimTextEdit::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void VimTextEdit::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void VimTextEdit::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(0, 60, 30, 102);
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
    explicit FindReplaceDialog(QWidget *parent = nullptr)
        : QDialog(parent)
        , findEdit(new QLineEdit(this))
        , replaceEdit(new QLineEdit(this))
        , caseCheckBox(new QCheckBox("Match &case", this))
        , wholeWordsCheckBox(new QCheckBox("&Whole words", this))
    {
        setWindowTitle("Find and Replace");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
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
        connect(findNextButton, &QPushButton::clicked, this, &FindReplaceDialog::findNextClicked);
        connect(findPrevButton, &QPushButton::clicked, this, &FindReplaceDialog::findPreviousClicked);
        connect(replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceClicked);
        connect(replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceAllClicked);
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    }
    QString findText() const { return findEdit->text(); }
    QString replaceText() const { return replaceEdit->text(); }
    bool isCaseSensitive() const { return caseCheckBox->isChecked(); }
    bool isWholeWords() const { return wholeWordsCheckBox->isChecked(); }
    void setFindText(const QString &text) {
        findEdit->setText(text);
        findEdit->selectAll();
        findEdit->setFocus();
    }
signals:
    void findNextRequested();
    void findPreviousRequested();
    void replaceRequested();
    void replaceAllRequested();
private slots:
    void onFindTextChanged(const QString &text) {
        replaceButton->setEnabled(!text.isEmpty());
    }
    void findNextClicked() { emit findNextRequested(); }
    void findPreviousClicked() { emit findPreviousRequested(); }
    void replaceClicked() { emit replaceRequested(); }
    void replaceAllClicked() { emit replaceAllRequested(); }
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

class VexPluginInterface {
public:
    virtual ~VexPluginInterface() = default;
    virtual QString name() const = 0;
    virtual void initialize(class VexEditor *editor) = 0;
    virtual void cleanup() {}
};
#define VexPluginInterface_iid "org.vex.VexPluginInterface"
Q_DECLARE_INTERFACE(VexPluginInterface, VexPluginInterface_iid)

class VexEditor : public QMainWindow {
    Q_OBJECT
public:
    VexEditor(QWidget *parent = nullptr);
    ~VexEditor();
    void openFileAtPath(const QString &filePath);
    void loadPlugins();
    void unloadPlugins();
    QTabWidget* getTabWidget() { return tabWidget; }
    QString getFilePath(VimTextEdit *editor) { return filePaths.value(editor); }
public slots:
    void findNext();
    void findPrevious();
    void replace();
    void replaceAll();
    void newFile();
    void applyTheme(const QString &qss);
    void setSystemTheme(const QString &styleName);
    void openFileByName();
    void openTerminal();
    void openPluginsFolder();
    void reloadPlugins();
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void loadTheme();
    void saveTheme(const QString &qss);
    void updateWindowTitle();
    bool isSystemPath(const QString &path) const;
private slots:
    void openFile();
    void saveFile();
    void saveFileAs();
    void closeTab(int index);
    void changeFont();
    void toggleVimMode(bool enabled);
    void updateModeLabel(const QString &mode);
    void updateCursorPosition();
    void showFindReplaceDialog();
    void showAbout();
    void loadSettings();
    void saveSettings();
    VimTextEdit* getCurrentEditor();
private:
    QTabWidget *tabWidget;
    QLabel *modeLabel;
    QLabel *positionLabel;
    QAction *vimModeAction;
    QAction *findAction;
    QMap<VimTextEdit*, QString> filePaths;
    FindReplaceDialog *findDialog = nullptr;
    QString currentFindText;
    QString currentReplaceText;
    bool currentCaseSensitive = false;
    bool currentWholeWords = false;
    QString currentTheme;
    QString defaultTheme;
    QFileSystemWatcher *fileWatcher;
    QList<VexPluginInterface*> plugins;
    QString pluginsDirPath;
    QMenu *pluginMenu = nullptr;
};

bool VexEditor::isSystemPath(const QString &path) const {
    if (path.startsWith("/home/")) return false;
    if (path.startsWith("/media/")) return false;
    if (path.startsWith("/run/media/")) return false;
    if (path.startsWith("/mnt/")) return false;
    return true;
}

void VexEditor::openFileAtPath(const QString &filePath) {
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) return;

    QFile file(filePath);
    if (file.open(QFile::ReadOnly)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        QString content = in.readAll();
        file.close();

        VimTextEdit *editor = new VimTextEdit(this);
        editor->setVimMode(vimModeAction->isChecked());
        editor->setPlainText(content);
        connect(editor, &VimTextEdit::modeChanged, this, &VexEditor::updateModeLabel);
        connect(editor, &VimTextEdit::saveRequested, this, &VexEditor::saveFile);
        connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexEditor::updateCursorPosition);
        int index = tabWidget->addTab(editor, QFileInfo(filePath).fileName());
        tabWidget->setCurrentIndex(index);
        filePaths[editor] = filePath;
        updateWindowTitle();
        fileWatcher->addPath(filePath);
        return;
    }

    if (!info.isReadable() && isSystemPath(filePath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Permission Denied",
            QString("You don't have permission to read:<br><b>%1</b><br><br>"
                    "Open with administrator privileges?").arg(filePath),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
            QStringList terminals = {"konsole", "gnome-terminal", "xfce4-terminal", "alacritty", "xterm"};
            QString terminal;
            for (const QString &t : terminals) {
                if (!QStandardPaths::findExecutable(t).isEmpty()) {
                    terminal = t;
                    break;
                }
            }
            if (terminal.isEmpty()) {
                QMessageBox::warning(this, "Error", "No terminal found to run sudo.");
                return;
            }
            QString appPath = QCoreApplication::applicationFilePath();
            QString cmd = QString("sudo %1 %2").arg(appPath, filePath);
            if (terminal == "konsole") {
                QProcess::startDetached("konsole", {"-e", "sh", "-c", cmd + "; read -p 'Press Enter to close...'"});
            } else if (terminal == "gnome-terminal") {
                QProcess::startDetached("gnome-terminal", {"--", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"});
            } else {
                QProcess::startDetached(terminal, {"-e", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"});
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "Could not open file:\n" + filePath);
    }
}

void VexEditor::saveFile() {
    VimTextEdit *editor = getCurrentEditor();
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
            statusBar()->showMessage("File saved: " + fileName, 3000);
            return;
        }
    }

    QFileInfo info(fileName);
    if (isSystemPath(fileName)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Save Failed",
            QString("Could not save:<br><b>%1</b><br><br>"
                    "Retry with administrator privileges?").arg(fileName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
            QStringList terminals = {"konsole", "gnome-terminal", "xfce4-terminal", "alacritty", "xterm"};
            QString terminal;
            for (const QString &t : terminals) {
                if (!QStandardPaths::findExecutable(t).isEmpty()) {
                    terminal = t;
                    break;
                }
            }
            if (terminal.isEmpty()) {
                QMessageBox::warning(this, "Error", "No terminal found to run sudo.");
                return;
            }
            QString appPath = QCoreApplication::applicationFilePath();
            QString cmd = QString("sudo %1 %2").arg(appPath, fileName);
            if (terminal == "konsole") {
                QProcess::startDetached("konsole", {"-e", "sh", "-c", cmd + "; read -p 'Press Enter to close...'"});
            } else if (terminal == "gnome-terminal") {
                QProcess::startDetached("gnome-terminal", {"--", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"});
            } else {
                QProcess::startDetached(terminal, {"-e", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"});
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "Could not save file:\n" + fileName);
    }
}

void VexEditor::findNext() {
    VimTextEdit *editor = getCurrentEditor();
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

void VexEditor::findPrevious() {
    VimTextEdit *editor = getCurrentEditor();
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

void VexEditor::replace() {
    VimTextEdit *editor = getCurrentEditor();
    if (!editor || currentFindText.isEmpty()) return;
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == currentFindText) {
        cursor.insertText(currentReplaceText);
    }
    findNext();
}

void VexEditor::replaceAll() {
    VimTextEdit *editor = getCurrentEditor();
    if (!editor || currentFindText.isEmpty()) return;
    QTextDocument::FindFlags flags = editor->getFindFlags(currentCaseSensitive, currentWholeWords);
    QTextCursor cursor = editor->document()->find(currentFindText, 0, flags);
    int replacements = 0;
    while (!cursor.isNull()) {
        cursor.insertText(currentReplaceText);
        ++replacements;
        cursor = editor->document()->find(currentFindText, cursor, flags);
    }
    statusBar()->showMessage(QString("%1 replacements made").arg(replacements), 2000);
}

void VexEditor::closeEvent(QCloseEvent *event) {
    for (int i = 0; i < tabWidget->count(); ++i) {
        VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(i));
        if (editor && editor->document()->isModified()) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Unsaved Changes",
                QString("Tab \"%1\" has unsaved changes. Save before closing?").arg(tabWidget->tabText(i)),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
                );
            if (reply == QMessageBox::Save) {
                tabWidget->setCurrentIndex(i);
                saveFile();
            } else if (reply == QMessageBox::Cancel) {
                event->ignore();
                return;
            }
        }
    }
    event->accept();
}

void VexEditor::setupUI() {
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    setCentralWidget(tabWidget);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &VexEditor::closeTab);
    modeLabel = new QLabel("INSERT", this);
    modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #2a5a2a; color: #d0f0d0; font-weight: bold; }");
    statusBar()->addPermanentWidget(modeLabel);
    positionLabel = new QLabel("Line: 1, Col: 1", this);
    statusBar()->addPermanentWidget(positionLabel);
    connect(tabWidget, &QTabWidget::currentChanged, this, &VexEditor::updateWindowTitle);
}

void VexEditor::setupMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &VexEditor::newFile);
    QAction *openAction = fileMenu->addAction("&Open");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &VexEditor::openFile);
    QAction *openByNameAction = fileMenu->addAction("Open &By Name...");
    openByNameAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(openByNameAction, &QAction::triggered, this, &VexEditor::openFileByName);
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &VexEditor::saveFile);
    QAction *saveAsAction = fileMenu->addAction("Save &As");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &VexEditor::saveFileAs);
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu("&Edit");
    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, [this]() {
        if (getCurrentEditor()) getCurrentEditor()->undo();
    });
    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, [this]() {
        if (getCurrentEditor()) getCurrentEditor()->redo();
    });
    editMenu->addSeparator();
    findAction = editMenu->addAction("&Find and Replace");
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &VexEditor::showFindReplaceDialog);

    QMenu *viewMenu = menuBar()->addMenu("&View");
    QAction *fontAction = viewMenu->addAction("&Font");
    connect(fontAction, &QAction::triggered, this, &VexEditor::changeFont);
    vimModeAction = viewMenu->addAction("&Vim Mode");
    vimModeAction->setCheckable(true);
    vimModeAction->setChecked(false);
    connect(vimModeAction, &QAction::toggled, this, &VexEditor::toggleVimMode);

    QMenu *themeMenu = viewMenu->addMenu("Themes");
    QAction *vexDefaultAction = themeMenu->addAction("Vex Default");
    connect(vexDefaultAction, &QAction::triggered, this, [this]() {
        applyTheme(defaultTheme);
        saveTheme(defaultTheme);
    });
    QStringList styles = QStyleFactory::keys();
    QAction *fusionAction = themeMenu->addAction("System: Fusion");
    connect(fusionAction, &QAction::triggered, this, [this]() {
        setSystemTheme("Fusion");
    });
    if (styles.contains("Breeze")) {
        QAction *breezeAction = themeMenu->addAction("System: Breeze");
        connect(breezeAction, &QAction::triggered, this, [this]() {
            setSystemTheme("Breeze");
        });
    }
    QAction *qtDefaultAction = themeMenu->addAction("System: Qt Default");
    connect(qtDefaultAction, &QAction::triggered, this, [this]() {
        QApplication::setStyle(QStyleFactory::create(""));
        qApp->setStyleSheet("");
        currentTheme = "";
    });
    themeMenu->addSeparator();
    QAction *themeEditorAction = themeMenu->addAction("Theme Editor");
    connect(themeEditorAction, &QAction::triggered, this, [this]() {
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i) == "Theme Editor") {
                tabWidget->setCurrentIndex(i);
                return;
            }
        }
        VimTextEdit *editor = new VimTextEdit(this);
        editor->setPlainText(currentTheme.isEmpty() ? defaultTheme : currentTheme);
        editor->setVimMode(false);
        connect(editor, &QPlainTextEdit::textChanged, this, [this, editor]() {
            applyTheme(editor->toPlainText());
        });
        int index = tabWidget->addTab(editor, "Theme Editor");
        tabWidget->setCurrentIndex(index);
        QWidget *buttonWidget = new QWidget();
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
        QPushButton *saveBtn = new QPushButton("Save Theme");
        QPushButton *resetBtn = new QPushButton("Reset to Default");
        buttonLayout->addWidget(saveBtn);
        buttonLayout->addWidget(resetBtn);
        buttonLayout->addStretch();
        statusBar()->addPermanentWidget(buttonWidget);
        connect(saveBtn, &QPushButton::clicked, this, [this, editor, buttonWidget]() {
            saveTheme(editor->toPlainText());
            statusBar()->removeWidget(buttonWidget);
            delete buttonWidget;
            QMessageBox::information(this, "Saved", "Theme saved to ~/.config/VexEditor/theme.qss");
        });
        connect(resetBtn, &QPushButton::clicked, this, [this, editor, buttonWidget]() {
            editor->setPlainText(defaultTheme);
            applyTheme(defaultTheme);
            statusBar()->removeWidget(buttonWidget);
            delete buttonWidget;
        });
    });

    pluginMenu = viewMenu->addMenu("Extensions");
    QAction *openPluginsDirAction = pluginMenu->addAction("Open Plugins Folder");
    connect(openPluginsDirAction, &QAction::triggered, this, &VexEditor::openPluginsFolder);
    QAction *reloadPluginsAction = pluginMenu->addAction("Reload Plugins");
    connect(reloadPluginsAction, &QAction::triggered, this, &VexEditor::reloadPlugins);

    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &VexEditor::showAbout);
}

void VexEditor::setupToolbar() {
    QToolBar *toolbar = addToolBar("Main");
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto addThemedAction = [&](const QString &label, const QString &iconName, const QObject *receiver, const char *slot) {
        QIcon icon = QIcon::fromTheme(iconName);
        QAction *action = new QAction(icon, label, this);
        connect(action, SIGNAL(triggered()), receiver, slot);
        toolbar->addAction(action);
        return action;
    };
    addThemedAction("Url", "document-open-remote", this, SLOT(openFileByName()));
    addThemedAction("New", "document-new", this, SLOT(newFile()));
    addThemedAction("Open file", "document-open", this, SLOT(openFile()));
    addThemedAction("Save", "document-save", this, SLOT(saveFile()));
    toolbar->addSeparator();
    addThemedAction("Undo", "edit-undo", this, SLOT(undo()));
    addThemedAction("Redo", "edit-redo", this, SLOT(redo()));
    toolbar->addSeparator();
    QAction *findReplaceAction = toolbar->addAction(
        QIcon::fromTheme("edit-find-replace", QIcon::fromTheme("edit-find")),
        "Find and Replace"
        );
    findReplaceAction->setShortcut(QKeySequence::Find);
    findReplaceAction->setToolTip("Find and Replace (Ctrl+H)");
    connect(findReplaceAction, &QAction::triggered, this, &VexEditor::showFindReplaceDialog);
    addThemedAction("Open in Terminal", "utilities-terminal", this, SLOT(openTerminal()));
    connect(toolbar, &QToolBar::orientationChanged, this, [toolbar](Qt::Orientation orientation) {
        toolbar->setToolButtonStyle(
            orientation == Qt::Vertical ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon
            );
    });
}

void VexEditor::applyTheme(const QString &qss) {
    currentTheme = qss;
    qApp->setStyleSheet(qss);
}

void VexEditor::setSystemTheme(const QString &styleName) {
    QApplication::setStyle(QStyleFactory::create(styleName));
    qApp->setStyleSheet("");
    currentTheme = "";
}

void VexEditor::loadTheme() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/VexEditor";
    QDir().mkpath(configDir);
    QFile file(configDir + "/theme.qss");
    if (file.open(QFile::ReadOnly)) {
        QString savedTheme = file.readAll();
        file.close();
        if (savedTheme.contains("url(")) {
            QMessageBox::warning(this, "Unsafe Theme", "External resources not allowed.");
            applyTheme(defaultTheme);
            return;
        }
        if (savedTheme.size() > 200000) {
            QMessageBox::warning(this, "Theme too large", "Invalid theme size.");
            applyTheme(defaultTheme);
            return;
        }
        applyTheme(savedTheme);
        return;
    }
    applyTheme(defaultTheme);
}

void VexEditor::saveTheme(const QString &qss) {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/VexEditor";
    QDir().mkpath(configDir);
    QString path = configDir + "/theme.qss";
    QString backup = path + ".bak";
    QFile::remove(backup);
    QFile::copy(path, backup);
    QFile file(path);
    if (file.open(QFile::WriteOnly)) {
        file.write(qss.toUtf8());
        file.close();
    }
}

void VexEditor::newFile() {
    VimTextEdit *editor = new VimTextEdit(this);
    editor->setVimMode(vimModeAction->isChecked());
    connect(editor, &VimTextEdit::modeChanged, this, &VexEditor::updateModeLabel);
    connect(editor, &VimTextEdit::saveRequested, this, &VexEditor::saveFile);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexEditor::updateCursorPosition);
    int index = tabWidget->addTab(editor, "Untitled");
    tabWidget->setCurrentIndex(index);
    filePaths[editor] = QString();
    updateWindowTitle();
}

void VexEditor::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", QString(),
                                                    "All Files (*)");
    if (!fileName.isEmpty()) {
        openFileAtPath(fileName);
    }
}

void VexEditor::openFileByName() {
    bool ok;
    QString fileName = QInputDialog::getText(this, "Open File by Name", "Enter file path:", QLineEdit::Normal, "", &ok);
    if (ok && !fileName.isEmpty()) {
        openFileAtPath(fileName);
    }
}

void VexEditor::openTerminal() {
    const QStringList terminals = {
        "/usr/bin/konsole",
        "/usr/bin/gnome-terminal",
        "/usr/bin/xfce4-terminal",
        "/usr/bin/alacritty",
        "/usr/bin/x-terminal-emulator"
    };
    for (const QString &term : terminals) {
        if (QFile::exists(term)) {
            QProcess::startDetached(term);
            return;
        }
    }
    QMessageBox::warning(this, "Error", "No terminal emulator found.");
}

void VexEditor::saveFileAs() {
    VimTextEdit *editor = getCurrentEditor();
    if (!editor) return;
    if (!editor->document()->isModified() && filePaths.value(editor).isEmpty()) return;
    QString fileName = QFileDialog::getSaveFileName(this, "Save File", QString(),
                                                    "All Files (*)");
    if (!fileName.isEmpty()) {
        filePaths[editor] = fileName;
        QFileInfo fileInfo(fileName);
        tabWidget->setTabText(tabWidget->currentIndex(), fileInfo.fileName());
        saveFile();
    }
}

void VexEditor::closeTab(int index) {
    VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(index));
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
    if (tabWidget->count() == 0) {
        newFile();
    }
    updateWindowTitle();
}

void VexEditor::changeFont() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, getCurrentEditor()->font(), this);
    if (ok) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(i));
            if (editor) {
                editor->setFont(font);
            }
        }
    }
}

void VexEditor::toggleVimMode(bool enabled) {
    for (int i = 0; i < tabWidget->count(); ++i) {
        VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(i));
        if (editor) {
            editor->setVimMode(enabled);
        }
    }
    updateModeLabel(enabled ? "NORMAL" : "INSERT");
}

void VexEditor::updateModeLabel(const QString &mode) {
    modeLabel->setText(mode);
    if (mode == "NORMAL") {
        modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #2196F3; color: white; font-weight: bold; }");
    } else if (mode == "INSERT") {
        modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #2a5a2a; color: #d0f0d0; font-weight: bold; }");
    } else if (mode == "COMMAND") {
        modeLabel->setStyleSheet("QLabel { padding: 2px 10px; background-color: #FF9800; color: white; font-weight: bold; }");
    }
}

void VexEditor::updateCursorPosition() {
    VimTextEdit *editor = getCurrentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        positionLabel->setText(QString("Line: %1, Col: %2").arg(line).arg(col));
    }
}

void VexEditor::showFindReplaceDialog() {
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

void VexEditor::showAbout() {
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("About Vex");
    aboutDialog.setMinimumSize(500, 400);
    QVBoxLayout *layout = new QVBoxLayout(&aboutDialog);
    auto createIconLabel = [this]() -> QLabel* {
        QPixmap iconPixmap;
        QSvgRenderer renderer(QStringLiteral(":/vex.svgz"));
        if (renderer.isValid()) {
            QImage image(170, 170, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            renderer.render(&painter);
            painter.end();
            iconPixmap = QPixmap::fromImage(image);
        } else {
            iconPixmap = windowIcon().pixmap(170, 170);
        }
        QLabel *label = new QLabel;
        label->setPixmap(iconPixmap);
        label->setAlignment(Qt::AlignCenter);
        return label;
    };
    layout->addWidget(createIconLabel());
    QLabel *aboutLabel = new QLabel;
    aboutLabel->setTextFormat(Qt::RichText);
    aboutLabel->setWordWrap(true);
    aboutLabel->setAlignment(Qt::AlignCenter);
    aboutLabel->setStyleSheet("border: none; background: transparent; padding: 12px; font-size: 13px;");
    aboutLabel->setText(R"(
        <h2>Vex Editor v1.0</h2>
        <p><i>A fast and minimal, Vim-inspired Qt text editor.</i></p>
    )");
    layout->addWidget(aboutLabel);
    QTabWidget *tabs = new QTabWidget;
    tabs->setTabPosition(QTabWidget::North);
    auto makeTab = [](const QString &html) {
        QTextBrowser *browser = new QTextBrowser;
        browser->setHtml(html);
        browser->setOpenExternalLinks(true);
        browser->setStyleSheet("border: none; padding: 8px; background: transparent;");
        return browser;
    };
    QMap<QString, QString> tabData = {
        { "About", R"(
            <h3>Version</h3>
            <ul>
                <li><b>Version:</b> 1.0.0</li>
                <li><b>Status:</b> Emerald (Stable)</li>
                <li><b>Release Date:</b> October 2025</li>
                <li><b>Security Support Until:</b> 2026</li>
                <li><b>Warranty:</b> None</li>
            </ul>
        )"},
        { "Normal", R"(
            <h3>Normal Mode</h3>
            <ul>
                <li><b>h / j / k / l</b> ￢ﾆﾒ Move cursor</li>
                <li><b>i / a</b> ￢ﾆﾒ Enter INSERT mode</li>
                <li><b>x</b> ￢ﾆﾒ Delete character</li>
                <li><b>o</b> ￢ﾆﾒ New line below</li>
                <li><b>w / b</b> ￢ﾆﾒ Word forward/back</li>
                <li><b>Shift + D</b> ￢ﾆﾒ Delete to end of line</li>
                <li><b>Ctrl + W</b> ￢ﾆﾒ Save file</li>
            </ul>
        )"},
        { "Insert", R"(
            <h3>Insert Mode</h3>
            <ul>
                <li><b>Esc</b> ￢ﾆﾒ Return to NORMAL mode</li>
                <li><b>Ctrl + S</b> ￢ﾆﾒ Save file</li>
                <li><b>Ctrl + Z / Y</b> ￢ﾆﾒ Undo / Redo</li>
                <li><b>Home / End</b> ￢ﾆﾒ Line start/end</li>
            </ul>
        )"},
        { "App Shortcuts", R"(
            <h3>App Shortcuts</h3>
            <ul>
                <li><b>Ctrl + N / O / S</b> ￢ﾆﾒ New / Open / Save</li>
                <li><b>Ctrl + H</b> ￢ﾆﾒ Find & Replace</li>
                <li><b>F3 / Shift + F3</b> ￢ﾆﾒ Find next / previous</li>
                <li><b>Ctrl + Q</b> ￢ﾆﾒ Quit</li>
            </ul>
        )"},
        { "License", R"(
            <h3>License</h3>
            <p>Vex Editor is licensed under the <b>Apache License 2.0</b>.</p>
            <p><a href='https://www.apache.org/licenses/LICENSE-2.0'>Apache LICENSE-2.0</a></p>
            <h3>Author</h3>
            <p><b>Zynomon aelius</b></p>
        )"}
    };
    for (auto it = tabData.constBegin(); it != tabData.constEnd(); ++it)
        tabs->addTab(makeTab(it.value()), it.key());
    layout->addWidget(tabs);
    QPushButton *okBtn = new QPushButton("OK I'm satisfied");
    okBtn->setFixedWidth(130);
    QObject::connect(okBtn, &QPushButton::clicked, &aboutDialog, &QDialog::accept);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    layout->addLayout(btnLayout);
    aboutDialog.exec();
}

void VexEditor::loadSettings() {
    QSettings settings("VexEditor", "Vex");
    restoreGeometry(settings.value("geometry").toByteArray());
    bool vimMode = settings.value("vimMode", false).toBool();
    vimModeAction->setChecked(vimMode);
}

void VexEditor::saveSettings() {
    QSettings settings("VexEditor", "Vex");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("vimMode", vimModeAction->isChecked());
}

VimTextEdit* VexEditor::getCurrentEditor() {
    return qobject_cast<VimTextEdit*>(tabWidget->currentWidget());
}

void VexEditor::updateWindowTitle() {
    VimTextEdit *editor = getCurrentEditor();
    if (editor) {
        QString fileName = filePaths.value(editor);
        if (fileName.isEmpty()) {
            setWindowTitle("Vex - Untitled");
        } else {
            QFileInfo fileInfo(fileName);
            setWindowTitle("Vex - " + fileInfo.fileName());
        }
    } else {
        setWindowTitle("Vex");
    }
}

void VexEditor::loadPlugins() {
    unloadPlugins();
    pluginsDirPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                     + "/VexEditor/plugins";
    QDir().mkpath(pluginsDirPath);
    QDir dir(pluginsDirPath);
    if (!dir.exists()) return;

    QStringList loadedNames;
    const QStringList entries = dir.entryList({"*.so"}, QDir::Files);
    for (const QString &fileName : entries) {
        QString fullPath = dir.absoluteFilePath(fileName);
        QPluginLoader loader(fullPath);
        QObject *pluginObj = loader.instance();
        if (pluginObj) {
            VexPluginInterface *plugin = qobject_cast<VexPluginInterface*>(pluginObj);
            if (plugin) {
                plugin->initialize(this);
                plugins.append(plugin);
                loadedNames << plugin->name();

                QAction *action = pluginMenu->addAction(plugin->name());
                action->setCheckable(true);
                action->setChecked(true);
            }
        }
    }

    if (!loadedNames.isEmpty()) {
        statusBar()->showMessage("Loaded plugins: " + loadedNames.join(", "), 3000);
    }
}

void VexEditor::unloadPlugins() {
    for (auto plugin : plugins) {
        plugin->cleanup();
    }
    qDeleteAll(plugins);
    plugins.clear();

    QList<QAction*> actions = pluginMenu->actions();
    for (int i = 2; i < actions.size(); ++i) {
        pluginMenu->removeAction(actions[i]);
        delete actions[i];
    }
}

void VexEditor::reloadPlugins() {
    loadPlugins();
}

void VexEditor::openPluginsFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(pluginsDirPath));
}

VexEditor::VexEditor(QWidget *parent) : QMainWindow(parent) {
    QIcon appIcon(QStringLiteral(":/vex.svgz"));
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
    }
    setWindowTitle("Vex");
    resize(1000, 700);
    defaultTheme =
        "QMainWindow { background-color: #0a0a0a; color: #d0f0d0; }"
        "QMenuBar { background-color: #1a1a1a; color: #a0e0a0; border-bottom: 1px solid rgba(255, 255, 255, 30); }"
        "QMenu { background-color: #1a1a1a; color: #a0e0a0; border: 1px solid rgba(255, 255, 255, 20); padding: 4px; }"
        "QMenu::item { padding: 6px 20px; }"
        "QMenu::item:selected { background-color: #2a5a2a; color: white; }"
        "QToolBar { background-color: #1a1a1a; border-bottom: 1px solid rgba(255, 255, 255, 30); spacing: 4px; padding: 4px; }"
        "QStatusBar { background-color: #1a1a1a; color: #a0e0a0; border-top: 1px solid rgba(255, 255, 255, 20); }"
        "QTabWidget::pane { border: 0; background-color: #0a0a0a; }"
        "QTabBar::tab { background: #2a2a2a; color: #a0e0a0; padding: 8px 16px; margin-right: 2px; border: 1px solid rgba(255, 255, 255, 15); border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background: #0a0a0a; color: #d0f0d0; border: 1px solid rgba(255, 255, 255, 30); border-bottom: none; }"
        "QPlainTextEdit { background-color: #0a0a0a; color: #d0f0d0; selection-background-color: #2a5a2a; border: 1px solid rgba(255, 255, 255, 20); padding: 6px; }"
        "QLabel { color: #a0e0a0; }"
        "QLineEdit { background: #1a1a1a; color: #d0f0d0; border: 1px solid rgba(255, 255, 255, 25); padding: 4px; border-radius: 3px; }"
        "QLineEdit:focus { border: 1px solid rgba(160, 224, 160, 80); }"
        "QCheckBox { color: #a0e0a0; }"
        "QPushButton { background-color: #2a2a2a; color: #a0e0a0; border: 1px solid rgba(255, 255, 255, 20); padding: 4px 10px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
        "QPushButton:pressed { background-color: #2a5a2a; color: white; }";
    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        QMessageBox::information(this, "File Changed", "The file has been modified externally.");
    });
    setupUI();
    setupMenus();
    setupToolbar();
    loadSettings();
    loadTheme();
    loadPlugins();
}

VexEditor::~VexEditor() {
    qDeleteAll(plugins);
    saveSettings();
}

class VexApplication : public QApplication {
public:
    VexApplication(int &argc, char **argv) : QApplication(argc, argv) {}
protected:
    bool notify(QObject *receiver, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_F3) {
                if (VexEditor *editor = qobject_cast<VexEditor*>(activeWindow())) {
                    if (keyEvent->modifiers() & Qt::ShiftModifier) {
                        editor->findPrevious();
                    } else {
                        editor->findNext();
                    }
                    return true;
                }
            }
        }
        return QApplication::notify(receiver, event);
    }
};

int main(int argc, char *argv[]) {
    VexApplication app(argc, argv);
    app.setApplicationName("Vex");
    app.setOrganizationName("VexEditor");
    app.setApplicationVersion("1.0");
    VexEditor editor;
    bool hasOpenedFile = false;
    for (int i = 1; i < argc; ++i) {
        QString path = QString::fromLocal8Bit(argv[i]);
        if (QFileInfo(path).isFile()) {
            editor.openFileAtPath(path);
            hasOpenedFile = true;
        }
    }
    if (!hasOpenedFile) {
        editor.newFile();
    }
    editor.show();
    return app.exec();
}

#include "main.moc"
