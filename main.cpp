/****************************************************************
 *               YO best up looking the code ?                  *
 *                                                              *
 *                         Apache 2.0                           *
 *     Copyright Zynomon aelius <zynomon@proton.me>  2025       *
 *               Project         :        Vex                   *
 *               Version         :        3.0 (Mint Leaf)       *
 ****************************************************************/

#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMimeData>
#include <QBuffer>
#include <QFileDialog>
#include <QStringListModel>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QCompleter>
#include <QFileSystemModel>
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
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QTextDocument>
#include <QTextCursor>
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
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QStringConverter>
#include <QPushButton>
#include <QTimer>
#include <QClipboard>

class VimTextEdit;
class LineNumberArea;
class VexEditor;
class VimTextEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    VimTextEdit(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setVimMode(bool enabled);
    bool isVimMode() const;
    QSyntaxHighlighter *highlighter = nullptr;
    QString getCurrentMode() const;
    QTextDocument::FindFlags getFindFlags(bool caseSensitive, bool wholeWords) const;
    QSyntaxHighlighter *getHighlighter() const { return highlighter; }
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
    QString syntaxRulesFilePath() const;
    LineNumberArea *lineNumberArea;
    bool vimMode;
    bool insertMode;
    QString currentMode = "INSERT";
    QString currentCommandLine;
    void loadConfigFile(QString &content, const QString &configPath, const QString &qrcPath);
};
class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    struct ValidationError {
        int lineNumber;
        QString message;
        QString line;
    };

    struct Rule {
        enum Type { ExactString, Heredoc };
        Type type;
        QString language;
        QTextCharFormat format;
        QRegularExpression startPattern;
        QRegularExpression endPattern;
        bool isEOLComment = false;
    };

    explicit SyntaxHighlighter(QTextDocument *parent = nullptr)
        : QSyntaxHighlighter(parent)
        , activeHeredocRule(-1)
    {
        QFile res(":/syntax.vex");
        if (res.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString text = QString::fromUtf8(res.readAll());
            res.close();
            loadRules(text);
        }
    }

    void setLanguage(const QString &lang) {
        currentLanguage = lang;
        rehighlight();
    }

    void loadRules(const QString &rulesText) {
        rules.clear();
        extensionToLanguage.clear();
        startStringToLanguage.clear();
        validationErrors.clear();
        currentLanguage = "default";
        activeHeredocRule = -1;

        const QStringList lines = rulesText.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            const QString raw = lines[i];
            const QString line = raw.trimmed();
            const int lineNum = i + 1;
            if (line.isEmpty())
                continue;


            if (line.startsWith("REG = \"")) {
                int start = line.indexOf('\"') + 1;
                int end   = line.indexOf('\"', start);
                if (end == -1) {
                    addError(lineNum, "REG missing closing quote", raw);
                    currentLanguage = "default";
                } else {
                    currentLanguage = line.mid(start, end - start);
                }
                continue;
            }


            if (line.startsWith("File =")) {
                QString exts = line.mid(6).trimmed();
                const QStringList parts = exts.split("&&", Qt::SkipEmptyParts);
                for (QString ext : parts) {
                    ext = ext.trimmed();
                    if (ext.isEmpty())
                        continue;
                    if (!ext.startsWith('.'))
                        ext.prepend('.');
                    extensionToLanguage[ext] = currentLanguage;
                }
                continue;
            }


            if (line.startsWith("Sw =")) {
                QString rest = line.mid(4).trimmed();
                const QStringList parts = rest.split("&&", Qt::SkipEmptyParts);
                for (QString p : parts) {
                    p = p.trimmed();
                    QString content = extractESContent(p);
                    if (!content.isEmpty())
                        startStringToLanguage[content] = currentLanguage;
                }
                continue;
            }


            if (line.contains('=') && (line.contains("+ES'") || line.contains("+HS'"))) {
                int eqPos = line.indexOf('=');
                if (eqPos <= 0 || eqPos >= line.size() - 1) {
                    addError(lineNum, "Malformed rule (no attributes)", raw);
                    continue;
                }

                QString patternSection = line.left(eqPos).trimmed();
                QString attrSection    = line.mid(eqPos + 1).trimmed();

                QTextCharFormat fmt = parseAttributes(attrSection, lineNum, raw);
                if (!fmt.foreground().color().isValid()) {

                }

                const QStringList patterns = patternSection.split("&&", Qt::SkipEmptyParts);
                for (QString p : patterns) {
                    p = p.trimmed();
                    if (p.contains("+ES'")) {
                        QString content = extractESContent(p);
                        if (content.isEmpty())
                            continue;
                        Rule r;
                        r.type = Rule::ExactString;
                        r.language = currentLanguage;
                        r.format = fmt;
                        r.startPattern = QRegularExpression(QRegularExpression::escape(content));
                        rules.append(r);
                    } else if (p.contains("+HS'") && p.contains("+HE'") && p.contains("to")) {
                        QString startDelim, endDelim;
                        if (!extractHSHE(p, startDelim, endDelim)) {
                            addError(lineNum, "Malformed HS/HE pattern", raw);
                            continue;
                        }
                        Rule r;
                        r.type = Rule::Heredoc;
                        r.language = currentLanguage;
                        r.format = fmt;
                        r.startPattern = QRegularExpression(QRegularExpression::escape(startDelim));
                        if (endDelim.isEmpty()) {
                            r.endPattern = QRegularExpression("$");
                            r.isEOLComment = true;
                        } else {
                            r.endPattern = QRegularExpression(QRegularExpression::escape(endDelim));
                            r.isEOLComment = false;
                        }
                        rules.append(r);
                    }
                }
                continue;
            }
        }

        emit validationCompleted(!validationErrors.isEmpty(), validationErrors);
        rehighlight();
    }

    QList<ValidationError> getValidationErrors() const { return validationErrors; }
    const QMap<QString, QString> &getExtensionToLanguage() const { return extensionToLanguage; }
    const QMap<QString, QString> &getStartStringToLanguage() const { return startStringToLanguage; }

