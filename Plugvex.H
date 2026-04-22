#ifndef PLUGVEX_H
#define PLUGVEX_H

#include <QPluginLoader>
#include <QtPlugin>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <QCommandLineParser>
#include <utility>
#include "Settings.H"

class CmdLine {
public:
    struct CMD {
        QStringList names;
        QString help;
        QString value;
    };

    struct FlagArgs {
        QString flag;
        QStringList args;
    };

    enum ParseResult {
        ParseOk,
        ParseError,
        HelpRequested,
        VersionRequested
    };

    static CmdLine& instance() {
        static CmdLine _instance;
        return _instance;
    }

    void addCommand(const CMD& cmd) {
        if (parsed) {
            qWarning() << "Cannot add commands after parse() has been called";
            return;
        }
        commands.append(cmd);
    }

    ParseResult parse(int argc, char *argv[]) {
        if (parsed) return ParseOk;

        parser.setApplicationDescription("Vex");
        parser.addHelpOption();
        parser.addVersionOption();

        for (const CMD& cmd : std::as_const(commands)) {
            if (cmd.names.isEmpty()) continue;

            QCommandLineOption option(
                cmd.names[0],
                cmd.help,
                cmd.value,
                cmd.value
                );

            parser.addOption(option);

            for (int i = 1; i < cmd.names.size(); ++i) {
                QCommandLineOption aliasOption(
                    cmd.names[i],
                    cmd.help,
                    cmd.value,
                    cmd.value
                    );
                parser.addOption(aliasOption);
            }
        }

        if (!parser.parse(QCoreApplication::arguments())) {
            errorMsg = parser.errorText();
            return ParseError;
        }

        if (parser.isSet("help")) {
            showHelp();
            return HelpRequested;
        }
        if (parser.isSet("version")) {
            qDebug().noquote() << "Vex 4.0 (Cytoplasm)";
            return VersionRequested;
        }

        collectFlagArgs();

        parsed = true;
        return ParseOk;
    }

    bool isSet(const QString& name) const {
        return parser.isSet(name);
    }

    QString value(const QString& name) const {
        return parser.value(name);
    }

    QStringList positionalArgs() const {
        return parser.positionalArguments();
    }

    QString errorString() const {
        return errorMsg;
    }

    QStringList flagArgs(const QString& flagName) const {
        for (const FlagArgs& fa : std::as_const(flagArgsList)) {
            QString cleanFlag = fa.flag;
            if (cleanFlag.startsWith("--"))
                cleanFlag = cleanFlag.mid(2);
            else if (cleanFlag.startsWith("-"))
                cleanFlag = cleanFlag.mid(1);

            if (cleanFlag == flagName) {
                return fa.args;
            }
        }
        return QStringList();
    }

private:
    void collectFlagArgs() {
        QStringList allArgs = QCoreApplication::arguments();
        flagArgsList.clear();

        for (int i = 1; i < allArgs.size(); ++i) {
            QString arg = allArgs[i];

            if (arg.startsWith('-')) {
                FlagArgs current;
                current.flag = arg;

                int j = i + 1;
                while (j < allArgs.size() && !allArgs[j].startsWith('-')) {
                    current.args << allArgs[j];
                    j++;
                }

                flagArgsList.append(current);
                i = j - 1;
            }
        }
    }

    void showHelp() {
        QString help;
        QTextStream stream(&help);

        stream << "\n";
        stream << "Vex\n";
        stream << "───────────────────────────────────\n";
        stream << "\n";
        stream << "About:\n";
        stream << "  Version 4.0 (Cytoplasm)\n";
        stream << "\n";
        stream << "\n";
        stream << "Commands:\n";
        stream << "\n";
        stream << "vex <flag> <options>                    Description\n";
        stream << "───────────────────────────────────\n";

        for (const CMD& cmd : std::as_const(commands)) {
            QString names = cmd.names.join(", ");
            if (cmd.value.isEmpty()) {
                stream << QString("  %1 %2\n")
                .arg(names, -30)
                    .arg(cmd.help);
            } else {
                stream << QString("  %1 <%2> %3\n")
                .arg(names, -30)
                    .arg(cmd.value, -15)
                    .arg(cmd.help);
            }
        }
        stream << "\n";

        qDebug().noquote() << help;
    }

