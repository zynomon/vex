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
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStyleFactory>
#include <QApplication>
#include <QFileSystemWatcher>
#include <QStatusBar>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QStackedWidget>
#include <QFileInfo>
#include <QIcon>
#include <QColor>
#include <QStyle>
#include <QRegularExpression>
#include <algorithm>
#include "Plugvex.H"
#include "Settings.H"

class LookAndFeelCorePlugin : public QObject, public CorePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "vex.core/4.0")
    Q_INTERFACES(CorePlugin)

public:
    PluginMetadata meta() const override {
        PluginMetadata metadata;
        metadata.importance = PluginMetadata::Simple;
        return metadata;
    }

    bool initialize(MainWindow* window, Settings* settings, CmdLine& cmdLine) override {
        Q_UNUSED(settings)
        Q_UNUSED(cmdLine)

        m_mainWindow = reinterpret_cast<QMainWindow*>(window);

        basePath = Settings::instance().basePath();
        themesPath = basePath + "/themes";
        stylesheetsPath = themesPath + "/styleSheets";
        iconsPath = themesPath + "/icon";

        QDir().mkpath(themesPath);
        QDir().mkpath(stylesheetsPath);
        QDir().mkpath(iconsPath);

        themeWatcher = new QFileSystemWatcher(this);
        themeWatcher->addPath(stylesheetsPath);
        themeWatcher->addPath(iconsPath);

        connect(themeWatcher, &QFileSystemWatcher::directoryChanged,
                this, &LookAndFeelCorePlugin::onThemeDirectoryChanged);
        connect(themeWatcher, &QFileSystemWatcher::fileChanged,
                this, &LookAndFeelCorePlugin::onThemeFileChanged);

        createViewMenuIntegration();
        loadPreferences();

        m_mainWindow->statusBar()->showMessage("Look & Feel plugin loaded", 3000);

        return true;
    }

    QIcon resolveIcon(const QString &baseName) {
        if (!currentIconTheme.isEmpty()) {
            QString userThemePath = iconsPath + "/" + currentIconTheme;
            if (QDir(userThemePath).exists()) {
                static const QStringList exts = {"svg", "png", "ico", "icns", "svgz"};
                for (const QString &ext : std::as_const(exts)) {
                    QString iconFile = userThemePath + "/" + baseName + "." + ext;
                    if (QFile::exists(iconFile)) {
                        return QIcon(iconFile);
                    }
                }
            }
        }

        return Settings::resolveIcon(baseName);
    }

private slots:
    void onQtStyleSelected(QAction *action) {
        QString styleName = action->data().toString();
        QApplication::setStyle(QStyleFactory::create(styleName));
        Settings::instance().setValue("qtStyle", styleName);
        m_mainWindow->statusBar()->showMessage("Qt Style: " + styleName, 2000);
    }

    void onThemeSelected(QAction *action) {
        QString themeName = action->data().toString();
        if (themeName == "CREATE_NEW") {
            createNewTheme();
            return;
        }

        QString themeFile = stylesheetsPath + "/" + themeName + ".qss";
        applyTheme(themeFile, true);
        currentTheme = themeName;
        Settings::instance().setValue("currentTheme", themeName);
        m_mainWindow->statusBar()->showMessage("Theme: " + themeName, 2000);
    }

    void onIconThemeSelected(QAction *action) {
        QString iconThemeName = action->data().toString();
        currentIconTheme = iconThemeName;
        Settings::instance().setValue("currentIconTheme", iconThemeName);

        updateIconSearchPaths();

        m_mainWindow->statusBar()->showMessage("Icon theme: " + iconThemeName, 2000);
    }

    void onThemeDirectoryChanged(const QString &path) {
        Q_UNUSED(path)
        m_mainWindow->statusBar()->showMessage("Theme directory changed, reloading...", 2000);
        rebuildThemesMenu();
        rebuildIconMenu();
    }

    void onThemeFileChanged(const QString &path) {
        m_mainWindow->statusBar()->showMessage(
            QString("Theme file changed: %1").arg(QFileInfo(path).fileName()), 2000);

        if (!themeWatcher->files().contains(path)) {
            themeWatcher->addPath(path);
        }

        QString themeName = QFileInfo(path).baseName();
        if (themeName == currentTheme) {
            applyTheme(path, false);
            m_mainWindow->statusBar()->showMessage("Theme hot-reloaded: " + themeName, 2000);
        }
    }

    void onThemeEditorTextChanged() {
        QPlainTextEdit *editor = qobject_cast<QPlainTextEdit*>(sender()->parent());
        if (!editor) return;

        if (!editor->property("isThemeEditor").toBool()) return;

        QString content = editor->toPlainText();
        applyThemeFromContent(content, false);
    }