signals:
    void validationCompleted(bool hasErrors, const QList<ValidationError> &errors);

protected:
    void highlightBlock(const QString &text) override {
        if (text.isEmpty() || text.size() > 10000) {
            setCurrentBlockState(-1);
            return;
        }

        const int prevState = previousBlockState();


        if (prevState >= 0 && prevState < rules.size()) {
            const Rule &hr = rules[prevState];
            if (hr.type == Rule::Heredoc &&
                (hr.language == currentLanguage || hr.language == "default")) {

                if (hr.isEOLComment) {

                    setFormat(0, text.length(), hr.format);
                    setCurrentBlockState(-1);
                    activeHeredocRule = -1;
                    return;
                } else {
                    QRegularExpressionMatch endMatch = hr.endPattern.match(text);
                    if (endMatch.hasMatch()) {
                        int endPos = endMatch.capturedEnd();
                        setFormat(0, endPos, hr.format);
                        setCurrentBlockState(-1);
                        activeHeredocRule = -1;
                        return;
                    } else {

                        setFormat(0, text.length(), hr.format);
                        setCurrentBlockState(prevState);
                        activeHeredocRule = prevState;
                        return;
                    }
                }
            }
        }


        for (int i = 0; i < rules.size(); ++i) {
            const Rule &r = rules[i];
            if (r.type != Rule::Heredoc)
                continue;
            if (r.language != currentLanguage && r.language != "default")
                continue;

            QRegularExpressionMatch startMatch = r.startPattern.match(text);
            if (!startMatch.hasMatch())
                continue;

            const int startPos = startMatch.capturedStart();
            if (r.isEOLComment) {

                setFormat(startPos, text.length() - startPos, r.format);
                setCurrentBlockState(-1);
                activeHeredocRule = -1;
                return;
            } else {
                QRegularExpressionMatch endMatch =
                    r.endPattern.match(text, startPos + startMatch.capturedLength());
                if (endMatch.hasMatch()) {
                    int endPos = endMatch.capturedEnd();
                    setFormat(startPos, endPos - startPos, r.format);
                    setCurrentBlockState(-1);
                    activeHeredocRule = -1;
                    return;
                } else {
                    setFormat(startPos, text.length() - startPos, r.format);
                    setCurrentBlockState(i);
                    activeHeredocRule = i;
                    return;
                }
            }
        }


        for (const Rule &r : rules) {
            if (r.type != Rule::ExactString)
                continue;
            if (r.language != currentLanguage && r.language != "default")
                continue;

            QRegularExpressionMatchIterator it = r.startPattern.globalMatch(text);
            while (it.hasNext()) {
                const QRegularExpressionMatch m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), r.format);
            }
        }

        setCurrentBlockState(-1);
        activeHeredocRule = -1;
    }