    CmdLine() = default;
    CmdLine(const CmdLine&) = delete;
    CmdLine& operator=(const CmdLine&) = delete;
    QCommandLineParser parser;
    QList<CMD> commands;
    QList<FlagArgs> flagArgsList;
    QString errorMsg;
    bool parsed = false;
};

struct PluginMetadata {
    enum Importance {
        Xylem,
        Phloem,
        Simple
    };
    Importance importance;
};

class CorePlugin {
public:
    virtual ~CorePlugin() = default;
    virtual PluginMetadata meta() const = 0;
    virtual bool initialize(class MainWindow*, class Settings*, CmdLine& cmdLine) = 0;
};

Q_DECLARE_INTERFACE(CorePlugin, "vex.core/4.0")

class PluginLoader {
public:
    static void loadAndInitialize(MainWindow* window, Settings* settings, CmdLine& cmdLine, int argc, char *argv[]) {
cmdLine.addCommand({{"d", "debug"}, "Enable debug mode", ""});

        QStringList paths;
        paths << QCoreApplication::applicationDirPath();

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
        QByteArray ldLibraryPath = qgetenv("LD_LIBRARY_PATH");
        if (!ldLibraryPath.isEmpty()) {
            QStringList ldDirs = QString::fromLocal8Bit(ldLibraryPath).split(':');
            for (const QString& dir : ldDirs) {
                if (!dir.isEmpty()) {
                    paths << dir + "/vex";
                }
            }
        }
        paths << "/usr/lib/vex";
        paths << "/usr/local/lib/vex";
#endif

        QList<CorePlugin*> xylem, phloem, simple;
        QList<PluginError> errors;

        for (const QString& path : std::as_const(paths)) {
            QDir dir(path);
            const QStringList files = dir.entryList(getPluginExtensions(), QDir::Files);
            for (const QString& file : std::as_const(files)) {
                if (QPluginLoader* loader = tryLoadPlugin(dir.absoluteFilePath(file), errors)) {
                    activeLoaders.append(loader);
                    CorePlugin* plugin = qobject_cast<CorePlugin*>(loader->instance());

                    switch (plugin->meta().importance) {
                    case PluginMetadata::Xylem:  xylem.append(plugin); break;
                    case PluginMetadata::Phloem: phloem.append(plugin); break;
                    case PluginMetadata::Simple: simple.append(plugin); break;
                    }
                }
            }
        }

        if (!errors.isEmpty()) {
            showErrorSummary(errors);
        }

        initializePhase("Xylem", xylem, window, settings, cmdLine);
        initializePhase("Phloem", phloem, window, settings, cmdLine);
        initializePhase("Simple", simple, window, settings, cmdLine);

        CmdLine::ParseResult result = cmdLine.parse(argc, argv);
        if (result == CmdLine::HelpRequested || result == CmdLine::VersionRequested) {
            exit(0);
        }
        if (result == CmdLine::ParseError) {
            qCritical() << cmdLine.errorString();
            exit(1);
        }

        Debug_Mode = cmdLine.isSet("debug");
    }

    static void cleanup() {
        if (Debug_Mode) {
            qDebug() << "Unloading" << activeLoaders.size() << "plugins";
        }
        for (QPluginLoader* loader : std::as_const(activeLoaders)) {
            loader->unload();
            delete loader;
        }
        activeLoaders.clear();
    }

    static void setDebugMode(bool enable) {
        Debug_Mode = enable;
    }

private:
    struct PluginError {
        QString filename;
        QString error;
    };

    static QList<QPluginLoader*> activeLoaders;
    static bool Debug_Mode;

    static void Debug_Log(const QString& message) {
        if (Debug_Mode) {
            qDebug() << message;
        }
    }

    static void Debug_Warning(const QString& message) {
        if (Debug_Mode) {
            qWarning() << message;
        }
    }

