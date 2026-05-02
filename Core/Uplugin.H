#ifndef UPLUGIN_H
#define UPLUGIN_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QIcon>
#include <QDockWidget>
#include <QDesktopServices>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QTabWidget>
#include <QThread>
#include <QStackedWidget>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QMainWindow>
#include <QApplication>
#include <QPluginLoader>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QProcess>
#include <QCoreApplication>
#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QTextEdit>
#include <QRadioButton>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QPainter>
#include <QPixmap>
#include <QMessageBox>
#include <QDebug>
#include <QMap>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextDocument>
#include <utility>
#include "../Settings.H"

struct Dependency {
    QString name;
    QString downloadLink;
};

struct UpluginMetadata {
    QString name;
    QIcon icon;
    QString maintainer;
    QString description;
    QString version;
    QList<Dependency> depends;
};

class UpluginBridge;

class Uplugin {
public:
    virtual ~Uplugin() = default;
    virtual UpluginMetadata metadata() const = 0;
    virtual void setBridge(UpluginBridge* bridge) { m_bridge = bridge; }
    virtual QList<QDockWidget*> dockWidgets() const { return {}; }
    virtual QList<QAction*> menuActions() const { return {}; }
    virtual QList<QAction*> toolbarActions() const { return {}; }

protected:
    UpluginBridge* m_bridge = nullptr;
};

Q_DECLARE_INTERFACE(Uplugin, "vex.UP")

class UpluginBridge {
public:
    explicit UpluginBridge(QMainWindow* window) : m_window(window) {}

    QString readDoc() {
        QPlainTextEdit* editor = activeEditor();
        return editor ? editor->toPlainText() : QString();
    }

    void writeDoc(const QString& content) {
        QPlainTextEdit* editor = activeEditor();
        if (editor) editor->setPlainText(content);
    }

    QString docPath() {
        QTabWidget* tabs = m_window->findChild<QTabWidget*>("VexTab");
        if (!tabs || tabs->currentIndex() < 0) return QString();
        return tabs->tabToolTip(tabs->currentIndex());
    }
    void addMenu(QMenu* menu) {
        QMenuBar* mb = m_window->menuBar();
        if (!mb) return;
        QAction* before = nullptr;
        const QList<QAction*> actions = mb->actions();
        for (QAction* a : actions) {
            QString text = a->text().remove('&').toLower();
            if (text == "help") { before = a; break; }
        }
        mb->insertMenu(before, menu);
    }
    bool isDocModified() {
        QPlainTextEdit* editor = activeEditor();
        return editor ? editor->document()->isModified() : false;
    }

    void saveDoc() {
        QPlainTextEdit* editor = activeEditor();
        if (!editor) return;
        QString path = docPath();
        if (path.isEmpty()) return;
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(editor->toPlainText().toUtf8());
            file.close();
            editor->document()->setModified(false);
        }
    }

    void openDoc(const QString& path) {
        QString tempDir = Settings::basePath() + "/.temp";
        QDir().mkpath(tempDir);
        QString reqFile = tempDir + "/" + QString::number(QCoreApplication::applicationPid()) + ".Req";
        QFile file(reqFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << path << "\n";
            file.close();
        }
    }

    void setConf(const QString& key, const QString& value) {
        Settings::instance().setValue(key, value);
    }

    QString conf(const QString& key) {
        return Settings::instance().get<QString>(key);
    }

    QToolBar* toolbar(const QString& name = "Vtoolbar") {
        const QList<QToolBar*> bars = m_window->findChildren<QToolBar*>(QString(), Qt::FindChildrenRecursively);
        for (QToolBar* tb : bars) {
            if (tb->objectName() == name) return tb;
        }
        return nullptr;
    }

    QToolBar* createToolbar(const QString& name) {
        QToolBar* tb = m_window->addToolBar(name);
        tb->setObjectName(name);
        return tb;
    }

    void removeToolbar(const QString& name) {
        QToolBar* tb = toolbar(name);
        if (tb) {
            m_window->removeToolBar(tb);
            delete tb;
        }
    }

    void addTool(QAction* action, const QString& tbName = "Vtoolbar") {
        QToolBar* tb = toolbar(tbName);
        if (tb) tb->addAction(action);
    }

    void removeTool(QAction* action, const QString& tbName = "Vtoolbar") {
        QToolBar* tb = toolbar(tbName);
        if (tb) tb->removeAction(action);
    }

    void addToolSeparator(const QString& tbName = "Vtoolbar") {
        QToolBar* tb = toolbar(tbName);
        if (tb) tb->addSeparator();
    }

    void clearToolbar(const QString& tbName = "Vtoolbar") {
        QToolBar* tb = toolbar(tbName);
        if (tb) tb->clear();
    }

    QMenu* addMenu(const QString& title) {
        QMenuBar* mb = m_window->menuBar();
        if (!mb) return nullptr;
        QMenu* menu = new QMenu(title);
        QAction* before = nullptr;
        const QList<QAction*> actions = mb->actions();
        for (QAction* a : actions) {
            QString text = a->text().remove('&').toLower();
            if (text == "help") { before = a; break; }
        }
        mb->insertMenu(before, menu);
        return menu;
    }

    void removeMenu(QMenu* menu) {
        QMenuBar* mb = m_window->menuBar();
        if (mb) mb->removeAction(menu->menuAction());
        delete menu;
    }

    void status(const QString& msg, int timeout) {
        m_window->statusBar()->showMessage(msg, timeout);
    }

    void dock(QDockWidget* dock, Qt::DockWidgetArea area = Qt::LeftDockWidgetArea) {
        m_window->addDockWidget(area, dock);
    }

    void undock(QDockWidget* dock) {
        m_window->removeDockWidget(dock);
    }

    void restart() {
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
        QCoreApplication::quit();
    }

    void quit() {
        QCoreApplication::quit();
    }