private:
    QVector<Rule> rules;
    QString currentLanguage;
    QMap<QString, QString> extensionToLanguage;
    QMap<QString, QString> startStringToLanguage;
    QList<ValidationError> validationErrors;


    int activeHeredocRule;

    void addError(int lineNum, const QString &msg, const QString &line) {
        ValidationError e{lineNum, msg, line};
        validationErrors.append(e);
    }

    QTextCharFormat parseAttributes(const QString &attrSection,
                                    int lineNum,
                                    const QString &rawLine) {
        QTextCharFormat fmt;
        const QStringList parts = attrSection.split("=", Qt::SkipEmptyParts);
        for (QString part : parts) {
            part = part.trimmed();
            if (part.startsWith("color:")) {
                const QString c = part.mid(6).trimmed();
                QColor col(c);
                if (!col.isValid())
                    addError(lineNum, "Invalid color: " + c, rawLine);
                fmt.setForeground(col);
            } else if (part.startsWith("font:")) {
                const QString f = part.mid(5).trimmed();
                if (f == "B") {
                    fmt.setFontWeight(QFont::Bold);
                } else if (f == "I") {
                    fmt.setFontItalic(true);
                } else if (f == "BI") {
                    fmt.setFontWeight(QFont::Bold);
                    fmt.setFontItalic(true);
                }
            }
        }
        return fmt;
    }


    QString extractESContent(const QString &pattern) {
        int start = pattern.indexOf("+ES'");
        if (start < 0)
            return QString();
        start += 4;
        int end = pattern.indexOf("ES-", start);
        if (end < 0 || end <= start)
            return QString();
        return pattern.mid(start, end - start);
    }


    bool extractHSHE(const QString &pattern,
                     QString &startDelim,
                     QString &endDelim) {

        QRegularExpression re(R"(\+HS'([^']*)'HS-\s*to\s*\+HE'([^']*)'HE-)");
        QRegularExpressionMatch m = re.match(pattern);
        if (m.hasMatch()) {
            startDelim = m.captured(1);
            endDelim   = m.captured(2);
            return true;
        }

        int hsStart = pattern.indexOf("+HS'");
        if (hsStart < 0)
            return false;
        hsStart += 4;
        int hsEnd = pattern.indexOf("HS-", hsStart);
        if (hsEnd < 0 || hsEnd <= hsStart)
            return false;
        startDelim = pattern.mid(hsStart, hsEnd - hsStart);

        int heStart = pattern.indexOf("+HE'", hsEnd);
        if (heStart < 0) {
            endDelim.clear();
            return true;
        }
        heStart += 4;
        int heEnd = pattern.indexOf("HE-", heStart);
        if (heEnd < 0 || heEnd <= heStart) {
            endDelim.clear();
            return true;
        }
        endDelim = pattern.mid(heStart, heEnd - heStart);
        return true;
    }
};
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(VimTextEdit *editor) : QWidget(editor), codeEditor(editor) {
        setAutoFillBackground(true);
    }

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
    : QPlainTextEdit(parent), vimMode(false), insertMode(true), lineNumberArea(new LineNumberArea(this)) {
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(40);
    highlighter = new SyntaxHighlighter(document());
    highlighter->setParent(this);
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
        currentMode = "VI";
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
            currentMode = "VI";
            emit modeChanged(currentMode);
            return;
        }
        QPlainTextEdit::keyPressEvent(e);
        return;
    }
    if (currentMode == "COMMAND") {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            QString cmd = currentCommandLine.mid(1).trimmed();
            bool success = false;
            QString msg;
            if (cmd == "w" || cmd == "write") {
                emit saveRequested();
                success = true;
                msg = "File saved";
            } else if (cmd == "q" || cmd == "quit") {
                parentWidget()->close();
                success = true;
                msg = "Quit";
            } else if (cmd == "wq") {
                emit saveRequested();
                parentWidget()->close();
                success = true;
                msg = "Saved & quit";
            } else if (cmd == "q!") {
                parentWidget()->close();
                success = true;
                msg = "Quit without save";
            } else {
                msg = "Unknown command: " + cmd;
            }
            emit commandExecuted(cmd, success);
            currentCommandLine.clear();
            currentMode = "VI";
            emit modeChanged(currentMode);
            emit commandLineChanged("");
            return;
        } else if (e->key() == Qt::Key_Escape) {
            currentCommandLine.clear();
            currentMode = "VI";
            emit modeChanged(currentMode);
            emit commandLineChanged("");
            return;
        } else if (e->key() == Qt::Key_Backspace && !currentCommandLine.isEmpty()) {
            currentCommandLine.chop(1);
            emit commandLineChanged(currentCommandLine);
            return;
        } else if (!e->text().isEmpty()) {
            currentCommandLine += e->text();
            emit commandLineChanged(currentCommandLine);
            return;
        }
    }
    QTextCursor cursor = textCursor();
    if (e->key() == Qt::Key_I) {
        insertMode = true;
        currentMode = "INSERT";
        emit modeChanged(currentMode);
        emit vimKeyPressed("i");
    } else if (e->key() == Qt::Key_A) {
        insertMode = true;
        currentMode = "INSERT";
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        emit modeChanged(currentMode);
        emit vimKeyPressed("a");
    } else if (e->key() == Qt::Key_H) {
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
        emit vimKeyPressed("h");
    } else if (e->key() == Qt::Key_J) {
        cursor.movePosition(QTextCursor::Down);
        setTextCursor(cursor);
        emit vimKeyPressed("j");
    } else if (e->key() == Qt::Key_K) {
        cursor.movePosition(QTextCursor::Up);
        setTextCursor(cursor);
        emit vimKeyPressed("k");
    } else if (e->key() == Qt::Key_L) {
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        emit vimKeyPressed("l");
    } else if (e->key() == Qt::Key_X) {
        cursor.deleteChar();
        emit vimKeyPressed("x");
    } else if (e->key() == Qt::Key_D && e->modifiers() & Qt::ShiftModifier) {
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        emit vimKeyPressed("D");
    } else if (e->key() == Qt::Key_D && !(e->modifiers() & Qt::ShiftModifier)) {
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        emit vimKeyPressed("dd");
    } else if (e->key() == Qt::Key_Y && !(e->modifiers() & Qt::ShiftModifier)) {
        cursor.select(QTextCursor::LineUnderCursor);
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(cursor.selectedText());
        cursor.clearSelection();
        emit vimKeyPressed("yy");
    } else if (e->key() == Qt::Key_O) {
        cursor.movePosition(QTextCursor::EndOfLine);
        setTextCursor(cursor);
        insertPlainText("\n");
        insertMode = true;
        currentMode = "INSERT";
        emit modeChanged(currentMode);
        emit vimKeyPressed("o");
    } else if (e->key() == Qt::Key_W) {
        cursor.movePosition(QTextCursor::NextWord);
        setTextCursor(cursor);
        emit vimKeyPressed("w");
    } else if (e->key() == Qt::Key_B) {
        cursor.movePosition(QTextCursor::PreviousWord);
        setTextCursor(cursor);
        emit vimKeyPressed("b");
    } else if (e->key() == Qt::Key_Colon) {
        currentMode = "COMMAND";
        emit modeChanged(currentMode);
        currentCommandLine = ":";
        emit commandLineChanged(currentCommandLine);
    } else if (e->key() == Qt::Key_W && e->modifiers() & Qt::ControlModifier) {
        emit saveRequested();
        emit vimKeyPressed("<C-w>");
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

FindReplaceDialog::FindReplaceDialog(QWidget *parent)
    : QDialog(parent),
    findEdit(new QLineEdit(this)),
    replaceEdit(new QLineEdit(this)),
    caseCheckBox(new QCheckBox("Match &case", this)),
    wholeWordsCheckBox(new QCheckBox("&Whole words", this)) {
    setWindowTitle("Find and Replace");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon::fromTheme("wxHexEditor"));
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
            QFileInfo fi(path);
            return fi.fileName();
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
    return QString();
}

