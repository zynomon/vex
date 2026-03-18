/****************************************************************
*                                                              *
*                         Apache 2.0                           *
*     Copyright Zynomon aelius <zynomon@proton.me>  2026       *
*               Project         :        Vex                   *
*               Version         :        4.0 (Cytoplasm)       *
*                                                              *
*                                                              *
****************************************************************/
#include <QObject>
#include <QtPlugin>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QMainWindow>
#include <QStatusBar>
#include <QComboBox>
#include <QIcon>
#include <QStackedWidget>
#include <QTabWidget>
#include <QFileSystemWatcher>
#include <QPlainTextEdit>
#include "Plugvex.H"
#include "Settings.H"

struct Token {
    int start;
    int length;
    QTextCharFormat style;
};

struct ExactMatch {
    QString text;
    QTextCharFormat style;
};

struct BlockMatch {
    QString opener;
    QString closer;
    QTextCharFormat style;
    int id;
    bool endsAtNewline;
    bool capturesDelimiter;
};

struct SyntaxDef {
    QString langName;
    QString icon;
    QStringList extensions;
    QStringList contentStarts;
    QList<ExactMatch> exactMatches;
    QList<BlockMatch> blockMatches;
};

class ColorPalette {
public:
    QColor comment{128, 128, 128};
    QColor critical{255, 255, 255};
    QColor quote{0, 255, 0};
    QColor keyword{135, 206, 250};
    QColor string{255, 165, 0};

    QColor lookup(const QString &name) const {
        if (name == "comment") return comment;
        if (name == "critical") return critical;
        if (name == "quote") return quote;
        if (name == "keyword") return keyword;
        if (name == "string") return string;
        return QColor(name);
    }
};

class TextPainter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit TextPainter(QTextDocument *doc = nullptr)
        : QSyntaxHighlighter(doc)
        , active(false)
    {
    }

    void activate(const QString &lang, const QString &fileContent) {
        currentLang.clear();
        definitions.clear();

        if (lang.isEmpty() || fileContent.isEmpty()) {
            active = false;
            rehighlight();
            return;
        }

        readSyntaxFile(fileContent);

        if (definitions.contains(lang)) {
            currentLang = lang;
            active = true;
        } else {
            active = false;
        }

        rehighlight();
    }

    void updateColors(const ColorPalette &pal) {
        palette = pal;

        for (auto &def : definitions) {
            for (auto &em : def.exactMatches) {
                updateFormatColors(em.style);
            }
            for (auto &bm : def.blockMatches) {
                updateFormatColors(bm.style);
            }
        }

        rehighlight();
    }

protected:
    void highlightBlock(const QString &line) override {
        if (!active || line.isEmpty()) {
            setCurrentBlockState(-1);
            return;
        }

        if (!definitions.contains(currentLang)) {
            setCurrentBlockState(-1);
            return;
        }

        const SyntaxDef &syntax = definitions[currentLang];
        int previousState = previousBlockState();

        if (previousState > 0) {
            resumeBlock(line, syntax, previousState);
            return;
        }
        setCurrentBlockState(-1);

        for (int i = 0; i < syntax.blockMatches.size(); ++i) {
            const BlockMatch &block = syntax.blockMatches[i];

            int pos = line.indexOf(block.opener);
            if (pos == -1) continue;

            int afterOpener = pos + block.opener.length();

            if (block.endsAtNewline) {
                setFormat(pos, line.length() - pos, block.style);
                setCurrentBlockState(-1);
                return;
            }

            int closePos = line.indexOf(block.closer, afterOpener);

            if (closePos != -1) {
                int len = closePos + block.closer.length() - pos;
                setFormat(pos, len, block.style);
                continue;
            } else {
                setFormat(pos, line.length() - pos, block.style);
                setCurrentBlockState(block.id);
                return;
            }
        }

        for (const ExactMatch &exact : syntax.exactMatches) {
            int pos = 0;
            while ((pos = line.indexOf(exact.text, pos)) != -1) {
                setFormat(pos, exact.text.length(), exact.style);
                pos += exact.text.length();
            }
        }
    }

