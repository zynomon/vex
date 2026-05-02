/****************************************************************
*                                                              *
*                         Apache 2.0                           *
*     Copyright Zynomon aelius <zynomon@proton.me>  2026       *
*               Project         :        Vex                   *
*               Version         :        4.3 (Cytoplasm)       *
*                                                              *
****************************************************************/

#include "Uplugin.H"
#include "qlightterminal.h"

#include <QClipboard>
#include <QColorDialog>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QUrl>

struct ShellProfile {
    QString name, path;
    static QList<ShellProfile> All() {
        QList<ShellProfile> out;
#ifdef Q_OS_WIN
        const char* exes[] = {"cmd.exe", "powershell.exe", "pwsh.exe", "wsl.exe"};
        for (const auto& n : exes) {
            QString p = QStandardPaths::findExecutable(n);
            if (!p.isEmpty()) {
                QString nm(n);
                nm.replace(".exe", "");
                nm[0] = nm.at(0).toUpper();
                out.append({nm, p});
            }
        }
        QString bash = QStandardPaths::findExecutable("bash.exe");
        if (!bash.isEmpty()) out.append({"Bash", bash});
#else
        QFile sf("/etc/shells");
        if (sf.open(QIODevice::ReadOnly)) {
            QTextStream in(&sf);
            while (!in.atEnd()) {
                QString l = in.readLine().trimmed();
                if (l.isEmpty() || l.startsWith("#")) continue;
                if (QFile::exists(l))
                    out.append({QFileInfo(l).baseName(), l});
            }
        }
        if (out.isEmpty()) {
            const char* names[] = {"bash", "zsh", "fish", "dash", "sh"};
            for (const auto& n : names) {
                QString p = QStandardPaths::findExecutable(n);
                if (!p.isEmpty()) {
                    QString nm(n);
                    nm[0] = nm.at(0).toUpper();
                    out.append({nm, p});
                }
            }
        }
#endif
        return out;
    }
};

class TermTab : public QWidget {
    Q_OBJECT
public:
    QLightTerminal* term;
    TermTab(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* l = new QVBoxLayout(this);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        term = new QLightTerminal;
        term->setFontSize(10, QFont::Normal);
        term->setPadding(8, 0);
        term->setLineHeightScale(1.25);
        l->addWidget(term);
    }
};

class TerminalPlugin : public QObject, public Uplugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "vex.UP")
    Q_INTERFACES(Uplugin)

    UpluginBridge* br = nullptr;
    QDockWidget* dk = nullptr;
    QTabWidget* tb = nullptr;
    QString lastShell;

public:
    UpluginMetadata metadata() const override {
        UpluginMetadata m;
        m.name = "Terminal"; m.version = "1.0";
        m.maintainer = "Vex team"; m.description = "Terminal emulator using st ( Simple terminal )";
        m.icon = Settings::resolveIcon("utilities-terminal");
        return m;
    }

    QList<QDockWidget*> dockWidgets() const override {
        return dk ? QList<QDockWidget*>{dk} : QList<QDockWidget*>{};
    }
    QList<QAction*> menuActions() const override { return {}; }
    QList<QAction*> toolbarActions() const override { return {}; }

    void setBridge(UpluginBridge* b) override {
        br = b;
        QSettings s(confPath(), QSettings::IniFormat);
        lastShell = s.value("shell").toString();
        QString schemePath = s.value("colorscheme").toString();
        BuildUI();
        if (!schemePath.isEmpty()) {
            for (int i = 0; i < tb->count(); ++i) {
                TermTab* t = qobject_cast<TermTab*>(tb->widget(i));
                if (t) t->term->loadColorScheme(schemePath);
            }
        }
    }