bool AdminFileHandler::saveWithAdmin(const QString &filePath, const QString &content) {
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
        const QStringList fallbacks = {
            "/usr/bin/x-terminal-emulator",
            "/bin/x-terminal-emulator",
            "x-terminal-emulator"
        };
        for (const QString &term : fallbacks) {
            if (QFile::exists(term) || !QStandardPaths::findExecutable(term).isEmpty()) {
                QProcess::startDetached(term);
                QMessageBox::warning(nullptr, "Terminal Launched",
                                     "No known terminal supports argument passing.\n"
                                     "Please run the save command manually in the opened terminal:\n"
                                     "sudo cp \"[TEMP_FILE]\" \"" + filePath + "\"");
                return true;
            }
        }
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
    } else if (terminal == "xfce4-terminal") {
        terminalArgs = {"-x", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    } else {
        terminalArgs = {"-e", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    }
    return QProcess::startDetached(terminal, terminalArgs);
}

bool AdminFileHandler::openWithAdmin(const QString &filePath) {
    QString terminal = findTerminal();
    if (terminal.isEmpty()) {
        const QStringList fallbacks = {
            "/usr/bin/x-terminal-emulator",
            "/bin/x-terminal-emulator",
            "x-terminal-emulator"
        };
        for (const QString &term : fallbacks) {
            if (QFile::exists(term) || !QStandardPaths::findExecutable(term).isEmpty()) {
                QProcess::startDetached(term);
                QMessageBox::warning(nullptr, "Terminal Launched",
                                     "Please run the following command in the opened terminal:\n"
                                     "sudo \"" + QCoreApplication::applicationFilePath() + "\" \"" + filePath + "\"");
                return true;
            }
        }
        QMessageBox::warning(nullptr, "Error", "No terminal emulator found.");
        return false;
    }
    QString appPath = QCoreApplication::applicationFilePath();
    QString cmd = QString("sudo \"%1\" \"%2\"").arg(appPath, filePath);
    QStringList terminalArgs;
    if (terminal == "konsole") {
        terminalArgs = {"-e", "sh", "-c", cmd + "; read -p 'Press Enter to close...'"};
    } else if (terminal == "gnome-terminal") {
        terminalArgs = {"--", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    } else {
        terminalArgs = {"-e", "sh", "-c", cmd + "; echo 'Press Enter to close...'; read"};
    }
    return QProcess::startDetached(terminal, terminalArgs);
}

class EmptyStateView : public QLabel {
public:
    explicit EmptyStateView(QWidget *parent = nullptr) : QLabel(parent) {
        setAlignment(Qt::AlignCenter);
        setTextFormat(Qt::RichText);
        setStyleSheet(
            "QLabel {"
            "    color: #a0e0a0;"
            "    background-color: #0a0a0a;"
            "    border: none;"
            "    margin: 0px;"
            "    padding: 20px;"
            "}"
            );
        setMinimumSize(1, 1);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QIcon vexIcon = QIcon::fromTheme("vex");

        QLabel *iconLabel = new QLabel(this);
        iconLabel->setPixmap(vexIcon.pixmap(128, 128));
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("opacity: 0.3; margin-bottom: 20px;");

        setText("<div align='center' style='opacity: 0.7;'>"
                "<div style='font-family: \"Nimbus Mono\", monospace; font-size: 18px; font-weight: bold;'>"
                "Welcome to Vex"
                "</div>"
                "<br><div style='font-family: \"Nimbus Mono\", monospace; font-size: 16px;'>"
                "Open a file to start editing<br><br>"
                ""
                "<br><hr style='border: 1px solid #ccc; margin: 20px 0;'>"
                "<div style='text-align: left; display: inline-block; max-width: 600px;'>"

                "&bull; <b>Ctrl+N</b>       &nbsp;&nbsp; New Empty file<br>"
                "&bull; <b>Ctrl+O</b>       &nbsp;&nbsp; Open file using url (instant)<br>"
                "&bull; <b>Ctrl+Shift+O</b> Open file using your Gui file manager provider<br>"
                "&bull; <b>Ctrl+S</b>       &nbsp;&nbsp; Save file<br>"
                "&bull; <b>Ctrl+Shift+S</b> Save as. dialog<br>"
                "&bull; <b>Ctrl+F</b>       &nbsp;&nbsp; Find and replace<br>"
                "&bull; <b>Ctrl+Z</b>       &nbsp;&nbsp; Undo<br>"
                "&bull; <b>Ctrl+Y</b>       &nbsp;&nbsp; Redo<br>"
                "&bull; <b>esc</b>          &nbsp;&nbsp; Change mode<br><br>"
                "&bull; <b>!tip: you can also drag a file to this window to make it open , </b><br>"
                "&nbsp;&nbsp;&nbsp;&nbsp;you can also open about from the menu to get more hint</b><br><hr style='border: 1px solid #ccc; margin: 20px 0;'>"

                "</div>"
                "</div>"
            );


    }


protected:
    void resizeEvent(QResizeEvent *event) override {
        setGeometry(0, 0, parentWidget()->width(), parentWidget()->height());
        QLabel::resizeEvent(event);
    }
};


class VexPluginInterface {
public:
    virtual ~VexPluginInterface() = default;
    virtual QString name() const = 0;
    virtual void initialize(VexEditor *editor) = 0;
    virtual void cleanup() {}
};
#define VexPluginInterface_iid "org.vex.VexPluginInterface"
Q_DECLARE_INTERFACE(VexPluginInterface, VexPluginInterface_iid)


class VexEditor : public QMainWindow {
    Q_OBJECT
public:
    explicit VexEditor(QWidget *parent = nullptr);
    ~VexEditor();
    void openFileAtPath(const QString &path);
    void loadPlugins();
    void unloadPlugins();
    QTabWidget* getTabWidget() { return tabWidget; }
    QString getFilePath(VimTextEdit *editor) { return filePaths.value(editor); }
private slots:
    void editSyntaxRules();
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
    void undo();
    void redo();
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void loadTheme();
    void saveTheme(const QString &qss);
    void updateWindowTitle();
    bool isSystemPath(const QString &path) const;
    bool hasBinaryContent(const QByteArray &data) const;
    void updateRecentMenu();
    QString syntaxRulesFilePath() const;
    QTabWidget *tabs = nullptr;
    EmptyStateView *emptyView = nullptr;
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
    QLabel *vimHintLabel = nullptr;
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
    AdminFileHandler adminHandler;
    QMenu *recentMenu = nullptr;
    QStringList recentFiles;
    const int MAX_RECENT_FILES = 10;
    void updateTabAppearance(int tabIndex);
};

QString VexEditor::syntaxRulesFilePath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/VexEditor";
    QDir().mkpath(dir);
    return dir + "/syntax.vex";
}

VimTextEdit* VexEditor::getCurrentEditor() {
    return qobject_cast<VimTextEdit*>(tabWidget->currentWidget());
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
    vimHintLabel = new QLabel("", this);
    vimHintLabel->setStyleSheet("QLabel { font-family: monospace; }");
    statusBar()->addPermanentWidget(vimHintLabel);
    connect(tabWidget, &QTabWidget::currentChanged, this, &VexEditor::updateWindowTitle);
    emptyView = new EmptyStateView(tabWidget);
    emptyView->raise();
    emptyView->show();
    connect(tabWidget, &QTabWidget::currentChanged, [this](int){
        emptyView->setVisible(tabWidget->count() == 0);
    });
}

void VexEditor::setupMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *newAction = fileMenu->addAction("&New");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &VexEditor::newFile);
    QAction *openAction = fileMenu->addAction("&Open");
    openAction->setShortcut(QKeySequence("Crtl+Shift+O"));
    connect(openAction, &QAction::triggered, this, &VexEditor::openFile);
    QAction *openByNameAction = fileMenu->addAction("Open &By Name...");
    openByNameAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(openByNameAction, &QAction::triggered, this, &VexEditor::openFileByName);
    fileMenu->addSeparator();
    recentMenu = fileMenu->addMenu("Open &Recent");
    updateRecentMenu();
    fileMenu->addSeparator();
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
    connect(undoAction, &QAction::triggered, this, &VexEditor::undo);
    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &VexEditor::redo);
    editMenu->addSeparator();
    findAction = editMenu->addAction("&Find and Replace");
    findAction->setShortcut(QKeySequence("Ctrl+F"));
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
    QAction *syntaxEditorAction = viewMenu->addAction("Edit Syntax Rules...");
    connect(syntaxEditorAction, &QAction::triggered, this, &VexEditor::editSyntaxRules);
    QAction *resetSyntaxAction = viewMenu->addAction("Reset Syntax Rules to Default");
    connect(resetSyntaxAction, &QAction::triggered, this, [this]() {
        if (QMessageBox::question(this, "Reset Syntax",
                                  "Delete syntax.vex and restore built-in default?",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QString userPath = syntaxRulesFilePath();
            QFile::remove(userPath);
            statusBar()->showMessage("Syntax rules reset - reopen files to apply default", 3000);
        }
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
    auto addThemedAction = [&](const QString &label, const QString &iconName, const char *slot) {
        QIcon icon = QIcon::fromTheme(iconName);
        QAction *action = new QAction(icon, label, this);
        connect(action, SIGNAL(triggered()), this, slot);
        toolbar->addAction(action);
        return action;
    };
    addThemedAction("Url", "url-copy", SLOT(openFileByName()));
    addThemedAction("New", "document-new", SLOT(newFile()));
    addThemedAction("Open file", "document-open", SLOT(openFile()));
    addThemedAction("Save", "document-save", SLOT(saveFile()));
    toolbar->addSeparator();
    addThemedAction("Undo", "edit-undo", SLOT(undo()));
    addThemedAction("Redo", "edit-redo", SLOT(redo()));
    toolbar->addSeparator();
    QAction *findReplaceAction = toolbar->addAction(
        QIcon::fromTheme("edit-find-replace", QIcon::fromTheme("edit-find")),
        "Find and Replace"
        );
    findReplaceAction->setShortcut(QKeySequence::Find);
    findReplaceAction->setToolTip("Find and Replace (Ctrl+H)");
    connect(findReplaceAction, &QAction::triggered, this, &VexEditor::showFindReplaceDialog);
    addThemedAction("Terminal", "utilities-terminal", SLOT(openTerminal()));
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
    connect(editor, &VimTextEdit::vimKeyPressed, this, [this](const QString &key) {
        vimHintLabel->setText(key);
        QTimer::singleShot(800, this, [this]() { vimHintLabel->clear(); });
    });
    connect(editor, &VimTextEdit::commandLineChanged, this, [this](const QString &cmd) {
        vimHintLabel->setText(cmd);
    });
    connect(editor, &VimTextEdit::commandExecuted, this, [this](const QString &cmd, bool success) {
        QString msg = success ? "Success: " + cmd : "Failed: " + cmd;
        statusBar()->showMessage(msg, 2000);
        vimHintLabel->clear();
    });
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexEditor::updateCursorPosition);
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool changed) {
        int index = tabWidget->indexOf(editor);
        if (index != -1) {
            updateTabAppearance(index);
        }
    });
    int index = tabWidget->addTab(editor, "");
    tabWidget->setCurrentIndex(index);
    filePaths[editor] = QString();
    updateTabAppearance(index);
    updateWindowTitle();
}
void VexEditor::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", QString(),
                                                    "All Files (*)");
    if (!fileName.isEmpty()) {
        openFileAtPath(fileName);
    }
}