private:
    bool active;
    QString currentLang;
    QMap<QString, SyntaxDef> definitions;
    ColorPalette palette;

    void resumeBlock(const QString &line, const SyntaxDef &syntax, int state) {
       int blockIdx = state - 1;

        if (blockIdx < 0 || blockIdx >= syntax.blockMatches.size()) {
            setCurrentBlockState(-1);
            return;
        }

        const BlockMatch &block = syntax.blockMatches[blockIdx];

        int closePos = line.indexOf(block.closer);

        if (closePos != -1) {
            int len = closePos + block.closer.length();
            setFormat(0, len, block.style);
            setCurrentBlockState(-1);

        } else {
            setFormat(0, line.length(), block.style);
            setCurrentBlockState(state);
        }
    }

    void readSyntaxFile(const QString &content) {
        QStringList lines = content.split('\n');

        SyntaxDef current;
        int blockIdCounter = 1;

        for (const QString &rawLine : std::as_const(lines)) {
            QString line = rawLine.trimmed();
            if (line.isEmpty() || line.startsWith("⇏ ")) {
                continue;
            }

            if (line.startsWith("REG = \"")) {
                if (!current.langName.isEmpty()) {
                    definitions[current.langName] = current;
                }
                current = SyntaxDef();
                current.langName = grabQuoted(line);
                blockIdCounter = 1;
                continue;
            }

            if (line.startsWith("ICNS = \"")) {
                current.icon = grabQuoted(line);
                continue;
            }

            if (line.startsWith("File =")) {
                QString rest = line.mid(6).trimmed();
                QStringList parts = rest.split("&&");
                for (QString ext : parts) {
                    ext = ext.trimmed();
                    if (!ext.startsWith('.')) {
                        ext = "." + ext;
                    }
                    current.extensions.append(ext);
                }
                continue;
            }

            if (line.startsWith("Sw =")) {
                QString rest = line.mid(4).trimmed();
                QStringList parts = rest.split("&&");
                for (QString pat : parts) {
                    current.contentStarts.append(pat.trimmed());
                }
                continue;
            }

            if (line.contains('=')) {
                int eqPos = line.indexOf('=');
                QString left = line.left(eqPos).trimmed();
                QString right = line.mid(eqPos + 1).trimmed();

                QTextCharFormat fmt = buildFormat(right);

                QStringList patterns = left.split("&&");

                for (QString pat : patterns) {
                    pat = pat.trimmed();

                    if (pat.contains("+ES'") && pat.contains("'ES-")) {
                        QString inside = extractInside(pat, "+ES'", "'ES-");
                        if (!inside.isEmpty()) {
                            ExactMatch em;
                            em.text = inside;
                            em.style = fmt;
                            current.exactMatches.append(em);
                        }
                    }
                    else if (pat.contains("+HS'") && pat.contains("to") && pat.contains("+HE'")) {
                        QString opener = extractInside(pat, "+HS'", "'HS-");
                        QString closer = extractInside(pat, "+HE'", "'HE-");

                        if (!opener.isEmpty()) {
                            BlockMatch bm;
                            bm.opener = opener;
                            bm.closer = closer;
                            bm.style = fmt;
                            bm.id = blockIdCounter++;

                         bm.endsAtNewline = pat.contains("+HE''HE-");

                            bm.capturesDelimiter = (opener.contains("delem") || closer.contains("delem"));

                            current.blockMatches.append(bm);
                        }
                    }
                }
            }
        }

        if (!current.langName.isEmpty()) {
            definitions[current.langName] = current;
        }
    }

    QString grabQuoted(const QString &line) {
        int start = line.indexOf('\"');
        if (start == -1) return QString();
        start++;

        int end = line.indexOf('\"', start);
        if (end == -1) return QString();

        return line.mid(start, end - start);
    }

    QString extractInside(const QString &text, const QString &before, const QString &after) {
        int startPos = text.indexOf(before);
        if (startPos == -1) return QString();

        startPos += before.length();

        int endPos = text.indexOf(after, startPos);
        if (endPos == -1) return QString();

        return text.mid(startPos, endPos - startPos);
    }

    QTextCharFormat buildFormat(const QString &attrs) {
        QTextCharFormat fmt;

        QStringList parts = attrs.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        for (const QString &part : std::as_const(parts)) {
            if (part.startsWith("color:")) {
                QString colorName = part.mid(6);
                QColor color = palette.lookup(colorName);
                if (color.isValid()) {
                    fmt.setForeground(QBrush(color));
                }
            }
            else if (part.startsWith("font:")) {
                QString style = part.mid(5);
                if (style == "B") {
                    fmt.setFontWeight(QFont::Bold);
                } else if (style == "I") {
                    fmt.setFontItalic(true);
                } else if (style == "BI") {
                    fmt.setFontWeight(QFont::Bold);
                    fmt.setFontItalic(true);
                } else if (style == "N") {
                    fmt.setFontWeight(QFont::Normal);
                    fmt.setFontItalic(false);
                }
            }
        }

        return fmt;
    }

    void updateFormatColors(QTextCharFormat &fmt) {
    }
};