private:
    QMainWindow *m_mainWindow{nullptr};
    QMenu *viewMenu{nullptr};
    QMenu *qtStyleMenu{nullptr};
    QMenu *themesMenu{nullptr};
    QMenu *iconMenu{nullptr};
    QFileSystemWatcher *themeWatcher{nullptr};
    QString basePath;
    QString themesPath;
    QString stylesheetsPath;
    QString iconsPath;
    QString currentTheme;
    QString currentIconTheme;

    void createViewMenuIntegration() {
        QList<QAction*> menuActions = m_mainWindow->menuBar()->actions();

        for (QAction *action : std::as_const(menuActions)) {
            if (action->text() == "&View" || action->text() == "View") {
                viewMenu = action->menu();
                break;
            }
        }

        if (!viewMenu) {
            viewMenu = m_mainWindow->menuBar()->addMenu("&View");
        }

        viewMenu->addSeparator();

        qtStyleMenu = viewMenu->addMenu("Qt St&yle");
        populateQtStyleMenu();

        themesMenu = viewMenu->addMenu("&Themes");
        themesMenu->addAction("Create New Theme...", this, [this]() {
            createNewTheme();
        });
        themesMenu->addSeparator();
        rebuildThemesMenu();

        iconMenu = viewMenu->addMenu("&Icon Theme");
        rebuildIconMenu();
    }

    void populateQtStyleMenu() {
        QStringList availableStyles = QStyleFactory::keys();
        QString currentStyle = QApplication::style()->objectName();

        QActionGroup *styleGroup = new QActionGroup(this);
        styleGroup->setExclusive(true);

        for (const QString &styleName : std::as_const(availableStyles)) {
            QAction *action = qtStyleMenu->addAction(styleName);
            action->setCheckable(true);
            action->setData(styleName);
            styleGroup->addAction(action);

            if (styleName.toLower() == currentStyle.toLower()) {
                action->setChecked(true);
            }
        }

        connect(styleGroup, &QActionGroup::triggered,
                this, &LookAndFeelCorePlugin::onQtStyleSelected);
    }

    void rebuildThemesMenu() {
        QList<QAction*> actions = themesMenu->actions();
        for (int i = actions.size() - 1; i >= 2; --i) {
            themesMenu->removeAction(actions[i]);
            delete actions[i];
        }

        QDir stylesDir(stylesheetsPath);
        QStringList filters;
        filters << "*.qss";
        QStringList themeFiles = stylesDir.entryList(filters, QDir::Files);

        if (themeFiles.isEmpty()) {
            QAction *noThemes = themesMenu->addAction("No themes available");
            noThemes->setEnabled(false);
            return;
        }

        QActionGroup *themeGroup = new QActionGroup(this);
        themeGroup->setExclusive(true);

        QStringList sortedThemes;
        for (const QString &file : std::as_const(themeFiles)) {
            sortedThemes.append(QFileInfo(file).baseName());
        }
        sortedThemes.sort(Qt::CaseInsensitive);

        for (const QString &themeName : std::as_const(sortedThemes)) {
            QAction *action = themesMenu->addAction(themeName);
            action->setCheckable(true);
            action->setData(themeName);
            themeGroup->addAction(action);

            if (themeName == currentTheme) {
                action->setChecked(true);
            }

            QString themeFile = stylesheetsPath + "/" + themeName + ".qss";
            if (!themeWatcher->files().contains(themeFile)) {
                themeWatcher->addPath(themeFile);
            }
        }

        connect(themeGroup, &QActionGroup::triggered,
                this, &LookAndFeelCorePlugin::onThemeSelected);
    }

    void rebuildIconMenu() {
        iconMenu->clear();

        QActionGroup *iconGroup = new QActionGroup(this);
        iconGroup->setExclusive(true);

        QDir iconDir(iconsPath);
        QStringList userIconThemes = iconDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        if (!userIconThemes.isEmpty()) {
            for (const QString &themeName : std::as_const(userIconThemes)) {
                QAction *action = iconMenu->addAction(themeName + " (User)");
                action->setCheckable(true);
                action->setData(themeName);
                iconGroup->addAction(action);

                if (themeName == currentIconTheme) {
                    action->setChecked(true);
                }
            }

            iconMenu->addSeparator();
        }

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
        QStringList xdgIconPaths = {
            "/usr/share/icons",
            QDir::homePath() + "/.local/share/icons",
            QDir::homePath() + "/.icons"
        };

        QStringList xdgThemes;
        for (const QString &xdgPath : std::as_const(xdgIconPaths)) {
            QDir xdgDir(xdgPath);
            if (xdgDir.exists()) {
                QStringList themes = xdgDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString &theme : std::as_const(themes)) {
                    if (!xdgThemes.contains(theme)) {
                        xdgThemes.append(theme);
                    }
                }
            }
        }

        xdgThemes.sort(Qt::CaseInsensitive);

        for (const QString &themeName : std::as_const(xdgThemes)) {
            QAction *action = iconMenu->addAction(themeName + " (System)");
            action->setCheckable(true);
            action->setData(themeName);
            iconGroup->addAction(action);

            if (themeName == currentIconTheme) {
                action->setChecked(true);
            }
        }