void VexEditor::openFileAtPath(const QString &filePath) {
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
    QByteArray data = file.peek(8192);
    file.close();
    if (hasBinaryContent(data)) {
        QMessageBox::warning(this, "Binary File",
                             "This file appears to be binary. Vex Editor only supports text files.");
        return;
    }
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
        connect(editor, &VimTextEdit::vimKeyPressed, this, [this](const QString &key) {
            vimHintLabel->setText(key);
            QTimer::singleShot(800, this, [this]() { vimHintLabel->clear(); });
        });
        connect(editor, &VimTextEdit::commandLineChanged, this, [this](const QString &cmd) {
            vimHintLabel->setText(cmd);
        });
        connect(editor, &VimTextEdit::commandExecuted, this, [this](const QString &cmd, bool success) {
            QString msg = success ? "Success: " + cmd : "Failed: " + cmd;
            statusBar()->showMessage(msg, 2000);
            vimHintLabel->clear();
        });
        connect(editor, &QPlainTextEdit::cursorPositionChanged, this, &VexEditor::updateCursorPosition);
        connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool changed) {
            int index = tabWidget->indexOf(editor);
            if (index != -1) {
                updateTabAppearance(index);
            }
        });
        int index = tabWidget->addTab(editor, "");
        tabWidget->setCurrentIndex(index);
        filePaths[editor] = filePath;
        updateTabAppearance(index);
        updateWindowTitle();
        fileWatcher->addPath(filePath);
        if (SyntaxHighlighter *hl = qobject_cast<SyntaxHighlighter*>(editor->getHighlighter())) {
            QString userPath = syntaxRulesFilePath();
            QFile userFile(userPath);
            QString rulesText;
            if (userFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                rulesText = QString::fromUtf8(userFile.readAll());
                userFile.close();
                hl->loadRules(rulesText);
            }
            QString fileExt = QFileInfo(filePath).suffix().toLower();
            QString detectedLang = "default";
            for (auto it = hl->getExtensionToLanguage().constBegin(); it != hl->getExtensionToLanguage().constEnd(); ++it) {
                if (fileExt == it.key().mid(1)) {
                    detectedLang = it.value();
                    break;
                }
            }
            if (detectedLang == "default") {
                QString firstLine = content.split('\n').value(0);
                for (auto it = hl->getStartStringToLanguage().constBegin(); it != hl->getStartStringToLanguage().constEnd(); ++it) {
                    if (firstLine.startsWith(it.key())) {
                        detectedLang = it.value();
                        break;
                    }
                }
            }
            hl->setLanguage(detectedLang);
            QString ext = QFileInfo(filePath).suffix().toLower();
            QString pathToShow = userPath;
            if (QFile(userPath).exists()) {
                pathToShow = userPath;
            } else {
                pathToShow = ":/syntax.vex";
            }
            statusBar()->showMessage("Using syntax: " + QFileInfo(pathToShow).fileName(), 2000);
        }
    }
}
void VexEditor::editSyntaxRules() {
    QString userPath = syntaxRulesFilePath();
    QString content;
    QFile userFile(userPath);
    if (userFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        content = QString::fromUtf8(userFile.readAll());
        userFile.close();
    } else {
        QFile defaultFile(":/syntax.vex");
        if (defaultFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString::fromUtf8(defaultFile.readAll());
            defaultFile.close();
        }
    }
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (tabWidget->tabText(i) == "Syntax Rules") {
            tabWidget->setCurrentIndex(i);
            return;
        }
    }
    VimTextEdit *editor = new VimTextEdit(this);
    editor->setVimMode(false);
    editor->setPlainText(content);
    tabWidget->addTab(editor, "Syntax Rules");
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
    statusBar()->showMessage("Edit syntax rules - Ctrl+S or File → Save to save", 5000);
}
void VexEditor::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void VexEditor::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}
void VexEditor::dropEvent(QDropEvent *event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    QStringList files;
    for (const QUrl &url : urls) {
        QString path = url.toLocalFile();
        if (QFileInfo(path).isFile()) {
            files << path;
        }
    }
    if (!files.isEmpty()) {
        openFileAtPath(files.join("|"));
    }
    event->acceptProposedAction();
}