class SyntaxCorePlugin : public QObject, public CorePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "vex.core/4.0")
    Q_INTERFACES(CorePlugin)

public:
    PluginMetadata meta() const override {
        PluginMetadata metadata;
        metadata.importance = PluginMetadata::Phloem;
        return metadata;
    }

    bool initialize(MainWindow* window, Settings* settings, CmdLine& cmdLine) override {
        Q_UNUSED(settings)
        Q_UNUSED(cmdLine)

        mainWin = reinterpret_cast<QMainWindow*>(window);

        QStackedWidget* stack = mainWin->findChild<QStackedWidget*>("VexStack");
        if (!stack) return true;

        tabs = stack->findChild<QTabWidget*>("VexTab");
        if (!tabs) return true;

        syntaxDir = Settings::instance().basePath() + "/syntax";
        QDir().mkpath(syntaxDir);

        selector = new QComboBox(mainWin);
        selector->setObjectName("SyntaxLanguageSelector");
        selector->setToolTip("Select syntax highlighting");
        selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        selector->setIconSize(QSize(16, 16));

        mainWin->statusBar()->insertPermanentWidget(0, selector);

        watcher = new QFileSystemWatcher(this);
        watcher->addPath(syntaxDir);

        connect(watcher, &QFileSystemWatcher::directoryChanged,
                this, &SyntaxCorePlugin::onDirectoryChanged);
        connect(watcher, &QFileSystemWatcher::fileChanged,
                this, &SyntaxCorePlugin::onFileChanged);

        scanFiles();
        buildSelector();

        connect(selector, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SyntaxCorePlugin::onSelectionChanged);

        connect(tabs, &QTabWidget::currentChanged,
                this, &SyntaxCorePlugin::onTabSwitched);
        connect(tabs, &QTabWidget::tabCloseRequested,
                this, &SyntaxCorePlugin::onTabClosed);

        attachToEditors();

        mainWin->statusBar()->showMessage("Syntax highlighting ready", 2000);

        return true;
    }