private:
    QMainWindow* m_window;

    QPlainTextEdit* activeEditor() {
        QTabWidget* tabs = m_window->findChild<QTabWidget*>("VexTab");
        if (!tabs) return nullptr;
        return qobject_cast<QPlainTextEdit*>(tabs->currentWidget());
    }
};
struct PluginInfo {
    QString name;
    QIcon icon;
    QString maintainer;
    QString description;
    QString version;
    QString filePath;
};

class UpluginLoader {
public:
    UpluginLoader() {}

    bool IsPluginTrusted(const QString& filePath) {
        return Settings::instance().contains("trusted/" + filePath);
    }

    void SetPluginTrustLevel(const QString& filePath, bool trust) {
        if (trust) Settings::instance().setValue("trusted/" + filePath, true);
        else Settings::instance().remove("trusted/" + filePath);
    }

    void ProbePluginDirectory(const QString& pluginsPath) {
        plugins.clear();
        QDir dir(pluginsPath);
        if (!dir.exists()) return;
        QStringList filters;
#ifdef Q_OS_WIN
        filters << "*.dll";
#elif defined(Q_OS_MAC)
        filters << "*.dylib";
#else
        filters << "*.so";
#endif
        const QStringList files = dir.entryList(filters, QDir::Files);
        for (const QString& file : files) {
            QString fullPath = dir.absoluteFilePath(file);
            QPluginLoader loader(fullPath);
            QObject* instance = loader.instance();
            if (!instance) continue;
            Uplugin* plugin = qobject_cast<Uplugin*>(instance);
            if (!plugin) continue;
            UpluginMetadata meta = plugin->metadata();
            PluginInfo info;
            info.name = meta.name; info.icon = meta.icon;
            info.maintainer = meta.maintainer; info.description = meta.description;
            info.version = meta.version; info.filePath = fullPath;
            plugins.append(info);
            loader.unload();
        }
    }