void VexEditor::saveFile() {
    VimTextEdit *editor = getCurrentEditor();
    if (!editor) return;
    if (tabWidget->tabToolTip(tabWidget->currentIndex()) == "Syntax Rules Editor") {
        QString userPath = syntaxRulesFilePath();
        QFile file(userPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(editor->toPlainText().toUtf8());
            file.close();
            editor->document()->setModified(false);
            QString rules = editor->toPlainText();
            for (int i = 0; i < tabWidget->count(); ++i) {
                if (VimTextEdit *ed = qobject_cast<VimTextEdit*>(tabWidget->widget(i))) {
                    if (SyntaxHighlighter *hl = qobject_cast<SyntaxHighlighter*>(ed->getHighlighter())) {
                        hl->loadRules(rules);
                    }
                }
            }
            statusBar()->showMessage("Syntax rules saved", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Could not save syntax rules.");
        }
        return;
    }
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
            int currentIndex = tabWidget->currentIndex();
            updateTabAppearance(currentIndex);
            statusBar()->showMessage("File saved: " + fileName, 3000);
            QSettings settings("VexEditor", "Vex");
            QStringList recentFiles = settings.value("recentFiles").toStringList();
            recentFiles.removeAll(fileName);
            recentFiles.prepend(fileName);
            while (recentFiles.size() > 10) recentFiles.removeLast();
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
                int currentIndex = tabWidget->currentIndex();
                updateTabAppearance(currentIndex);
                statusBar()->showMessage("Admin save initiated for: " + fileName, 3000);
                QSettings settings("VexEditor", "Vex");
                QStringList recentFiles = settings.value("recentFiles").toStringList();
                recentFiles.removeAll(fileName);
                recentFiles.prepend(fileName);
                while (recentFiles.size() > 10) recentFiles.removeLast();
                settings.setValue("recentFiles", recentFiles);
                updateRecentMenu();
            } else {
                QMessageBox::warning(this, "Error", "Failed to start admin save process.");
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "Could not save file:\n" + fileName);
    }
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
    updateWindowTitle();
}

void VexEditor::changeFont() {
    bool ok;
    QFont currentFont = getCurrentEditor() ? getCurrentEditor()->font() : QFont("Monospace", 10);
    QFont font = QFontDialog::getFont(&ok, currentFont, this, "Select Editor Font");
    if (ok) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(i));
            if (editor) {
                editor->setFont(font);
            }
        }
        QSettings settings("VexEditor", "Vex");
        settings.setValue("editorFont", font);
        statusBar()->showMessage(QString("Font set to %1, %2pt").arg(font.family()).arg(font.pointSize()), 3000);
    }
}

void VexEditor::toggleVimMode(bool enabled) {
    for (int i = 0; i < tabWidget->count(); ++i) {
        VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(i));
        if (editor) {
            editor->setVimMode(enabled);
        }
    }
    updateModeLabel(enabled ? "VI" : "INSERT");
}

