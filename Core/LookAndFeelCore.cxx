#include <QObject>
#include <QInputDialog>
#include <QtPlugin>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTabBar>
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
#include <QResource>
#include <algorithm>
#include "Plugvex.H"
#include "Settings.H"

class SyntaxColorEvent : public QEvent {
public:
    static const QEvent::Type eventType = static_cast<QEvent::Type>(QEvent::User + 1000);

    SyntaxColorEvent(const QMap<QString, QColor> &colors)
        : QEvent(eventType), m_colors(colors) {}

    QMap<QString, QColor> colors() const { return m_colors; }

private:
    QMap<QString, QColor> m_colors;
};

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
        stylesheetsPath = themesPath + "/stylesheets";
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

        if (styleName == "NONE") {
            QApplication::setStyle(QStyleFactory::create(""));
            Settings::instance().setValue("qtStyle", "");
            m_mainWindow->statusBar()->showMessage("Qt Style: System Default", 2000);
        } else {
            QApplication::setStyle(QStyleFactory::create(styleName));
            Settings::instance().setValue("qtStyle", styleName);
            m_mainWindow->statusBar()->showMessage("Qt Style: " + styleName, 2000);
        }
    }

    void onThemeSelected(QAction *action) {
        QString themePath = action->data().toString();

        if (themePath == "CREATE_NEW") {
            createNewTheme();
            return;
        }

        if (themePath == "NONE") {
            qApp->setStyleSheet("");
            currentThemePath = "";
            Settings::instance().setValue("currentThemePath", "");
            m_mainWindow->statusBar()->showMessage("Theme: None", 2000);

            QList<QAction*> actions = themesMenu->actions();
            for (QAction *act : actions) {
                if (act->data().toString() == "NONE") {
                    act->setChecked(true);
                    break;
                }
            }
            return;
        }

        applyTheme(themePath, true);
        m_mainWindow->statusBar()->showMessage("Theme: " + QFileInfo(themePath).baseName(), 2000);
    }

    void onIconThemeSelected(QAction *action) {
        QString iconThemeName = action->data().toString();

        if (iconThemeName.isEmpty()) {
            currentIconTheme = "";
            QIcon::setThemeName("");
            QIcon::setThemeSearchPaths(QStringList());
            Settings::instance().setValue("currentIconTheme", "");
            m_mainWindow->statusBar()->showMessage("Icon theme: System Default", 2000);
        } else {
            currentIconTheme = iconThemeName;
            Settings::instance().setValue("currentIconTheme", iconThemeName);
            updateIconSearchPaths();
            m_mainWindow->statusBar()->showMessage("Icon theme: " + iconThemeName, 2000);
        }
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

        if (path.startsWith(":/") || path.startsWith("qrc:"))
            return;

        if (path == currentThemePath) {
            applyTheme(path, false);
            m_mainWindow->statusBar()->showMessage("Theme hot-reloaded: " + QFileInfo(path).baseName(), 2000);
        }
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
    QString currentThemePath;
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

        QAction *noneThemeAction = themesMenu->addAction("None (No Theme)");
        noneThemeAction->setCheckable(true);
        noneThemeAction->setData("NONE");

        themesMenu->addSeparator();
        rebuildThemesMenu();

        iconMenu = viewMenu->addMenu("&Icon Theme");
        rebuildIconMenu();
    }

    void populateQtStyleMenu() {
        QStringList availableStyles = QStyleFactory::keys();
        QString currentStyle = QApplication::style()->objectName();
        QString savedStyle = Settings::instance().get<QString>("qtStyle");

        QActionGroup *styleGroup = new QActionGroup(this);
        styleGroup->setExclusive(true);

        QAction *noneAction = qtStyleMenu->addAction("System Default");
        noneAction->setCheckable(true);
        noneAction->setData("NONE");
        styleGroup->addAction(noneAction);

        if (savedStyle.isEmpty())
            noneAction->setChecked(true);

        qtStyleMenu->addSeparator();

        for (const QString &styleName : std::as_const(availableStyles)) {
            QAction *action = qtStyleMenu->addAction(styleName);
            action->setCheckable(true);
            action->setData(styleName);
            styleGroup->addAction(action);

            if (!savedStyle.isEmpty() && styleName.toLower() == savedStyle.toLower())
                action->setChecked(true);
            else if (savedStyle.isEmpty() && styleName.toLower() == currentStyle.toLower())
                action->setChecked(true);
        }

        connect(styleGroup, &QActionGroup::triggered,
                this, &LookAndFeelCorePlugin::onQtStyleSelected);
    }

    void rebuildThemesMenu() {
        QList<QAction*> actions = themesMenu->actions();

        for (int i = actions.size() - 1; i >= 4; --i) {
            themesMenu->removeAction(actions[i]);
            delete actions[i];
        }

        QActionGroup *themeGroup = new QActionGroup(this);
        themeGroup->setExclusive(true);

        QAction *noneAction = themesMenu->actions().at(2);

        themeGroup->addAction(noneAction);

        QStringList allPaths;

        QDir stylesDir(stylesheetsPath);
        QStringList fsThemes = stylesDir.entryList(QStringList() << "*.qss", QDir::Files);
        for (const QString &file : std::as_const(fsThemes)) {
            allPaths.append(stylesDir.absoluteFilePath(file));
        }

        QDir qrcDir(":/");
        if (qrcDir.exists()) {
            QStringList qrcFiles = qrcDir.entryList(QStringList() << "*.qss", QDir::Files);
            for (const QString &file : std::as_const(qrcFiles)) {
                allPaths.append(":/" + file);
            }
        }

        if (allPaths.isEmpty()) {
            QAction *noThemes = themesMenu->addAction("No themes available");
            noThemes->setEnabled(false);
            return;
        }

        std::sort(allPaths.begin(), allPaths.end(),
                  [](const QString &a, const QString &b) {
                      return QFileInfo(a).baseName().toLower() < QFileInfo(b).baseName().toLower();
                  });

        for (const QString &path : std::as_const(allPaths)) {
            QString displayName = QFileInfo(path).baseName();
            QAction *action = themesMenu->addAction(displayName);
            action->setCheckable(true);
            action->setData(path);
            themeGroup->addAction(action);

            if (path == currentThemePath)
                action->setChecked(true);

            if (!path.startsWith(":/") && !path.startsWith("qrc:")) {
                if (!themeWatcher->files().contains(path))
                    themeWatcher->addPath(path);
            }
        }

        if (currentThemePath.isEmpty())
            noneAction->setChecked(true);

        connect(themeGroup, &QActionGroup::triggered,
                this, &LookAndFeelCorePlugin::onThemeSelected);
    }

    void rebuildIconMenu() {
        iconMenu->clear();

        QActionGroup *iconGroup = new QActionGroup(this);
        iconGroup->setExclusive(true);

        QAction *noneAction = iconMenu->addAction("None (System Icons)");
        noneAction->setCheckable(true);
        noneAction->setData("");
        iconGroup->addAction(noneAction);

        if (currentIconTheme.isEmpty())
            noneAction->setChecked(true);

        iconMenu->addSeparator();

        QDir iconDir(iconsPath);
        QStringList userIconThemes = iconDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        if (!userIconThemes.isEmpty()) {
            for (const QString &themeName : std::as_const(userIconThemes)) {
                QAction *action = iconMenu->addAction(themeName + " (User)");
                action->setCheckable(true);
                action->setData(themeName);
                iconGroup->addAction(action);

                if (themeName == currentIconTheme)
                    action->setChecked(true);
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
                    if (!xdgThemes.contains(theme))
                        xdgThemes.append(theme);
                }
            }
        }

        xdgThemes.sort(Qt::CaseInsensitive);

        for (const QString &themeName : std::as_const(xdgThemes)) {
            QAction *action = iconMenu->addAction(themeName + " (System)");
            action->setCheckable(true);
            action->setData(themeName);
            iconGroup->addAction(action);

            if (themeName == currentIconTheme)
                action->setChecked(true);
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
        QString themeName = QInputDialog::getText(m_mainWindow, "New Theme",
                                                  "Enter theme name:");
        if (themeName.isEmpty()) return;

        QString originalName = themeName;
        if (!themeName.endsWith(".qss", Qt::CaseInsensitive))
            themeName += ".qss";

        QString themePath = stylesheetsPath + "/" + themeName;

        QFile templateFile(":/vex.qss");
        if (!templateFile.exists()) {
            QMessageBox::warning(m_mainWindow, "Error", "Default template :/vex.qss not found!");
            return;
        }

        if (templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QFile newFile(themePath);
            if (newFile.open(QIODevice::WriteOnly)) {
                newFile.write(templateFile.readAll());
                newFile.close();
            }
            templateFile.close();
        }

        QString fileReqPath = Settings::instance().basePath() + "/.temp/fileReq";
        QDir().mkpath(Settings::instance().basePath() + "/.temp");

        QFile pathFile(fileReqPath);
        if (pathFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&pathFile);
            out.setEncoding(QStringConverter::Utf8);
            out << themePath << "\n";
            pathFile.close();
        }

        QMetaObject::invokeMethod(m_mainWindow, "openFile",
                                  Q_ARG(QString, themePath));

        themeWatcher->addPath(themePath);
        applyTheme(themePath, true);
        rebuildThemesMenu();

        QList<QAction*> actions = themesMenu->actions();
        for (QAction *act : actions) {
            if (act->data().toString() == themePath) {
                act->setChecked(true);
                break;
            }
        }

        QMessageBox::information(m_mainWindow, "Theme Applied",
                                 QString(tr("Applied theme: %1\n\n"
                                            "Save the file (Ctrl+S) to see changes live!\n\n"
                                            "Theme file created at:\n%2"))
                                     .arg(originalName)
                                     .arg(themePath));
    }

    void applyTheme(const QString &themeFilePath, bool saveAsCurrent) {
        QFile file(themeFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_mainWindow->statusBar()->showMessage("Failed to load theme: " + themeFilePath, 3000);
            return;
        }

        QString qssContent = QString::fromUtf8(file.readAll());
        file.close();

        QMap<QString, QString> vexProps = extractVexProperties(qssContent);
        applyPropertiesToEditors(vexProps);

        QMap<QString, QColor> syntaxColors;
        QStringList colorKeys = {"comment", "critical", "quote", "keyword", "string"};
        for (const QString &key : colorKeys) {
            if (vexProps.contains(key)) {
                QColor color(vexProps[key]);
                if (color.isValid())
                    syntaxColors[key] = color;
            }
        }

        if (!syntaxColors.isEmpty()) {
            SyntaxColorEvent *event = new SyntaxColorEvent(syntaxColors);
            QApplication::postEvent(m_mainWindow, event);
        }

        QTabWidget *tabWidget = m_mainWindow->findChild<QTabWidget*>("VexTab");
        if (tabWidget) {
            tabWidget->setDocumentMode(false);
            tabWidget->setElideMode(Qt::ElideNone);
            QTabBar *tabBar = tabWidget->tabBar();
            if (tabBar) {
                tabBar->setExpanding(true);
                tabBar->setDrawBase(true);
                tabBar->setStyleSheet("");
            }
        }

        qApp->setStyleSheet(qssContent);

        if (saveAsCurrent) {
            Settings::instance().setValue("currentThemePath", themeFilePath);
            currentThemePath = themeFilePath;
        }
    }

    QMap<QString, QString> extractVexProperties(const QString &qssContent) {
        QMap<QString, QString> properties;
        QRegularExpression vexBlockRe(R"(VEX#QSS\s*\{([^}]*)\})");
        QRegularExpressionMatch match = vexBlockRe.match(qssContent);
        if (!match.hasMatch())
            return properties;

        QString vexBlock = match.captured(1);
        QStringList lines = vexBlock.split('\n');
        for (const QString &line : std::as_const(lines)) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || !trimmed.contains(':'))
                continue;
            int colonPos = trimmed.indexOf(':');
            QString key = trimmed.left(colonPos).trimmed();
            QString value = trimmed.mid(colonPos + 1).trimmed();
            if (value.endsWith(';'))
                value.chop(1);
            value = value.trimmed();
            properties[key] = value;
        }
        return properties;
    }

    void applyPropertiesToEditors(const QMap<QString, QString> &properties) {
        QStackedWidget* stack = m_mainWindow->findChild<QStackedWidget*>("VexStack");
        if (!stack) return;
        QTabWidget* tabWidget = stack->findChild<QTabWidget*>("VexTab");
        if (!tabWidget) return;
        QList<QPlainTextEdit*> editors = tabWidget->findChildren<QPlainTextEdit*>("VexEditor");
        for (QPlainTextEdit *editor : std::as_const(editors)) {
            if (properties.contains("line-highlight-color")) {
                QColor color(properties["line-highlight-color"]);
                if (color.isValid())
                    editor->setProperty("lineHighlightColor", color);
            }
            if (properties.contains("line-number-color-fg")) {
                QColor color(properties["line-number-color-fg"]);
                if (color.isValid())
                    editor->setProperty("lineNumberFg", color);
            }
            if (properties.contains("line-number-color-bg")) {
                QColor color(properties["line-number-color-bg"]);
                if (color.isValid())
                    editor->setProperty("lineNumberBg", color);
            }
            editor->viewport()->update();
        }
    }

    void updateIconSearchPaths() {
        if (currentIconTheme.isEmpty()) return;
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

#ifdef Q_OS_WIN
        Settings::instance().setValue("qtStyle", "Fusion");
#endif

        QString savedStyle = settings.get<QString>("qtStyle");
        if (!savedStyle.isEmpty() && QStyleFactory::keys().contains(savedStyle, Qt::CaseInsensitive)) {
            QApplication::setStyle(QStyleFactory::create(savedStyle));
        }

        currentThemePath = settings.get<QString>("currentThemePath");
        if (!currentThemePath.isEmpty() && QFile::exists(currentThemePath)) {
            applyTheme(currentThemePath, false);
        } else {

            QString defaultTheme = ":/vex.qss";
            if (QFile::exists(defaultTheme)) {
                applyTheme(defaultTheme, true);
            }
        }

        currentIconTheme = settings.get<QString>("currentIconTheme");
        if (!currentIconTheme.isEmpty())
            updateIconSearchPaths();
    }
};

#include "LookAndFeelCore.moc"