    void PresentUntrustedVerificationDialog(QWidget* parent) {
        const QList<PluginInfo> allPlugins = plugins;
        QList<PluginInfo> untrusted;
        for (const PluginInfo& p : allPlugins) { if (!IsPluginTrusted(p.filePath)) untrusted.append(p); }
        if (untrusted.isEmpty()) return;
        QDialog dlg(parent);
        dlg.setWindowTitle("Untrusted Plugins"); dlg.setModal(true); dlg.resize(650, 500);
        QVBoxLayout* ml = new QVBoxLayout(&dlg);
        QLabel* t = new QLabel("Unverified plugins detected"); t->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;"); ml->addWidget(t);
        QHBoxLayout* hl = new QHBoxLayout();
        QListWidget* lw = new QListWidget(); lw->setIconSize(QSize(32, 32)); lw->setMinimumWidth(200);
        QMap<int, PluginInfo*> im;
        for (int i = 0; i < untrusted.size(); ++i) {
            PluginInfo& info = untrusted[i]; QListWidgetItem* item = new QListWidgetItem();
            QIcon ei = info.icon.isNull() ? Settings::resolveIcon("application-x-addon") : info.icon;
            QPixmap c(48, 48); c.fill(Qt::transparent); QPainter p(&c);
            p.drawPixmap(0, 0, 32, 32, ei.pixmap(32, 32));
            p.drawPixmap(24, 24, 24, 24, Settings::resolveIcon("dialog-warning").pixmap(24, 24)); p.end();
            item->setIcon(QIcon(c)); item->setText(info.name); item->setToolTip(info.description);
            item->setData(Qt::UserRole, i); lw->addItem(item); im[i] = &info;
        }
        QVBoxLayout* dl = new QVBoxLayout();
        QLabel* ip = new QLabel(); ip->setFixedSize(128, 128); ip->setAlignment(Qt::AlignCenter); ip->setStyleSheet("background-color: transparent;"); dl->addWidget(ip, 0, Qt::AlignCenter);
        QLabel* nl = new QLabel("Plugin name"); nl->setStyleSheet("font-weight: bold;"); dl->addWidget(nl);
        QLabel* ml2 = new QLabel("Maintainer"); dl->addWidget(ml2);
        QLabel* vl = new QLabel("Version"); dl->addWidget(vl);
        dl->addSpacing(10);
        QLabel* dt = new QLabel("Details:"); dt->setStyleSheet("font-weight: bold;"); dl->addWidget(dt);
        QTextEdit* de = new QTextEdit(); de->setReadOnly(true); de->setMinimumHeight(120); dl->addWidget(de);
        QHBoxLayout* rl = new QHBoxLayout();
        QRadioButton* tr = new QRadioButton("Trust"); QRadioButton* ur = new QRadioButton("Untrust"); ur->setChecked(true);
        rl->addWidget(tr); rl->addWidget(ur); rl->addStretch(); dl->addLayout(rl);
        QWidget* dw = new QWidget(); dw->setLayout(dl); dw->setMinimumWidth(300);
        hl->addWidget(lw); hl->addWidget(dw); ml->addLayout(hl);
        QLabel* w = new QLabel("Plugins have full access to the editor."); w->setStyleSheet("color: red; font-weight: bold; padding: 5px;"); ml->addWidget(w);
        QHBoxLayout* bl = new QHBoxLayout();
        QPushButton* ta = new QPushButton("Trust All"); QPushButton* ua = new QPushButton("Untrust All"); QPushButton* db = new QPushButton("Done");
        bl->addWidget(ta); bl->addWidget(ua); bl->addStretch(); bl->addWidget(db); ml->addLayout(bl);
        int ci = -1;
        QObject::connect(lw, &QListWidget::currentItemChanged, [&](QListWidgetItem* cur, QListWidgetItem*) {
            if (!cur) return; ci = cur->data(Qt::UserRole).toInt(); PluginInfo* info = im.value(ci); if (!info) return;
            QIcon icon = info->icon.isNull() ? Settings::resolveIcon("application-x-addon") : info->icon;
            ip->setPixmap(icon.pixmap(128, 128)); nl->setText(info->name); ml2->setText(info->maintainer); vl->setText(info->version); de->setText(info->description); ur->setChecked(true);
        });
        lw->setCurrentRow(0);
        QObject::connect(ta, &QPushButton::clicked, [&]() { for (auto it = im.constBegin(); it != im.constEnd(); ++it) if (it.value()) SetPluginTrustLevel(it.value()->filePath, true); dlg.accept(); });
        QObject::connect(ua, &QPushButton::clicked, [&]() { for (auto it = im.constBegin(); it != im.constEnd(); ++it) if (it.value()) SetPluginTrustLevel(it.value()->filePath, false); dlg.accept(); });
        QObject::connect(db, &QPushButton::clicked, [&]() { if (ci >= 0) { PluginInfo* info = im.value(ci); if (info) SetPluginTrustLevel(info->filePath, tr->isChecked()); } dlg.accept(); });
        dlg.exec();
    }