void VexEditor::updateModeLabel(const QString &mode) {
    modeLabel->setText(mode);
    if (mode == "VI") {
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


void VexEditor::undo() {
    if (VimTextEdit *editor = getCurrentEditor()) {
        editor->undo();
    }
}

void VexEditor::redo() {
    if (VimTextEdit *editor = getCurrentEditor()) {
        editor->redo();
    }
}

void VexEditor::saveSettings() {
    QSettings settings("VexEditor", "Vex");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("vimMode", vimModeAction->isChecked());
    if (getCurrentEditor()) {
        settings.setValue("editorFont", getCurrentEditor()->font());
    }
    settings.setValue("recentFiles", recentFiles);
    if (!currentTheme.isEmpty() && currentTheme != defaultTheme) {
        settings.setValue("theme", currentTheme);
    } else {
        settings.remove("theme");
    }
}

void VexEditor::loadSettings() {
    QSettings settings("VexEditor", "Vex");
    restoreGeometry(settings.value("geometry").toByteArray());
    bool vimMode = settings.value("vimMode", false).toBool();
    vimModeAction->setChecked(vimMode);
    QFont defaultFont("Monospace", 10);
    defaultFont.setStyleHint(QFont::TypeWriter);
    QFont savedFont = settings.value("editorFont", defaultFont).value<QFont>();
    for (int i = 0; i < tabWidget->count(); ++i) {
        VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(i));
        if (editor) {
            editor->setFont(savedFont);
        }
    }
    recentFiles = settings.value("recentFiles").toStringList();
    updateRecentMenu();
    QString savedTheme = settings.value("theme", "").toString();
    if (!savedTheme.isEmpty()) {
        applyTheme(savedTheme);
        currentTheme = savedTheme;
    }
}

void VexEditor::updateRecentMenu() {
    if (!recentMenu) return;
    recentMenu->clear();
    QSettings settings("VexEditor", "Vex");
    QStringList recentFiles = settings.value("recentFiles").toStringList();
    for (int i = 0; i < recentFiles.size(); ++i) {
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

void VexEditor::openFileByName() {
    QDialog dialog(this);
    dialog.setWindowTitle("Open File by Path");
    dialog.setWindowIcon(QIcon::fromTheme("inode-symlink"));
    dialog.setMinimumSize(400, 300);
    dialog.setMaximumSize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    QLabel *iconArea = new QLabel(&dialog);
    iconArea->setFixedSize(100, 100);
    iconArea->setAlignment(Qt::AlignCenter);iconArea->setStyleSheet("border: 2px dashed #aaa; border-radius: 8px; background: transparent;");


    QLabel *iconLabel = new QLabel(iconArea);
    iconLabel->setFixedSize(64, 64);
    iconLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *iconLayout = new QVBoxLayout(iconArea);
    iconLayout->setContentsMargins(18, 18, 18, 18);
    iconLayout->addWidget(iconLabel);

    QFileIconProvider iconProvider;

    QIcon documentPreviewIcon = iconProvider.icon(QFileIconProvider::File);
    iconLabel->setPixmap(documentPreviewIcon.pixmap(64, 64));

    mainLayout->addWidget(iconArea, 0, Qt::AlignHCenter);


    QLabel *label = new QLabel("Enter file path (supports ~ for home):", &dialog);
    label->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(label);

    QLineEdit *pathEdit = new QLineEdit(&dialog);
    pathEdit->setPlaceholderText("e.g. ~/Documents/file.txt or /path/to/file");

    QCompleter *completer = new QCompleter(&dialog);
    QFileSystemModel *fsModel = new QFileSystemModel(completer);
    fsModel->setRootPath("");
    fsModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    completer->setModel(fsModel);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    pathEdit->setCompleter(completer);


    QIcon fallbackIcon = documentPreviewIcon;


    connect(pathEdit, &QLineEdit::textChanged, [pathEdit, completer, fsModel, iconLabel, fallbackIcon](const QString &text) {
        QFileIconProvider iconProvider;


        if (text.trimmed().isEmpty()) {
            iconLabel->setPixmap(fallbackIcon.pixmap(64, 64));
            fsModel->setRootPath("");
            return;
        }

        QString expanded = text;
        if (text.startsWith("~/")) {
            expanded = QDir::homePath() + text.mid(1);
        } else if (text == "~") {
            expanded = QDir::homePath();
        }

        QFileInfo info(expanded);
        QIcon icon = fallbackIcon;

        if (!expanded.isEmpty() && info.exists() && !info.isRoot()) {
            if (info.isDir()) {
                icon = iconProvider.icon(QFileIconProvider::Folder);
            } else {
                icon = iconProvider.icon(info);
            }
        }

        iconLabel->setPixmap(icon.pixmap(64, 64));

        if (info.isDir()) {
            fsModel->setRootPath(expanded);
        } else {
            fsModel->setRootPath(info.absolutePath());
        }
    });

    mainLayout->addWidget(pathEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Open | QDialogButtonBox::Cancel, &dialog);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);


    connect(pathEdit, &QLineEdit::returnPressed, [pathEdit, completer, fsModel, &dialog, iconLabel, fallbackIcon, this]() {
        QString currentText = pathEdit->text().trimmed();
        if (currentText.isEmpty()) {
            dialog.accept();
            return;
        }

        QString expanded = currentText;
        if (currentText.startsWith("~/")) {
            expanded = QDir::homePath() + currentText.mid(1);
        } else if (currentText == "~") {
            expanded = QDir::homePath();
        }

        QFileInfo info(expanded);


        if (info.exists() && info.isDir() && !info.isRoot()) {
            pathEdit->setText(expanded + "/");
            fsModel->setRootPath(expanded);
            QFileIconProvider iconProvider;
            iconLabel->setPixmap(iconProvider.icon(QFileIconProvider::Folder).pixmap(64, 64));
            pathEdit->selectAll();
            return;
        }

        dialog.accept();
    });

    pathEdit->setFocus();

    if (dialog.exec() == QDialog::Accepted) {
        QString filePath = pathEdit->text().trimmed();
        if (filePath.isEmpty()) {
            newFile();
            return;
        }


        if (filePath.startsWith("~/")) {
            filePath = QDir::homePath() + filePath.mid(1);
        } else if (filePath == "~") {
            filePath = QDir::homePath();
        }

        QFileInfo fileInfo(filePath);
        if (fileInfo.isRelative()) {
            filePath = QDir::current().absoluteFilePath(filePath);
        }
        fileInfo = QFileInfo(filePath);


        QString dirPath = fileInfo.absolutePath();
        QString fileName = fileInfo.fileName();


        QDir targetDir(dirPath);
        if (!targetDir.exists()) {
            if (!targetDir.mkpath(".")) {
                statusBar()->showMessage("Cannot create directory: " + dirPath, 3000);
                return;
            }
            statusBar()->showMessage("Created directory: " + dirPath, 2000);
        }


        QFile newFile(filePath);
        if (!fileInfo.exists()) {
            if (newFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                newFile.close();
                statusBar()->showMessage("Created: " + fileName + " in " + dirPath, 2000);
            } else {
                statusBar()->showMessage("Cannot create: " + filePath, 3000);
                return;
            }
        }

        openFileAtPath(filePath);
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

void VexEditor::updateTabAppearance(int tabIndex) {
    VimTextEdit *editor = qobject_cast<VimTextEdit*>(tabWidget->widget(tabIndex));
    if (!editor) return;

    QString filePath = filePaths.value(editor);
    QFileIconProvider iconProvider;
    QIcon fileIcon;
    QString tooltip;

    if (filePath.isEmpty()) {
        fileIcon = iconProvider.icon(QFileIconProvider::File);
        tooltip = "Untitled";
    } else {
        QFileInfo info(filePath);
        fileIcon = iconProvider.icon(info);
        tooltip = info.absoluteFilePath();
    }

    bool isModified = editor->document()->isModified();

    if (isModified) {
        tabWidget->setTabIcon(tabIndex, QIcon(fileIcon.pixmap(16, 16, QIcon::Disabled)));
    } else {
        tabWidget->setTabIcon(tabIndex, fileIcon);
    }

    QString tabTitle = filePath.isEmpty() ? "Untitled" : QFileInfo(filePath).fileName();
    tabWidget->setTabText(tabIndex, tabTitle);
    tabWidget->setTabToolTip(tabIndex, filePath.isEmpty() ? "Untitled" : filePath);
}
void VexEditor::showAbout() {
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("About Vex");
    aboutDialog.setMinimumSize(500, 400);
    aboutDialog.setWindowIcon(QIcon::fromTheme("liteinfo"));
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
                <li><b>Version:</b> 3.0</li>
                <li><b>Status:</b>MINT LEAF ( LTS )</li>
                <li><b>Release Date:</b> December 2025</li>
                <li><b>Security Support Until:</b> 01/5/2026</li>
                <li><b>Warranty:</b> Report bugs for issues</li>
            </ul>
        )"},
        { "Vi", R"(
            <h3>Vi Mode</h3>
            <ul>
                <li><b>h / j / k / l</b> → Move cursor</li>
                <li><b>i / a</b> → Enter INSERT mode</li>
                <li><b>x</b> → Delete character</li>
                <li><b>o</b> → New line below</li>
                <li><b>w / b</b> → Word forward/back</li>
                <li><b>Shift + D</b> → Delete to end of line</li>
                <li><b>Ctrl + W</b> → Save file</li>
            </ul>
        )"},
        { "Insert", R"(
            <h3>Insert Mode</h3>
            <ul>
                <li><b>Esc</b> → Return to VI mode</li>
                <li><b>Ctrl + S</b> → Save file</li>
                <li><b>Ctrl + Z / Y</b> → Undo / Redo</li>
                <li><b>Home / End</b> → Line start/end</li>
            </ul>
        )"},
        { "App Shortcuts", R"(
            <h3>App Shortcuts</h3>
            <ul>
                <li><b>Ctrl + N / O / S</b> → New / Open / Save</li>
                <li><b>Ctrl + H</b> → Find & Replace</li>
                <li><b>F3 / Shift + F3</b> → Find next / previous</li>
                <li><b>Ctrl + Q</b> → Quit</li>
            </ul>
        )"},
        { "License", R"(
            <h3>License</h3>
            <p>Vex Editor is licensed under the <b>Apache License 2.0</b>.</p>
            <p><a href='https:
            <h3>Author</h3>
            <p><b>Zynomon aelius</b></p>
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

void VexEditor::openPluginsFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(pluginsDirPath));
}

bool VexEditor::isSystemPath(const QString &path) const {
    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return false;
    }
    return !fileInfo.isWritable();
}

bool VexEditor::hasBinaryContent(const QByteArray &data) const {
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
        setWindowTitle("Vex - empty");
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

VexEditor::VexEditor(QWidget *parent) : QMainWindow(parent), recentMenu(nullptr) {
    QIcon appIcon = QIcon::fromTheme("vex");
    setWindowIcon(appIcon);
    resize(1000, 700);
    QFile themeFile(":/theme.qss");
    themeFile.open(QIODevice::ReadOnly | QIODevice::Text);
    defaultTheme = QString::fromUtf8(themeFile.readAll());
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
    setAcceptDrops(true);
}

VexEditor::~VexEditor() {
    qDeleteAll(plugins);
    saveSettings();
}

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    VexEditor editor;
    editor.show();

    if (argc > 1) {
        const QString path = QString::fromLocal8Bit(argv[1]);
        editor.openFileAtPath(path);
    }

    return app.exec();
}