private slots:
    void onSelectionChanged(int idx) {
        if (idx < 0) return;

        QString choice = selector->itemData(idx).toString();
        int currentTab = tabs->currentIndex();
        if (currentTab < 0) return;

        QWidget *w = tabs->widget(currentTab);
        QPlainTextEdit *editor = qobject_cast<QPlainTextEdit*>(w);
        if (!editor) return;

        TextPainter *painter = painters.value(editor);
        if (!painter) {
            painter = new TextPainter(editor->document());
            painters[editor] = painter;
        }

        if (choice == "AUTO") {
            QString detected = detectLanguage(currentTab);
            if (!detected.isEmpty() && syntaxFiles.contains(detected)) {
                painter->activate(detected, syntaxFiles[detected]);
                mainWin->statusBar()->showMessage("Auto: " + detected, 2000);

                for (int i = 0; i < selector->count(); ++i) {
                    if (selector->itemData(i).toString() == detected) {
                        selector->blockSignals(true);
                        selector->setCurrentIndex(i);
                        selector->blockSignals(false);
                        break;
                    }
                }
            } else {
                painter->activate(QString(), QString());
                mainWin->statusBar()->showMessage("No language detected", 2000);
            }
        } else if (choice == "PLAIN") {
            painter->activate(QString(), QString());
            mainWin->statusBar()->showMessage("Plain text", 2000);
        } else {
            if (syntaxFiles.contains(choice)) {
                painter->activate(choice, syntaxFiles[choice]);
                mainWin->statusBar()->showMessage("Language: " + choice, 2000);
            }
        }
    }

    void onTabSwitched(int idx) {
        if (idx < 0) {
            selector->setCurrentIndex(0);
            return;
        }
        selector->setCurrentIndex(0);
    }

    void onTabClosed(int idx) {
        QWidget *w = tabs->widget(idx);
        QPlainTextEdit *editor = qobject_cast<QPlainTextEdit*>(w);
        if (editor) {
            painters.remove(editor);
        }
    }

    void onDirectoryChanged(const QString &path) {
        Q_UNUSED(path)
        mainWin->statusBar()->showMessage("Syntax dir changed", 2000);
        reloadAll();
    }

    void onFileChanged(const QString &path) {
        mainWin->statusBar()->showMessage("File changed: " + QFileInfo(path).fileName(), 2000);

        if (!watcher->files().contains(path)) {
            watcher->addPath(path);
        }

        reloadFile(path);
    }

    void reloadAll() {
        icons.clear();
        syntaxFiles.clear();

        QStringList watched = watcher->files();
        if (!watched.isEmpty()) {
            watcher->removePaths(watched);
        }
        watcher->addPath(syntaxDir);

        scanFiles();
        buildSelector();

        int idx = selector->currentIndex();
        if (idx >= 0) {
            onSelectionChanged(idx);
        }
    }

    void reloadFile(const QString &path) {
        readFile(path);
        buildSelector();

        int idx = selector->currentIndex();
        if (idx >= 0) {
            onSelectionChanged(idx);
        }
    }