    static void Debug_Critical(const QString& message) {
        if (Debug_Mode) {
            qCritical() << message;
        }
    }

    static QStringList getPluginExtensions() {
        QStringList extensions;
#ifdef Q_OS_WIN
        extensions << "*.dll";
#elif defined(Q_OS_MAC)
        extensions << "*.dylib";
#else
        extensions << "*.so";
#endif
        return extensions;
    }

    static QPluginLoader* tryLoadPlugin(const QString& path, QList<PluginError>& errors) {
        QPluginLoader* loader = new QPluginLoader(path);

        if (!loader->load()) {
            QString filename = QFileInfo(path).fileName();
            QString error = loader->errorString();
            errors.append({filename, error});
            Debug_Warning("Failed to load plugin: " + filename + " Error: " + error);
            delete loader;
            return nullptr;
        }

        QObject* instance = loader->instance();
        if (!instance) {
            QString filename = QFileInfo(path).fileName();
            errors.append({filename, "Plugin created no instance"});
            Debug_Warning("Plugin created no instance: " + filename);
            loader->unload();
            delete loader;
            return nullptr;
        }

        CorePlugin* plugin = qobject_cast<CorePlugin*>(instance);
        if (!plugin) {
            QString filename = QFileInfo(path).fileName();
            errors.append({filename, "Not a valid CorePlugin interface"});
            Debug_Warning("Not a valid CorePlugin: " + filename);
            loader->unload();
            delete loader;
            return nullptr;
        }

        Debug_Log("Successfully loaded plugin: " + QFileInfo(path).fileName());
        return loader;
    }

    static void initializePhase(const char* phaseName, QList<CorePlugin*>& plugins,
                                MainWindow* window, Settings* settings, CmdLine& cmdLine) {
        Debug_Log("Initializing " + QString(phaseName) + " plugins (" +
                  QString::number(plugins.size()) + " found)");

        for (CorePlugin* plugin : std::as_const(plugins)) {
            try {
                if (!plugin->initialize(window, settings, cmdLine)) {
                    QString warningMsg = QString("Plugin returned false during %1 phase").arg(phaseName);
                    Debug_Warning(warningMsg);
                    if (Debug_Mode) {
                        QMessageBox::warning(nullptr, "Plugin Initialization Failed", warningMsg);
                    }
                }
            } catch (const std::bad_alloc& e) {
                QString errorMsg = QString("Critical memory error during %1 phase:\n%2").arg(phaseName).arg(e.what());
                Debug_Critical(errorMsg);
                QMessageBox::critical(nullptr, "Critical Plugin Error!", errorMsg);
                throw;
            } catch (const std::exception& e) {
                QString errorMsg = QString("Plugin threw exception during %1 phase:\n%2").arg(phaseName).arg(e.what());
                Debug_Critical(errorMsg);
                if (Debug_Mode) {
                    QMessageBox::critical(nullptr, "Plugin Exception!", errorMsg);
                }
            } catch (...) {
                QString errorMsg = QString("A plugin crashed during %1 phase and was isolated.").arg(phaseName);
                Debug_Critical(errorMsg);
                if (Debug_Mode) {
                    QMessageBox::critical(nullptr, "Plugin Crashed!", errorMsg);
                }
            }
        }
    }

    static void showErrorSummary(const QList<PluginError>& errors) {
        if (!Debug_Mode) {
            return;
        }

        QString summary = QString("Failed to load %1 plugin(s):\n\n").arg(errors.size());

        int count = 0;
        for (const auto& err : std::as_const(errors)) {
            summary += QString("• %1\n  %2\n\n").arg(err.filename, err.error);
            if (++count >= 10 && errors.size() > 10) {
                summary += QString("... and %1 more errors.\n").arg(errors.size() - 10);
                break;
            }
        }

        Debug_Warning("Plugin load errors: " + QString::number(errors.size()));
        QMessageBox::warning(nullptr, "Plugin Load Errors", summary);
    }
};

QList<QPluginLoader*> PluginLoader::activeLoaders;
bool PluginLoader::Debug_Mode = false;

#endif // PLUGVEX_H