    void ActivateTrustedPlugins(UpluginBridge* bridge, QMenu* extMenu) {
        const QList<PluginInfo> list = plugins;
        for (const PluginInfo& info : list) {
            if (!IsPluginTrusted(info.filePath)) continue;
            QPluginLoader* loader = new QPluginLoader(info.filePath);
            QObject* instance = loader->instance();
            if (!instance) { delete loader; continue; }
            Uplugin* plugin = qobject_cast<Uplugin*>(instance);
            if (!plugin) { loader->unload(); delete loader; continue; }
            plugin->setBridge(bridge);
            m_pluginMap[loader] = plugin;

            const QList<QDockWidget*> docks = plugin->dockWidgets();
            for (QDockWidget* dock : docks) {
                if (dock) {
                    bridge->dock(dock);
                    m_pluginDocks[loader].append(dock);
                }
            }
            const QList<QAction*> menuActs = plugin->menuActions();
            for (QAction* action : menuActs) {
                if (action && extMenu) {
                    extMenu->addAction(action);
                    m_pluginMenuActs[loader].append(action);
                }
            }
            const QList<QAction*> toolbarActs = plugin->toolbarActions();
            for (QAction* action : toolbarActs) {
                if (action) {
                    bridge->addTool(action);
                    m_pluginToolActs[loader].append(action);
                }
            }
            m_loaders.append(loader);
            loaded.append(plugin);
        }
    }

    void UnloadSinglePlugin(const QString& filePath) {
        for (int j = 0; j < m_loaders.size(); ++j) {
            QPluginLoader* loader = m_loaders[j];
            if (loader->fileName() == filePath) {
                Uplugin* plugin = m_pluginMap.value(loader, nullptr);
                if (plugin) {
                    for (QDockWidget* d : m_pluginDocks.value(loader))
                        delete d;
                    m_pluginDocks.remove(loader);
                    for (QAction* a : m_pluginMenuActs.value(loader))
                        delete a;
                    m_pluginMenuActs.remove(loader);
                    for (QAction* a : m_pluginToolActs.value(loader))
                        delete a;
                    m_pluginToolActs.remove(loader);

                    loaded.removeAll(plugin);
                    delete plugin;
                    m_pluginMap.remove(loader);
                }
                loader->unload();
                delete loader;
                m_loaders.removeAt(j);
                return;
            }
        }
    }

    void UnloadAllPlugins() {
        for (auto it = m_pluginMap.begin(); it != m_pluginMap.end(); ++it) {
            for (QDockWidget* d : m_pluginDocks.value(it.key()))
                delete d;
            for (QAction* a : m_pluginMenuActs.value(it.key()))
                delete a;
            for (QAction* a : m_pluginToolActs.value(it.key()))
                delete a;
        }
        m_pluginDocks.clear();
        m_pluginMenuActs.clear();
        m_pluginToolActs.clear();

        for (Uplugin* plugin : std::as_const(loaded)) delete plugin;
        loaded.clear();
        for (QPluginLoader* loader : std::as_const(m_loaders)) { loader->unload(); delete loader; }
        m_loaders.clear();
        m_pluginMap.clear();
    }

    QList<PluginInfo> plugins;
    QList<Uplugin*> loaded;

private:
    QList<QPluginLoader*> m_loaders;
    QMap<QPluginLoader*, Uplugin*> m_pluginMap;
    QMap<QPluginLoader*, QList<QDockWidget*>> m_pluginDocks;
    QMap<QPluginLoader*, QList<QAction*>> m_pluginMenuActs;
    QMap<QPluginLoader*, QList<QAction*>> m_pluginToolActs;
};