private:
    QString confPath() const { return Settings::basePath() + "/terminal.conf"; }

    QString WorkDir() const {
        QString p = br->docPath();
        if (!p.isEmpty()) { QFileInfo fi(p); return fi.isDir() ? p : fi.absolutePath(); }
        return QDir::currentPath();
    }

    QString DefShell() const {
        if (!lastShell.isEmpty()) return lastShell;
#ifdef Q_OS_WIN
        return qEnvironmentVariable("COMSPEC", "cmd.exe");
#else
        return qEnvironmentVariable("SHELL", "/bin/bash");
#endif
    }

    void BuildUI() {
        dk = new QDockWidget("Terminal");
        dk->setObjectName("TerminalDock");

        QWidget* w = new QWidget;
        QVBoxLayout* vl = new QVBoxLayout(w);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(0);

        tb = new QTabWidget;
        tb->setTabsClosable(true);
        tb->setMovable(true);
        tb->setDocumentMode(true);
        tb->tabBar()->setExpanding(true);

        QWidget* corner = new QWidget;
        QHBoxLayout* cl = new QHBoxLayout(corner);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(2);

        QPushButton* addBtn = new QPushButton;
        addBtn->setIcon(Settings::resolveIcon("list-add"));
        addBtn->setToolTip("New Terminal");

        cl->addWidget(addBtn);
        tb->setCornerWidget(corner);

        connect(tb->tabBar(), &QTabBar::tabCloseRequested, [this](int i) {
            if (tb->count() <= 1) return;
            delete tb->widget(i);
        });

        connect(tb->tabBar(), &QTabBar::tabBarDoubleClicked, [this](int i) {
            bool ok;
            QString n = QInputDialog::getText(dk, "Rename", "Name:", QLineEdit::Normal, tb->tabText(i), &ok);
            if (ok && !n.isEmpty()) tb->setTabText(i, n);
        });

        connect(addBtn, &QPushButton::clicked, [this] { NewTab(); });

        vl->addWidget(tb);
        dk->setWidget(w);

        NewTab();

        QShortcut* sc = new QShortcut(QKeySequence("F4"), dk);
        connect(sc, &QShortcut::activated, [this] {
            if (!dk->isVisible()) dk->setVisible(true);
            NewTab();
        });
    }

    void NewTab() {
        TermTab* t = new TermTab;

        connect(t->term, &QLightTerminal::contextMenuRequested, [this](QMenu* menu) {
            menu->addSeparator();
            QMenu* nm = menu->addMenu(Settings::resolveIcon("list-add"), "New Tab With");
            const QList<ShellProfile> shells = ShellProfile::All();
            for (const auto& sp : shells) {
                nm->addAction(Settings::resolveIcon("utilities-terminal"), sp.name, [this, sp] {
                    lastShell = sp.path;
                    QSettings s(confPath(), QSettings::IniFormat);
                    s.setValue("shell", sp.path);
                    s.sync();
                    NewTab();
                });
            }
            nm->addSeparator();
            nm->addAction(Settings::resolveIcon("document-open"), "Custom Shell...", [this] {
                QString p = QFileDialog::getOpenFileName(dk, "Select Shell Executable");
                if (!p.isEmpty()) {
                    lastShell = p;
                    QSettings s(confPath(), QSettings::IniFormat);
                    s.setValue("shell", p);
                    s.sync();
                    NewTab();
                }
            });
            menu->addSeparator();
            menu->addAction(Settings::resolveIcon("preferences-system"), "Configure...", [this] { ConfigDialog(); });
        });

        int idx = tb->addTab(t, Settings::resolveIcon("utilities-terminal"), "Terminal");
        tb->setCurrentIndex(idx);
    }

    void ConfigDialog() {
        QDialog dlg(dk);
        dlg.setWindowTitle("Terminal Configuration");
        dlg.setMinimumWidth(420);

        QVBoxLayout* vl = new QVBoxLayout(&dlg);
        QFormLayout* f = new QFormLayout;

        QLineEdit* shellEdit = new QLineEdit(lastShell.isEmpty() ? DefShell() : lastShell);
        QPushButton* shellBrowse = new QPushButton("Browse...");
        QHBoxLayout* shellRow = new QHBoxLayout;
        shellRow->addWidget(shellEdit);
        shellRow->addWidget(shellBrowse);
        f->addRow("Default Shell:", shellRow);
        connect(shellBrowse, &QPushButton::clicked, [&] {
            QString p = QFileDialog::getOpenFileName(&dlg, "Select Shell");
            if (!p.isEmpty()) shellEdit->setText(p);
        });

        QPushButton* fontBtn = new QPushButton("Monospace 10");
        QFont currentFont("Monospace", 10);
        connect(fontBtn, &QPushButton::clicked, [&] {
            bool ok;
            QFont ft = QFontDialog::getFont(&ok, currentFont, &dlg);
            if (ok) { currentFont = ft; fontBtn->setText(ft.family() + " " + QString::number(ft.pointSize())); }
        });
        f->addRow("Font:", fontBtn);

        QLineEdit* schemePath = new QLineEdit;
        schemePath->setPlaceholderText("Path to color scheme file");
        QPushButton* schemeBrowse = new QPushButton("Browse...");
        QHBoxLayout* schemeRowLay = new QHBoxLayout;
        schemeRowLay->addWidget(schemePath);
        schemeRowLay->addWidget(schemeBrowse);
        f->addRow("Color Scheme:", schemeRowLay);
        connect(schemeBrowse, &QPushButton::clicked, [&] {
            QString p = QFileDialog::getOpenFileName(&dlg, "Open Color Scheme", QDir::homePath(),
                                                     "All Schemes (*.colorscheme *.itermcolors *.json *.Xdefaults *.Xresources);;"
                                                     "Konsole (*.colorscheme);;iTerm2 (*.itermcolors);;"
                                                     "VSCode/Windows Terminal (*.json);;Xdefaults (*.Xdefaults *.Xresources)");
            if (!p.isEmpty()) schemePath->setText(p);
        });

        vl->addLayout(f);

        QHBoxLayout* bl = new QHBoxLayout;
        QPushButton* apply  = new QPushButton("Apply");
        QPushButton* ok     = new QPushButton("OK");
        QPushButton* cancel = new QPushButton("Cancel");
        bl->addStretch();
        bl->addWidget(apply);
        bl->addWidget(ok);
        bl->addWidget(cancel);
        vl->addLayout(bl);

        connect(apply, &QPushButton::clicked, [&] {
            lastShell = shellEdit->text().trimmed();
            QString sp = schemePath->text().trimmed();
            for (int i = 0; i < tb->count(); ++i) {
                TermTab* t = qobject_cast<TermTab*>(tb->widget(i));
                if (t) {
                    t->term->setFontFamily(currentFont.family());
                    t->term->setFontSize(currentFont.pointSize(), currentFont.weight());
                    if (!sp.isEmpty()) t->term->loadColorScheme(sp);
                }
            }
        });

        connect(ok, &QPushButton::clicked, [&] {
            lastShell = shellEdit->text().trimmed();
            QString sp = schemePath->text().trimmed();
            QSettings s(confPath(), QSettings::IniFormat);
            s.setValue("shell", lastShell);
            if (!sp.isEmpty()) s.setValue("colorscheme", sp);
            s.setValue("fontFamily", currentFont.family());
            s.setValue("fontSize", currentFont.pointSize());
            s.sync();
            for (int i = 0; i < tb->count(); ++i) {
                TermTab* t = qobject_cast<TermTab*>(tb->widget(i));
                if (t) {
                    t->term->setFontFamily(currentFont.family());
                    t->term->setFontSize(currentFont.pointSize(), currentFont.weight());
                    if (!sp.isEmpty()) t->term->loadColorScheme(sp);
                }
            }
            dlg.accept();
        });

        connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);

        dlg.exec();
    }
};

#include "Terminal.moc"