private:
    QMainWindow *mainWin{nullptr};
    QTabWidget *tabs{nullptr};
    QComboBox *selector{nullptr};
    QFileSystemWatcher *watcher{nullptr};
    QMap<QString, QString> icons;
    QMap<QString, QString> syntaxFiles;
    QString syntaxDir;
    QMap<QPlainTextEdit*, TextPainter*> painters;

    void attachToEditors() {
        QList<QPlainTextEdit*> editors = tabs->findChildren<QPlainTextEdit*>("VexEditor");

        for (QPlainTextEdit *ed : editors) {
            if (!painters.contains(ed)) {
                TextPainter *p = new TextPainter(ed->document());
                painters[ed] = p;
            }
        }

        int idx = tabs->currentIndex();
        if (idx >= 0) {
            selector->setCurrentIndex(0);
        }
    }

    QString detectLanguage(int tabIdx) {
        QString tabText = tabs->tabText(tabIdx);
        QWidget *w = tabs->widget(tabIdx);
        QPlainTextEdit *editor = qobject_cast<QPlainTextEdit*>(w);

        if (!editor) return QString();

        QString content = editor->toPlainText();
        QStringList lines = content.split('\n');
        int checkLines = qMin(10, lines.size());

        for (int i = 0; i < checkLines; ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;

            for (auto it = syntaxFiles.constBegin(); it != syntaxFiles.constEnd(); ++it) {
                QStringList starts = grabStartPatterns(it.value());
                for (const QString &pattern : std::as_const(starts)) {
                    if (line.startsWith(pattern)) {
                        return it.key();
                    }
                }
            }
        }

        QFileInfo info(tabText);
        QString ext = info.suffix().toLower();
        if (!ext.isEmpty()) {
            ext = "." + ext;

            for (auto it = syntaxFiles.constBegin(); it != syntaxFiles.constEnd(); ++it) {
                QStringList fileLines = it.value().split('\n');

                for (const QString &line : std::as_const(fileLines)) {
                    QString trimmed = line.trimmed();

                    if (trimmed.startsWith("File =")) {
                        QString exts = trimmed.mid(6).trimmed();
                        QStringList extList = exts.split("&&");

                        for (QString e : extList) {
                            e = e.trimmed();
                            if (!e.startsWith('.')) {
                                e = "." + e;
                            }

                            if (e == ext) {
                                return it.key();
                            }
                        }
                    }
                }
            }
        }

        return QString();
    }

    QStringList grabStartPatterns(const QString &content) {
        QStringList patterns;
        QStringList lines = content.split('\n');

        for (const QString &line : std::as_const(lines)) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("Sw =")) {
                QString rest = trimmed.mid(4).trimmed();
                patterns = rest.split("&&");
                for (QString &p : patterns) {
                    p = p.trimmed();
                }
                break;
            }
        }
        return patterns;
    }

    void scanFiles() {
        QDir dir(syntaxDir);
        QStringList filters;
        filters << "*.vxsyn";

        QStringList files = dir.entryList(filters, QDir::Files);

        for (const QString &fileName : std::as_const(files)) {
            QString path = dir.absoluteFilePath(fileName);
            readFile(path);

            if (!watcher->files().contains(path)) {
                watcher->addPath(path);
            }
        }
    }

    void readFile(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QString content = QString::fromUtf8(file.readAll());
        file.close();

        QStringList lines = content.split('\n');
        QString lang;
        QString icon;

        for (const QString &line : std::as_const(lines)) {
            QString trimmed = line.trimmed();

            if (trimmed.startsWith("REG = \"")) {
                int s = trimmed.indexOf('\"') + 1;
                int e = trimmed.indexOf('\"', s);
                if (e != -1) {
                    lang = trimmed.mid(s, e - s);
                }
            } else if (trimmed.startsWith("ICNS = \"")) {
                int s = trimmed.indexOf('\"') + 1;
                int e = trimmed.indexOf('\"', s);
                if (e != -1) {
                    icon = trimmed.mid(s, e - s);
                }
            }
        }

        if (!lang.isEmpty()) {
            syntaxFiles[lang] = content;
            icons[lang] = icon.isEmpty() ? "text-x-generic" : icon;
        }
    }

    void buildSelector() {
        QString current;
        int idx = selector->currentIndex();
        if (idx >= 0) {
            current = selector->itemData(idx).toString();
        }

        selector->clear();

        QIcon autoIcon = Settings::resolveIcon("system-run");
        selector->addItem(autoIcon, "Auto", "AUTO");

        QIcon plainIcon = Settings::resolveIcon("text-x-generic");
        selector->addItem(plainIcon, "Plain Text", "PLAIN");

        selector->insertSeparator(2);

        QStringList langs;
        for (auto it = syntaxFiles.constBegin(); it != syntaxFiles.constEnd(); ++it) {
            langs.append(it.key());
        }
        langs.sort(Qt::CaseInsensitive);

        for (const QString &lang : std::as_const(langs)) {
            QString iconName = icons.value(lang, "text-x-generic");
            QIcon icon = Settings::resolveIcon(iconName);
            selector->addItem(icon, lang, lang);
        }

        if (!current.isEmpty()) {
            for (int i = 0; i < selector->count(); ++i) {
                if (selector->itemData(i).toString() == current) {
                    selector->setCurrentIndex(i);
                    break;
                }
            }
        } else {
            selector->setCurrentIndex(0);
        }
    }
};

#include "SyntaxCore.moc"