#endif

        if (iconMenu->actions().isEmpty()) {
            QAction *noIcons = iconMenu->addAction("No icon themes available");
            noIcons->setEnabled(false);
        }

        connect(iconGroup, &QActionGroup::triggered,
                this, &LookAndFeelCorePlugin::onIconThemeSelected);
    }

    void createNewTheme() {
        QStackedWidget* stack = m_mainWindow->findChild<QStackedWidget*>("VexStack");
        if (!stack) {
            QMessageBox::warning(m_mainWindow, "Error", "Could not find VexStack");
            return;
        }

        QTabWidget* tabWidget = stack->findChild<QTabWidget*>("VexTab");
        if (!tabWidget) {
            QMessageBox::warning(m_mainWindow, "Error", "Could not find VexTab");
            return;
        }

        QPlainTextEdit *editor = new QPlainTextEdit(m_mainWindow);
        editor->setObjectName("VexEditor");
        editor->setPlainText(getDefaultThemeTemplate());

        int index = tabWidget->addTab(editor, "Untitled Theme");
        tabWidget->setCurrentIndex(index);

        editor->setProperty("isThemeEditor", true);
        editor->setProperty("themeSavePath", stylesheetsPath);

        connect(editor->document(), &QTextDocument::contentsChanged,
                this, &LookAndFeelCorePlugin::onThemeEditorTextChanged);

        applyThemeFromContent(editor->toPlainText(), false);

        m_mainWindow->statusBar()->showMessage(
            "Theme editor opened - save with Ctrl+S to " + stylesheetsPath, 3000);
    }

    void applyTheme(const QString &themeFilePath, bool saveAsCurrent) {
        QFile file(themeFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_mainWindow->statusBar()->showMessage("Failed to load theme: " + themeFilePath, 3000);
            return;
        }

        QString qssContent = QString::fromUtf8(file.readAll());
        file.close();

        applyThemeFromContent(qssContent, saveAsCurrent);
    }

    void applyThemeFromContent(const QString &qssContent, bool saveAsCurrent) {
        parseAndApplyVexProperties(qssContent);

        qApp->setStyleSheet(qssContent);

        if (saveAsCurrent) {
            Settings::instance().setValue("vexThemeProperties",
                                          QVariant::fromValue(extractVexProperties(qssContent)));
        }
    }

    QMap<QString, QString> extractVexProperties(const QString &qssContent) {
        QMap<QString, QString> properties;

        QRegularExpression vexBlockRe(R"(VEX#QSS\s*\{([^}]*)\})");
        QRegularExpressionMatch match = vexBlockRe.match(qssContent);

        if (!match.hasMatch()) {
            return properties;
        }

        QString vexBlock = match.captured(1);
        QStringList lines = vexBlock.split('\n');

        for (const QString &line : std::as_const(lines)) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || !trimmed.contains(':')) {
                continue;
            }

            int colonPos = trimmed.indexOf(':');
            QString key = trimmed.left(colonPos).trimmed();
            QString value = trimmed.mid(colonPos + 1).trimmed();

            if (value.endsWith(';')) {
                value.chop(1);
            }
            value = value.trimmed();

            properties[key] = value;
        }

        return properties;
    }

    void parseAndApplyVexProperties(const QString &qssContent) {
        QMap<QString, QString> properties = extractVexProperties(qssContent);

        if (properties.isEmpty()) {
            return;
        }

        applyPropertiesToEditors(properties);
    }

    void applyPropertiesToEditors(const QMap<QString, QString> &properties) {
        QStackedWidget* stack = m_mainWindow->findChild<QStackedWidget*>("VexStack");
        if (!stack) return;

        QTabWidget* tabWidget = stack->findChild<QTabWidget*>("VexTab");
        if (!tabWidget) return;

        QList<QPlainTextEdit*> editors = tabWidget->findChildren<QPlainTextEdit*>("VexEditor");

        for (QPlainTextEdit *editor : std::as_const(editors)) {
            applyPropertiesToEditor(editor, properties);
        }
    }

    void applyPropertiesToEditor(QPlainTextEdit *editor, const QMap<QString, QString> &properties) {
        if (!editor) return;

        if (properties.contains("line-highlight-color")) {
            QColor color(properties["line-highlight-color"]);
            if (color.isValid()) {
                editor->setProperty("lineHighlightColor", color);
            }
        }

        if (properties.contains("line-number-color-fg")) {
            QColor color(properties["line-number-color-fg"]);
            if (color.isValid()) {
                editor->setProperty("lineNumberFg", color);
            }
        }

        if (properties.contains("line-number-color-bg")) {
            QColor color(properties["line-number-color-bg"]);
            if (color.isValid()) {
                editor->setProperty("lineNumberBg", color);
            }
        }

        if (properties.contains("comment")) {
            QColor color(properties["comment"]);
            if (color.isValid()) {
                editor->setProperty("syntaxComment", color);
            }
        }

        if (properties.contains("critical")) {
            QColor color(properties["critical"]);
            if (color.isValid()) {
                editor->setProperty("syntaxCritical", color);
            }
        }

        if (properties.contains("quote")) {
            QColor color(properties["quote"]);
            if (color.isValid()) {
                editor->setProperty("syntaxQuote", color);
            }
        }

        if (properties.contains("keyword")) {
            QColor color(properties["keyword"]);
            if (color.isValid()) {
                editor->setProperty("syntaxKeyword", color);
            }
        }

        if (properties.contains("string")) {
            QColor color(properties["string"]);
            if (color.isValid()) {
                editor->setProperty("syntaxString", color);
            }
        }

        editor->viewport()->update();
    }

    void updateIconSearchPaths() {
        if (currentIconTheme.isEmpty()) {
            return;
        }

        QString userThemePath = iconsPath + "/" + currentIconTheme;
        if (QDir(userThemePath).exists()) {
            QIcon::setThemeSearchPaths(QStringList() << iconsPath);
            QIcon::setThemeName(currentIconTheme);
            return;
        }

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
        QIcon::setThemeName(currentIconTheme);
#endif
    }

    void loadPreferences() {
        Settings &settings = Settings::instance();

        QString savedStyle = settings.get<QString>("qtStyle");
        if (!savedStyle.isEmpty() && QStyleFactory::keys().contains(savedStyle, Qt::CaseInsensitive)) {
            QApplication::setStyle(QStyleFactory::create(savedStyle));
        }

        currentTheme = settings.get<QString>("currentTheme");
        if (!currentTheme.isEmpty()) {
            QString themeFile = stylesheetsPath + "/" + currentTheme + ".qss";
            if (QFile::exists(themeFile)) {
                applyTheme(themeFile, false);
            }
        }

        currentIconTheme = settings.get<QString>("currentIconTheme");
        if (!currentIconTheme.isEmpty()) {
            updateIconSearchPaths();
        }
    }

    QString getDefaultThemeTemplate() {
        return R"(/* Vex Theme Template
 * Save with Ctrl+S - will prompt to save in theme directory
 * File will automatically get .qss extension
 */

VEX#QSS {
    line-highlight-color: rgba(0, 60, 30, 102);
    line-number-color-fg: rgb(100, 180, 100);
    line-number-color-bg: rgb(30, 30, 30);
    comment: rgb(128, 128, 128);
    critical: rgb(255, 255, 255);
    quote: rgb(0, 255, 0);
    keyword: rgb(135, 206, 250);
    string: rgb(255, 165, 0);
}

/* Standard QSS below */

QMainWindow {
    background-color: #1e1e1e;
    color: #d4d4d4;
}

QPlainTextEdit {
    background-color: #1e1e1e;
    color: #d4d4d4;
    selection-background-color: #264f78;
    selection-color: #ffffff;
}

QMenuBar {
    background-color: #2d2d30;
    color: #cccccc;
}

QMenuBar::item:selected {
    background-color: #3e3e42;
}

QMenu {
    background-color: #2d2d30;
    color: #cccccc;
    border: 1px solid #454545;
}

QMenu::item:selected {
    background-color: #3e3e42;
}

QStatusBar {
    background-color: #007acc;
    color: #ffffff;
}

QTabWidget::pane {
    border: 1px solid #454545;
    background-color: #252526;
}

QTabBar::tab {
    background-color: #2d2d30;
    color: #969696;
    padding: 8px 16px;
    border: 1px solid #454545;
}

QTabBar::tab:selected {
    background-color: #1e1e1e;
    color: #ffffff;
    border-bottom-color: #1e1e1e;
}
)";
    }
};

#include "LookAndFeelCore.moc"