class PluginMan {
public:
    static void LaunchPluginManager(UpluginLoader* loader, UpluginBridge* bridge, const QString& pluginName, QWidget* parent) {
        QDialog dlg(parent); dlg.setWindowTitle("Plugin Manager"); dlg.setMinimumSize(650, 550);
        QVBoxLayout* ml = new QVBoxLayout(&dlg);
        QHBoxLayout* sl = new QHBoxLayout();
        QLineEdit* sb = new QLineEdit(); sb->setPlaceholderText("Search plugins..."); sb->setClearButtonEnabled(true);
        sl->addWidget(sb); ml->addLayout(sl);
        QLabel* t = new QLabel("Plugin Manager"); t->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;"); ml->addWidget(t);
        QHBoxLayout* hl = new QHBoxLayout();
        QListWidget* lw = new QListWidget(); lw->setIconSize(QSize(32, 32)); lw->setMinimumWidth(200);

        QVBoxLayout* dl = new QVBoxLayout();
        QLabel* ip = new QLabel(); ip->setFixedSize(128, 128); ip->setAlignment(Qt::AlignCenter); ip->setStyleSheet("background-color: transparent;"); dl->addWidget(ip, 0, Qt::AlignCenter);
        QLabel* nl = new QLabel(); nl->setStyleSheet("font-weight: bold;"); dl->addWidget(nl);
        QLabel* mll = new QLabel(); dl->addWidget(mll);
        QLabel* vl = new QLabel(); dl->addWidget(vl); dl->addSpacing(10);
        QLabel* dt = new QLabel("Details:"); dt->setStyleSheet("font-weight: bold;"); dl->addWidget(dt);
        QTextEdit* de = new QTextEdit(); de->setReadOnly(true); de->setMinimumHeight(80); dl->addWidget(de);
        QLabel* depl = new QLabel(); depl->setWordWrap(true); depl->setStyleSheet("color: #FF9800; font-weight: bold;"); depl->setVisible(false); dl->addWidget(depl); dl->addStretch();

        QHBoxLayout* bl = new QHBoxLayout();
        QPushButton* lb = new QPushButton(Settings::resolveIcon("process-start"), "Load");
#ifndef Q_OS_WIN
        QPushButton* ub = new QPushButton(Settings::resolveIcon("process-stop"), "Unload");
#endif
        QPushButton* rb = new QPushButton(Settings::resolveIcon("view-refresh"), "Refresh");
#ifndef Q_OS_WIN
        QPushButton* remb = new QPushButton(Settings::resolveIcon("list-remove"), "Remove");
#endif
        QPushButton* depb = new QPushButton(Settings::resolveIcon("emblem-system"), "Dependencies"); depb->setEnabled(false);
        QCheckBox* trustCheck = new QCheckBox("Trusted");
        bl->addWidget(lb);
#ifndef Q_OS_WIN
        bl->addWidget(ub);
#endif
        bl->addWidget(rb);
#ifndef Q_OS_WIN
        bl->addWidget(remb);
#endif
        bl->addWidget(depb);
        bl->addWidget(trustCheck);
        dl->addLayout(bl);

        QStackedWidget* detailsStack = new QStackedWidget();
        QWidget* detailsPage = new QWidget(); detailsPage->setLayout(dl);
        QWidget* emptyPage = new QWidget();
        QVBoxLayout* emptyLayout = new QVBoxLayout(emptyPage);
        emptyLayout->addStretch();
        QLabel* emptyIcon = new QLabel(); emptyIcon->setFixedSize(128, 128); emptyIcon->setAlignment(Qt::AlignCenter);
        emptyIcon->setPixmap(Settings::resolveIcon("application-x-addon").pixmap(96, 96));
        emptyLayout->addWidget(emptyIcon, 0, Qt::AlignCenter);
        QLabel* emptyLabel = new QLabel("No Plugins");
        QFont ef = emptyLabel->font(); ef.setBold(true); ef.setPointSize(14); emptyLabel->setFont(ef);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #888;");
        emptyLayout->addWidget(emptyLabel);
        QLabel* emptySub = new QLabel("No plugins discovered.\nPlace .so/.dll/.dylib files in ~/.vex/plugins/");
        emptySub->setAlignment(Qt::AlignCenter);
        emptySub->setWordWrap(true);
        emptySub->setStyleSheet("color: #aaa;");
        emptyLayout->addWidget(emptySub);
#ifdef Q_OS_WIN
        QLabel* winNote = new QLabel("Windows does not support removing DLLs while the application is running.");
        winNote->setAlignment(Qt::AlignCenter);
        winNote->setWordWrap(true);
        winNote->setStyleSheet("color: #FF9800; font-weight: bold; padding: 5px;");
        emptyLayout->addWidget(winNote);
#endif
        emptyLayout->addStretch();
        detailsStack->addWidget(detailsPage);
        detailsStack->addWidget(emptyPage);
        detailsStack->setMinimumWidth(300);

        hl->addWidget(lw);
        hl->addWidget(detailsStack);
        ml->addLayout(hl);

        const QList<PluginInfo> ap = loader->plugins;
        if (ap.isEmpty()) {
            detailsStack->setCurrentIndex(1);
            lb->setEnabled(false);
#ifndef Q_OS_WIN
            ub->setEnabled(false);
            remb->setEnabled(false);
#endif
            depb->setEnabled(false);
        } else {
            detailsStack->setCurrentIndex(0);
            lb->setEnabled(true);
#ifndef Q_OS_WIN
            ub->setEnabled(true);
            remb->setEnabled(true);
#endif
        }

        QMap<int, PluginInfo*> im; int sr = -1;
        for (int i = 0; i < ap.size(); ++i) {
            const PluginInfo& info = ap[i];
            QListWidgetItem* item = new QListWidgetItem();
            item->setIcon(info.icon.isNull() ? Settings::resolveIcon("application-x-addon") : info.icon);
            item->setText(info.name + " (" + info.version + ")");
            item->setData(Qt::UserRole, i);
            item->setToolTip(info.description);
            lw->addItem(item);
            im[i] = const_cast<PluginInfo*>(&info);
            QFileInfo fi(info.filePath);
            if (fi.baseName() == pluginName || fi.fileName() == pluginName) sr = i;
        }

        QCheckBox* stc = new QCheckBox("Load plugins without checking trust");
        stc->setChecked(Settings::instance().get<bool>("uplugin/skipTrustCheck", false));
        QObject::connect(stc, &QCheckBox::toggled, [](bool c) { Settings::instance().setValue("uplugin/skipTrustCheck", c); });
        ml->addWidget(stc);

        QHBoxLayout* bol = new QHBoxLayout();
        QPushButton* grb = new QPushButton(Settings::resolveIcon("view-refresh"), "Refresh All");
        QPushButton* cb = new QPushButton("Close");
        bol->addWidget(grb); bol->addStretch(); bol->addWidget(cb);
        ml->addLayout(bol);

        PluginInfo* ci = nullptr; QList<Dependency> cd;
        auto LoadPluginMeta = [&](PluginInfo* info) {
            if (!info) return;
            QPluginLoader ldr(info->filePath);
            QObject* inst = ldr.instance();
            Uplugin* up = inst ? qobject_cast<Uplugin*>(inst) : nullptr;
            if (up) {
                UpluginMetadata meta = up->metadata();
                cd = meta.depends;
                depl->setVisible(!meta.depends.isEmpty());
                if (!meta.depends.isEmpty()) {
                    QString dt;
                    for (const Dependency& d : meta.depends) dt += d.name + " ";
                    depl->setText("Dependencies: " + dt);
                }
                depb->setEnabled(!meta.depends.isEmpty());
            } else {
                depl->setVisible(false);
                depb->setEnabled(false);
            }
            ldr.unload();
        };

        auto RefreshList = [&]() {
            lw->blockSignals(true);
            lw->clear();
            im.clear();
            const QList<PluginInfo> ps = loader->plugins;
            if (ps.isEmpty()) {
                detailsStack->setCurrentIndex(1);
                lb->setEnabled(false);
#ifndef Q_OS_WIN
                ub->setEnabled(false);
                remb->setEnabled(false);
#endif
                depb->setEnabled(false);
            } else {
                detailsStack->setCurrentIndex(0);
                lb->setEnabled(true);
#ifndef Q_OS_WIN
                ub->setEnabled(true);
                remb->setEnabled(true);
#endif
            }
            for (int i = 0; i < ps.size(); ++i) {
                const PluginInfo& info = ps[i];
                QListWidgetItem* item = new QListWidgetItem();
                item->setIcon(info.icon.isNull() ? Settings::resolveIcon("application-x-addon") : info.icon);
                item->setText(info.name + " (" + info.version + ")");
                item->setData(Qt::UserRole, i);
                item->setToolTip(info.description);
                lw->addItem(item);
                im[i] = const_cast<PluginInfo*>(&info);
            }
            depl->setVisible(false);
            depb->setEnabled(false);
            ci = nullptr;
            cd.clear();
            trustCheck->setChecked(false);
            lw->blockSignals(false);
        };

        QObject::connect(sb, &QLineEdit::textChanged, [&](const QString& tx) {
            for (int i = 0; i < lw->count(); ++i)
                lw->item(i)->setHidden(!tx.isEmpty() && !lw->item(i)->text().contains(tx, Qt::CaseInsensitive));
        });

        QObject::connect(lw, &QListWidget::currentItemChanged, [&](QListWidgetItem* cur, QListWidgetItem*) {
            if (!cur) {
                depl->setVisible(false); depb->setEnabled(false);
                detailsStack->setCurrentIndex(1);
                trustCheck->setChecked(false);
                trustCheck->setEnabled(false);
                ci = nullptr;
                return;
            }
            int idx = cur->data(Qt::UserRole).toInt();
            ci = im.value(idx);
            if (!ci) return;
            detailsStack->setCurrentIndex(0);
            QIcon icon = ci->icon.isNull() ? Settings::resolveIcon("application-x-addon") : ci->icon;
            ip->setPixmap(icon.pixmap(128, 128));
            nl->setText(ci->name);
            mll->setText(ci->maintainer);
            vl->setText(ci->version);
            de->setText(ci->description);
            LoadPluginMeta(ci);
            trustCheck->blockSignals(true);
            trustCheck->setChecked(loader->IsPluginTrusted(ci->filePath));
            trustCheck->setEnabled(true);
            trustCheck->blockSignals(false);
        });

        if (sr >= 0) lw->setCurrentRow(sr);

        QObject::connect(depb, &QPushButton::clicked, [&]() {
            if (cd.isEmpty()) return;
            QString dl2;
            for (const Dependency& d : cd) dl2 += "<li><b>" + d.name + "</b></li>";
            QMessageBox mb(&dlg);
            mb.setWindowTitle("Dependencies Required");
            mb.setText("This plugin needs:");
            mb.setInformativeText("<ul>" + dl2 + "</ul>");
            mb.setIcon(QMessageBox::Information);
            QPushButton* ob = mb.addButton("Open Links", QMessageBox::AcceptRole);
            mb.addButton("Cancel", QMessageBox::RejectRole);
            mb.exec();
            if (mb.clickedButton() == ob) {
                for (const Dependency& d : cd)
                    if (!d.downloadLink.isEmpty())
                        QDesktopServices::openUrl(QUrl(d.downloadLink));
                QMessageBox::information(&dlg, "Install",
                                         "After download:\n.vxpkg: Extension Manager or vex -i\n.so/.dll/.dylib: place in ~/.vex/plugins/");
            }
        });

        QObject::connect(rb, &QPushButton::clicked, [&]() {
            loader->UnloadAllPlugins();
            loader->ProbePluginDirectory(Settings::basePath()+"/plugins");
            loader->ActivateTrustedPlugins(bridge, nullptr);
            RefreshList();
        });

        QObject::connect(grb, &QPushButton::clicked, [&]() { RefreshList(); });

        QObject::connect(lb, &QPushButton::clicked, [loader, bridge]() {
            loader->ActivateTrustedPlugins(bridge, nullptr);
        });

#ifndef Q_OS_WIN
        QObject::connect(ub, &QPushButton::clicked, [&]() {
            if (!ci) return;
            loader->UnloadSinglePlugin(ci->filePath);
            RefreshList();
        });

        QObject::connect(remb, &QPushButton::clicked, [&]() {
            if (!ci) return;
            auto r = QMessageBox::question(&dlg, "Remove", QString("Remove '%1'?").arg(ci->name),
                                           QMessageBox::Yes|QMessageBox::No);
            if (r != QMessageBox::Yes) return;
            QString fp = ci->filePath;
            loader->SetPluginTrustLevel(fp, false);
            QFile::remove(fp);
            for (int i = 0; i < loader->plugins.size(); ++i) {
                if (loader->plugins[i].filePath == fp) {
                    loader->plugins.removeAt(i);
                    break;
                }
            }
            RefreshList();
        });
#endif

        QObject::connect(trustCheck, &QCheckBox::toggled, [&](bool checked) {
            if (!ci) return;
            loader->SetPluginTrustLevel(ci->filePath, checked);
        });

        QObject::connect(cb, &QPushButton::clicked, &dlg, &QDialog::accept);
        dlg.exec();
    }
};

#endif // UPLUGIN_H
