/****************************************************************
*                                                              *
*                         Apache 2.0                           *
*     Copyright Zynomon aelius <zynomon@proton.me>  2026       *
*               Project         :        Vex                   *
*               Version         :        4.3 (Cytoplasm)       *
*                                                              *
****************************************************************/

#include <QObject>
#include <QtPlugin>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>
#include <QCoreApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QCheckBox>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QApplication>
#include <QDateTime>
#include <QUrl>
#include <QDesktopServices>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFileIconProvider>
#include <QScrollArea>
#include <QTimer>
#include <QComboBox>
#include <QPainter>
#include "Uplugin.H"
#include "Settings.H"
#include "Plugvex.H"

class PackageUtil {
public:
    static QString FindArchiver() {
        const QStringList names = {
#ifdef Q_OS_WIN
            "7z.exe", "7za.exe"
#else
            "7z", "7za", "7zz"
#endif
        };
        for (const QString& name : names) {
            QString path = QStandardPaths::findExecutable(name);
            if (!path.isEmpty()) return path;
        }
        const QStringList commonPaths = {
#ifdef Q_OS_WIN
            "C:/Program Files/7-Zip/7z.exe", "C:/Program Files (x86)/7-Zip/7z.exe"
#else
            "/usr/bin/7z", "/usr/local/bin/7z", "/usr/bin/7zz", "/usr/local/bin/7zz"
#endif
        };
        for (const QString& path : commonPaths) {
            if (QFile::exists(path)) return path;
        }
        return QString();
    }

    static void AlertArchiverMissing(QWidget* parent) {
        QDialog dlg(parent);
        dlg.setWindowTitle("7-Zip Required");
        dlg.setMinimumSize(450, 250);
        QVBoxLayout* layout = new QVBoxLayout(&dlg);
        QLabel* iconLabel = new QLabel();
        iconLabel->setPixmap(Settings::resolveIcon("dialog-warning").pixmap(64, 64));
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);
        QLabel* titleLabel = new QLabel("7-Zip is required for package operations");
        titleLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);
        QLabel* descLabel = new QLabel();
        descLabel->setWordWrap(true);
        descLabel->setText(
            "Vex uses 7-Zip to create and extract .vxpkg package files.\n\n"
            "Please install 7-Zip:\n\n"
#ifdef Q_OS_WIN
            "  https://www.7-zip.org/download.html\n"
#elif defined(Q_OS_MAC)
            "  brew install p7zip\n"
#else
            "  sudo apt install p7zip-full    (Debian/Ubuntu)\n"
            "  sudo dnf install p7zip          (Fedora)\n"
            "  sudo pacman -S p7zip            (Arch)\n"
#endif
            "\nAfter installation, restart Vex."
            );
        layout->addWidget(descLabel);
        QPushButton* okBtn = new QPushButton("OK");
        QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        layout->addWidget(okBtn, 0, Qt::AlignCenter);
        dlg.exec();
    }

    static void CopyDirectory(const QString& src, const QString& dst) {
        QDir srcDir(src);
        if (!srcDir.exists()) return;
        QDir().mkpath(dst);
        const QStringList entries = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            QString sp = src + "/" + entry, dp = dst + "/" + entry;
            QFileInfo fi(sp);
            if (fi.isDir()) { CopyDirectory(sp, dp); }
            else { if (QFile::exists(dp)) QFile::remove(dp); QFile::copy(sp, dp); }
        }
    }

    static QIcon DerivePluginIcon(const QString& iconVal) {
        if (iconVal.isEmpty()) return Settings::resolveIcon("application-x-addon");
        if (iconVal.endsWith(".png") || iconVal.endsWith(".svg") ||
            iconVal.endsWith(".ico") || iconVal.endsWith(".icns") ||
            iconVal.endsWith(".jpg") || iconVal.endsWith(".jpeg"))
            return QIcon(iconVal);
        QIcon icon = Settings::resolveIcon(iconVal);
        return icon.isNull() ? Settings::resolveIcon("application-x-addon") : icon;
    }

    static QIcon DeriveDirectoryIcon(const QString& dirPath, const QIcon& fallback) {
        const QStringList exts = {"*.png", "*.svg", "*.ico", "*.icns", "*.jpg", "*.jpeg"};
        for (const QString& ext : exts) {
            QStringList matches = QDir(dirPath).entryList({ext}, QDir::Files);
            if (!matches.isEmpty()) return QIcon(dirPath + "/" + matches.first());
        }
        return fallback;
    }

    static QIcon DeriveGenericIcon(const QString& iconVal, const QString& dirPath, bool isIconTheme = false) {
        if (isIconTheme) {
            QString previewPath = dirPath + "/.theme_preview.png";
            if (QFile::exists(previewPath)) return QIcon(previewPath);
            QStringList allIcons;
            const QStringList exts = {"*.png", "*.svg", "*.ico", "*.icns"};
            for (const QString& ext : exts) {
                QStringList found = QDir(dirPath).entryList({ext}, QDir::Files);
                for (const QString& f : found) allIcons.append(dirPath + "/" + f);
            }
            if (!allIcons.isEmpty()) {
                QPixmap combined(64, 64);
                combined.fill(Qt::transparent);
                QPainter painter(&combined);
                int count = qMin(4, allIcons.size());
                int cols = qMin(2, count);
                int cellW = 64 / cols;
                int cellH = count > 2 ? 32 : 64;
                for (int i = 0; i < count; ++i) {
                    QPixmap iconPm(allIcons[i]);
                    int row = i / cols;
                    int col = i % cols;
                    painter.drawPixmap(col * cellW, row * cellH, cellW, cellH,
                                       iconPm.scaled(cellW, cellH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
                painter.end();
                combined.save(previewPath, "PNG");
                return QIcon(combined);
            }
            return Settings::resolveIcon("folder");
        }
        if (iconVal == "true" || iconVal.isEmpty()) {
            return DeriveDirectoryIcon(dirPath, Settings::resolveIcon("application-x-addon"));
        }
        return DerivePluginIcon(iconVal);
    }

    static QString ClassifyAssetType(const QString& filePath, const QString& basePath) {
        QFileInfo fi(filePath);
        QString fn = fi.fileName();
        if (fn.endsWith(".vxsyn")) return "vex.syntax";
        if (fn.endsWith(".qss")) return "vex.theme.qss";
        if (fn.endsWith(".so") || fn.endsWith(".dll") || fn.endsWith(".dylib")) return "vex.plugin";
        if (fi.isDir()) return "vex.theme.icon";
        if (filePath.startsWith(basePath + "/syntax")) return "vex.syntax";
        if (filePath.startsWith(basePath + "/themes/stylesheets")) return "vex.theme.qss";
        if (filePath.startsWith(basePath + "/themes/icon")) return "vex.theme.icon";
        if (filePath.startsWith(basePath + "/plugins")) return "vex.plugin";
        return QString();
    }

    static QString ResolvePackageOwner(const QString& filePath, const QString& assetsPath) {
        QDir ad(assetsPath);
        if (!ad.exists()) return QString();
        const QStringList pkgs = ad.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& pkg : pkgs) {
            QString cp = assetsPath + "/" + pkg + "/data.conf";
            if (!QFile::exists(cp)) continue;
            QSettings conf(cp, QSettings::IniFormat);
            const QStringList groups = {"vex.plugin", "vex.theme.qss", "vex.theme.icon", "vex.syntax"};
            for (const QString& g : groups) {
                conf.beginGroup(g);
                const QStringList keys = conf.childKeys();
                for (const QString& key : keys) {
                    if (conf.value(key).toString() == QFileInfo(filePath).fileName()) {
                        conf.endGroup();
                        return pkg;
                    }
                }
                conf.endGroup();
            }
        }
        return QString();
    }

    static void StripConfigEntry(const QString& confPath, const QString& fileName) {
        if (!QFile::exists(confPath)) return;
        QSettings conf(confPath, QSettings::IniFormat);
        const QStringList groups = {"vex.plugin", "vex.theme.qss", "vex.theme.icon", "vex.syntax"};
        bool modified = false;
        for (const QString& g : groups) {
            conf.beginGroup(g);
            const QStringList keys = conf.childKeys();
            for (const QString& key : keys) {
                if (conf.value(key).toString() == fileName) { conf.remove(key); modified = true; }
            }
            conf.endGroup();
        }
        if (modified) conf.sync();
        bool hasAny = false;
        for (const QString& g : groups) { conf.beginGroup(g); hasAny = hasAny || !conf.childKeys().isEmpty(); conf.endGroup(); }
        if (!hasAny) { QDir(QFileInfo(confPath).absolutePath()).removeRecursively(); }
    }

    static QString GetCurrentOS() {
#ifdef Q_OS_WIN
        return "win";
#elif defined(Q_OS_MACOS)
        return "mac";
#elif defined(Q_OS_LINUX)
        return "linux";
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD) || defined(Q_OS_NETBSD)
        return "bsd";
#else
        return "any";
#endif
    }
};

class DataEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit DataEditorDialog(const QString& folderPath, QWidget* parent = nullptr)
        : QDialog(parent), m_folderPath(folderPath) {
        setWindowTitle("Configure package.data");
        setMinimumSize(650, 550);
        setAcceptDrops(true);
        QVBoxLayout* ml = new QVBoxLayout(this);
        QFormLayout* mf = new QFormLayout();
        m_nameEdit = new QLineEdit(QDir(folderPath).dirName());
        m_versionEdit = new QLineEdit("1.0");
        m_maintainerEdit = new QLineEdit();
        m_descEdit = new QTextEdit();
        m_descEdit->setMaximumHeight(60);

        // OS selector
        m_osCombo = new QComboBox();
        m_osCombo->addItem("Any (OS-independent)", "any");
        m_osCombo->addItem("Windows", "win");
        m_osCombo->addItem("Linux", "linux");
        m_osCombo->addItem("macOS", "mac");
        m_osCombo->addItem("BSD", "bsd");

        // Load existing data if available
        QString dp = folderPath + "/plugin.data";
        if (QFile::exists(dp)) {
            QSettings pk(dp, QSettings::IniFormat);
            m_nameEdit->setText(pk.value("vex.metadata/name", QDir(folderPath).dirName()).toString());
            m_versionEdit->setText(pk.value("vex.metadata/version", "1.0").toString());
            m_maintainerEdit->setText(pk.value("vex.metadata/maintainer").toString());
            m_descEdit->setPlainText(pk.value("vex.metadata/description").toString());

            QString osVal = pk.value("vex.metadata/os", "any").toString().toLower();
            int osIdx = m_osCombo->findData(osVal);
            if (osIdx >= 0) m_osCombo->setCurrentIndex(osIdx);
        }

        mf->addRow("Name:", m_nameEdit);
        mf->addRow("Version:", m_versionEdit);
        mf->addRow("Target OS:", m_osCombo);
        mf->addRow("Maintainer:", m_maintainerEdit);
        mf->addRow("Description:", m_descEdit);
        ml->addLayout(mf);

        m_iconLabel = new QLabel();
        m_iconLabel->setFixedSize(64, 64);
        m_iconLabel->setAlignment(Qt::AlignCenter);
        m_iconLabel->setPixmap(Settings::resolveIcon("application-x-addon").pixmap(64, 64));
        QPushButton* ib = new QPushButton("Set Icon...");
        QHBoxLayout* il = new QHBoxLayout();
        il->addWidget(m_iconLabel);
        il->addWidget(ib);
        ml->addLayout(il);
        QObject::connect(ib, &QPushButton::clicked, [this]() {
            QString p = QFileDialog::getOpenFileName(this, "Select Icon", QString(), "Images (*.png *.svg *.ico *.icns *.jpg *.jpeg)");
            if (!p.isEmpty()) { m_iconPath = p; m_iconLabel->setPixmap(QPixmap(p).scaled(64, 64, Qt::KeepAspectRatio)); }
        });

        // Load existing icon if present
        if (QFile::exists(dp)) {
            QSettings pk(dp, QSettings::IniFormat);
            QString iconVal = pk.value("vex.metadata/icon").toString();
            if (!iconVal.isEmpty() && iconVal != "true") {
                QString iconPath = folderPath + "/" + iconVal;
                if (QFile::exists(iconPath)) {
                    m_iconPath = iconPath;
                    m_iconLabel->setPixmap(QPixmap(iconPath).scaled(64, 64, Qt::KeepAspectRatio));
                }
            }
        }

        m_fileTree = new QTreeWidget();
        m_fileTree->setHeaderLabels({"Type", "File"});
        ml->addWidget(m_fileTree);
        QHBoxLayout* bl = new QHBoxLayout();
        QPushButton* ab = new QPushButton(Settings::resolveIcon("list-add"), "Add File");
        QPushButton* rb = new QPushButton(Settings::resolveIcon("list-remove"), "Remove");
        bl->addWidget(ab);
        bl->addWidget(rb);
        bl->addStretch();
        ml->addLayout(bl);
        QObject::connect(ab, &QPushButton::clicked, [this]() {
            QString p = QFileDialog::getOpenFileName(this, "Add File", Settings::basePath(), "All Files (*)");
            if (!p.isEmpty()) addFile(p);
        });
        QObject::connect(rb, &QPushButton::clicked, [this, rb]() {
            QTreeWidgetItem* c = m_fileTree->currentItem();
            if (c && c->parent()) { delete c->parent()->takeChild(c->parent()->indexOfChild(c)); }
            rb->setEnabled(false);
        });
        QObject::connect(m_fileTree, &QTreeWidget::currentItemChanged, [rb](QTreeWidgetItem* c, QTreeWidgetItem*) {
            rb->setEnabled(c && c->parent());
        });
        rb->setEnabled(false);
        QHBoxLayout* bot = new QHBoxLayout();
        QPushButton* ok = new QPushButton("Save && Close");
        QPushButton* cancel = new QPushButton("Cancel");
        bot->addStretch();
        bot->addWidget(ok);
        bot->addWidget(cancel);
        ml->addLayout(bot);
        QObject::connect(ok, &QPushButton::clicked, [this]() {
            QTreeWidgetItem* unknownItem = findTL("Unknown");
            if (unknownItem && unknownItem->childCount() > 0) {
                QMessageBox::warning(this, "Unknown Files", "Remove unknown files or assign a type before saving.");
                return;
            }
            saveDataFile();
            accept();
        });
        QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        QObject::connect(m_nameEdit, &QLineEdit::textChanged, [ok, this]() {
            ok->setEnabled(!m_nameEdit->text().trimmed().isEmpty());
        });
        populateFromFolder();
    }
protected:
    void dragEnterEvent(QDragEnterEvent* e) override { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
    void dropEvent(QDropEvent* e) override {
        const QList<QUrl> urls = e->mimeData()->urls();
        for (const QUrl& u : urls) addFile(u.toLocalFile());
    }
private:
    QString m_folderPath, m_iconPath;
    QLineEdit* m_nameEdit;
    QLineEdit* m_versionEdit;
    QLineEdit* m_maintainerEdit;
    QTextEdit* m_descEdit;
    QLabel* m_iconLabel;
    QTreeWidget* m_fileTree;
    QComboBox* m_osCombo;

    void populateFromFolder() {
        m_fileTree->clear();
        QTreeWidgetItem* si = new QTreeWidgetItem({"Syntax (.vxsyn)"});
        QTreeWidgetItem* ii = new QTreeWidgetItem({"Icon Theme"});
        QTreeWidgetItem* qi = new QTreeWidgetItem({"Stylesheet (.qss)"});
        QTreeWidgetItem* pi = new QTreeWidgetItem({"Plugin (.so/.dll/.dylib)"});
        QTreeWidgetItem* ui = new QTreeWidgetItem({"Unknown"});
        ui->setHidden(true);
        m_fileTree->addTopLevelItem(si);
        m_fileTree->addTopLevelItem(ii);
        m_fileTree->addTopLevelItem(qi);
        m_fileTree->addTopLevelItem(pi);
        m_fileTree->addTopLevelItem(ui);
        QDir d(m_folderPath);
        const QStringList es = d.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& e : es) {
            if (e == "plugin.data") continue;

            // Skip image files - they're handled as package icon separately
            QString ext = QFileInfo(e).suffix().toLower();
            if (ext == "png" || ext == "svg" || ext == "ico" || ext == "icns" ||
                ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "gif") {
                continue;
            }

            addFile(m_folderPath + "/" + e);
        }
        m_fileTree->expandAll();
    }

    void addFile(const QString& fp) {
        QFileInfo fi(fp);

        // Skip image files
        QString ext = fi.suffix().toLower();
        if (ext == "png" || ext == "svg" || ext == "ico" || ext == "icns" ||
            ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "gif") {
            return;
        }

        QString type = PackageUtil::ClassifyAssetType(fp, Settings::basePath());
        QTreeWidgetItem* parent = nullptr;
        QString tn;
        if (type == "vex.syntax") { parent = findTL("Syntax"); tn = "Syntax"; }
        else if (type == "vex.theme.qss") { parent = findTL("Stylesheet"); tn = "Stylesheet"; }
        else if (type == "vex.theme.icon") { parent = findTL("Icon Theme"); tn = "Icon Theme"; }
        else if (type == "vex.plugin") { parent = findTL("Plugin"); tn = "Plugin"; }
        else { parent = findTL("Unknown"); parent->setHidden(false); tn = "Unknown"; }
        QTreeWidgetItem* item = new QTreeWidgetItem({tn, fi.fileName()});
        item->setData(0, Qt::UserRole, fp);
        item->setData(0, Qt::UserRole + 1, type);
        parent->addChild(item);
        m_fileTree->expandAll();
    }

    QTreeWidgetItem* findTL(const QString& t) {
        for (int i = 0; i < m_fileTree->topLevelItemCount(); ++i) {
            if (m_fileTree->topLevelItem(i)->text(0).startsWith(t)) return m_fileTree->topLevelItem(i);
        }
        return nullptr;
    }

    void saveDataFile() {
        QString d;
        QTextStream s(&d);
        s << "[vex.metadata]\n"
          << "name=" << m_nameEdit->text().trimmed() << "\n"
          << "version=" << m_versionEdit->text().trimmed() << "\n"
          << "os=" << m_osCombo->currentData().toString() << "\n"
          << "uplugin-version=1.0\n"
          << "maintainer=" << m_maintainerEdit->text().trimmed() << "\n"
          << "description=" << m_descEdit->toPlainText().trimmed().replace('\n', ' ') << "\n";

        // Handle icon - just mark as true if icon is set or images exist in folder
        if (!m_iconPath.isEmpty()) {
            // Copy the selected icon to the package folder if it's not already there
            QFileInfo iconInfo(m_iconPath);
            QString iconName = iconInfo.fileName();
            if (iconInfo.absolutePath() != m_folderPath) {
                QString destIcon = m_folderPath + "/" + iconName;
                if (QFile::exists(destIcon)) QFile::remove(destIcon);
                QFile::copy(m_iconPath, destIcon);
            }
            s << "icon=true\n";
        } else {
            // Check if folder has any image files for auto-detection
            const QStringList exts = {"*.png", "*.svg", "*.ico", "*.icns", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};
            bool hasImage = false;
            for (const QString& ext : exts) {
                if (!QDir(m_folderPath).entryList({ext}, QDir::Files).isEmpty()) {
                    hasImage = true;
                    break;
                }
            }
            if (hasImage) s << "icon=true\n";
        }
        s << "\n";

        QMap<QString, QStringList> g;
        for (int i = 0; i < m_fileTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* t = m_fileTree->topLevelItem(i);
            if (t->text(0).startsWith("Unknown")) continue;
            for (int j = 0; j < t->childCount(); ++j) {
                QTreeWidgetItem* c = t->child(j);
                QString tp = c->data(0, Qt::UserRole + 1).toString();
                if (!tp.isEmpty()) g[tp].append(c->text(1));
            }
        }
        for (auto it = g.constBegin(); it != g.constEnd(); ++it) {
            s << "[" << it.key() << "]\n";
            for (const QString& f : it.value()) {
                s << (it.key() == "vex.theme.icon" ? "folder=" : "file=") << f << "\n";
            }
            s << "\n";
        }

        QFile f(m_folderPath + "/plugin.data");
        if (f.open(QIODevice::WriteOnly)) {
            f.write(d.toUtf8());
            f.close();
        }
    }
};

class ToggleButton : public QPushButton {
    Q_OBJECT
public:
    explicit ToggleButton(const QString& t, QWidget* p = nullptr) : QPushButton(t, p) {
        setCheckable(true);
        setFlat(true);
    }
};

class ExtensionManagerDialog : public QDialog {
    Q_OBJECT
public:
    QListWidget* m_installedList = nullptr;
    QString m_currentPath, m_currentOwner;
    bool m_isPackage = false, m_isPlugin = false;
    QString m_currentFilter = "All";
    QString m_currentSearch;

    explicit ExtensionManagerDialog(UpluginLoader* loader, QWidget* parent = nullptr)
        : QDialog(parent), m_loader(loader) {
        setWindowTitle("Extension Manager");
        setMinimumSize(850, 480);
        setAcceptDrops(true);

        QHBoxLayout* ml = new QHBoxLayout(this);
        ml->setContentsMargins(0, 0, 0, 0);

        QTabWidget* tabWidget = new QTabWidget();
        tabWidget->setTabPosition(QTabWidget::North);

        QWidget* installedTab = new QWidget();
        QVBoxLayout* instLayout = new QVBoxLayout(installedTab);
        instLayout->setContentsMargins(4, 4, 4, 4);
        instLayout->setSpacing(4);

        QHBoxLayout* searchLayout = new QHBoxLayout();
        QLineEdit* searchBox = new QLineEdit();
        searchBox->setPlaceholderText("Search...");
        searchBox->setClearButtonEnabled(true);
        searchLayout->addWidget(searchBox);
        instLayout->addLayout(searchLayout);

        QHBoxLayout* filterLayout = new QHBoxLayout();
        filterLayout->setSpacing(2);
        QButtonGroup* filterGroup = new QButtonGroup(this);
        filterGroup->setExclusive(true);
        const QStringList filterNames = {"All", "Packages", "Plugins", "Stylesheets", "Icons", "Syntax"};
        for (int i = 0; i < filterNames.size(); ++i) {
            ToggleButton* btn = new ToggleButton(filterNames[i]);
            if (i == 0) btn->setChecked(true);
            filterGroup->addButton(btn, i);
            filterLayout->addWidget(btn);
        }
        filterLayout->addStretch();
        instLayout->addLayout(filterLayout);

        m_installedList = new QListWidget();
        m_installedList->setIconSize(QSize(24, 24));
        m_installedList->setAlternatingRowColors(true);
        m_installedList->setContextMenuPolicy(Qt::CustomContextMenu);
        instLayout->addWidget(m_installedList, 1);

        QHBoxLayout* bulkLayout = new QHBoxLayout();
        QPushButton* selectAllBtn = new QPushButton("Select All");
        QPushButton* deselectAllBtn = new QPushButton("Deselect All");
        QPushButton* removeCheckedBtn = new QPushButton("Remove Checked");
        bulkLayout->addWidget(selectAllBtn);
        bulkLayout->addWidget(deselectAllBtn);
        bulkLayout->addWidget(removeCheckedBtn);
        bulkLayout->addStretch();
        instLayout->addLayout(bulkLayout);

        tabWidget->addTab(installedTab, Settings::resolveIcon("package-x-generic"), "Installed");

        QWidget* installTab = new QWidget();
        QVBoxLayout* icTabLayout = new QVBoxLayout(installTab);
        icTabLayout->setContentsMargins(4, 4, 4, 4);

        QPushButton* createBtn = new QPushButton(Settings::resolveIcon("document-new"), "Create .vxpkg");
        createBtn->setMinimumHeight(80);
        QPushButton* installBtn = new QPushButton(Settings::resolveIcon("document-open"), "Install .vxpkg");
        installBtn->setMinimumHeight(80);
        icTabLayout->addWidget(createBtn);
        icTabLayout->addWidget(installBtn);
        icTabLayout->addWidget(new QLabel("Path:"));
        m_urlEdit = new QLineEdit();
        m_urlEdit->setPlaceholderText("Type path or drop here...");
        icTabLayout->addWidget(m_urlEdit);
        icTabLayout->addStretch();

        tabWidget->addTab(installTab, Settings::resolveIcon("document-new"), "Install/Create");

        ml->addWidget(tabWidget, 1);

        m_rightStack = new QStackedWidget();
        m_rightStack->setMinimumWidth(280);

        QWidget* infoPage = new QWidget();
        QVBoxLayout* infoLayout = new QVBoxLayout(infoPage);
        infoLayout->setContentsMargins(12, 12, 12, 12);
        infoLayout->setSpacing(8);

        m_infoIcon = new QLabel();
        m_infoIcon->setFixedSize(128, 128);
        m_infoIcon->setAlignment(Qt::AlignCenter);
        infoLayout->addWidget(m_infoIcon, 0, Qt::AlignCenter);

        m_infoName = new QLabel("Select an item");
        QFont nf = m_infoName->font(); nf.setBold(true); nf.setPointSize(11); m_infoName->setFont(nf);
        m_infoName->setAlignment(Qt::AlignCenter); m_infoName->setWordWrap(true);
        infoLayout->addWidget(m_infoName);

        m_infoDesc = new QLabel();
        m_infoDesc->setWordWrap(true); m_infoDesc->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        QScrollArea* infoScroll = new QScrollArea(); infoScroll->setWidgetResizable(true);
        infoScroll->setWidget(m_infoDesc); infoScroll->setFrameShape(QFrame::NoFrame);
        infoLayout->addWidget(infoScroll, 1);

        QHBoxLayout* infoBtnLayout = new QHBoxLayout(); infoBtnLayout->setSpacing(6);
        m_editBtn = new QPushButton("Edit");
        m_configureBtn = new QPushButton("Configure");
        m_removeBtn = new QPushButton("Remove");
        m_editBtn->setVisible(false); m_configureBtn->setVisible(false); m_removeBtn->setEnabled(false);
        infoBtnLayout->addWidget(m_editBtn); infoBtnLayout->addWidget(m_configureBtn); infoBtnLayout->addWidget(m_removeBtn);
        infoLayout->addLayout(infoBtnLayout);
        m_rightStack->addWidget(infoPage);

        QWidget* icPanel = new QWidget();
        QVBoxLayout* icLayout = new QVBoxLayout(icPanel);
        icLayout->setContentsMargins(12, 12, 12, 12); icLayout->setSpacing(8);

        m_rightIcon = new QLabel(); m_rightIcon->setFixedSize(128, 128); m_rightIcon->setAlignment(Qt::AlignCenter);
        m_rightIcon->setPixmap(Settings::resolveIcon("document-open").pixmap(96, 96));
        icLayout->addWidget(m_rightIcon, 0, Qt::AlignCenter);

        m_rightName = new QLabel("Drop .vxpkg or folder here");
        m_rightName->setAlignment(Qt::AlignCenter); m_rightName->setWordWrap(true);
        QFont rnf = m_rightName->font(); rnf.setBold(true); rnf.setPointSize(11); m_rightName->setFont(rnf);
        icLayout->addWidget(m_rightName);

        m_rightDesc = new QLabel(); m_rightDesc->setWordWrap(true); m_rightDesc->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        QScrollArea* rightScroll = new QScrollArea(); rightScroll->setWidgetResizable(true);
        rightScroll->setWidget(m_rightDesc); rightScroll->setFrameShape(QFrame::NoFrame);
        icLayout->addWidget(rightScroll, 1);

        QHBoxLayout* rightBtnLayout = new QHBoxLayout(); rightBtnLayout->setSpacing(6);
        m_rightActionBtn = new QPushButton("Browse..."); m_rightActionBtn->hide();
        m_rightConfigBtn = new QPushButton(); m_rightConfigBtn->hide();
        rightBtnLayout->addWidget(m_rightActionBtn); rightBtnLayout->addWidget(m_rightConfigBtn);
        icLayout->addLayout(rightBtnLayout);
        m_rightStack->addWidget(icPanel);

        ml->addWidget(m_rightStack);

        QObject::connect(tabWidget, &QTabWidget::currentChanged, [this](int idx) { m_rightStack->setCurrentIndex(idx); });

        auto applyFilter = [this]() {
            for (int i = 0; i < m_installedList->count(); ++i) {
                QListWidgetItem* item = m_installedList->item(i);
                QString path = item->data(Qt::UserRole).toString();
                QString owner = item->data(Qt::UserRole + 1).toString();
                bool isPkg = !owner.isEmpty() && path.endsWith(".conf");
                bool isPlg = path.endsWith(".so") || path.endsWith(".dll") || path.endsWith(".dylib");
                bool isQss = path.endsWith(".qss");
                bool isIcon = path.contains("/themes/icon/") && QFileInfo(path).isDir();
                bool isSyn = path.endsWith(".vxsyn");
                bool catMatch = true;
                if (m_currentFilter == "Packages") catMatch = isPkg;
                else if (m_currentFilter == "Plugins") catMatch = isPlg;
                else if (m_currentFilter == "Stylesheets") catMatch = isQss;
                else if (m_currentFilter == "Icons") catMatch = isIcon;
                else if (m_currentFilter == "Syntax") catMatch = isSyn;
                bool searchMatch = m_currentSearch.isEmpty() || item->text().contains(m_currentSearch, Qt::CaseInsensitive);
                item->setHidden(!(catMatch && searchMatch));
            }
        };

        QObject::connect(searchBox, &QLineEdit::textChanged, [this, applyFilter](const QString& t) { m_currentSearch = t; applyFilter(); });
        QObject::connect(filterGroup, &QButtonGroup::idClicked, [this, applyFilter](int id) { const QStringList fn = {"All","Packages","Plugins","Stylesheets","Icons","Syntax"}; m_currentFilter = fn[id]; applyFilter(); });

        QObject::connect(m_installedList, &QListWidget::customContextMenuRequested, [this](const QPoint& pos) {
            QListWidgetItem* item = m_installedList->itemAt(pos);
            if (!item) return;
            m_installedList->setCurrentItem(item);
            QMenu menu;
            menu.addAction(Settings::resolveIcon("edit-find"), "Edit", [this]() { emit editRequested(m_currentPath); });
            menu.addAction(Settings::resolveIcon("list-remove"), "Remove", [this]() { emit removeRequested(m_currentPath, m_currentOwner); });
            if (m_isPackage || m_isPlugin) menu.addAction(Settings::resolveIcon("preferences-system"), "Configure", [this]() { if (m_isPackage) { DataEditorDialog dlg(QFileInfo(m_currentPath).absolutePath(), this); dlg.exec(); } else emit configurePluginRequested(m_currentPath); });
            menu.exec(m_installedList->viewport()->mapToGlobal(pos));
        });

        QObject::connect(selectAllBtn, &QPushButton::clicked, [this]() { for (int i=0;i<m_installedList->count();++i) if(!m_installedList->item(i)->isHidden()) m_installedList->item(i)->setCheckState(Qt::Checked); });
        QObject::connect(deselectAllBtn, &QPushButton::clicked, [this]() { for (int i=0;i<m_installedList->count();++i) m_installedList->item(i)->setCheckState(Qt::Unchecked); });
        QObject::connect(removeCheckedBtn, &QPushButton::clicked, [this]() { QStringList paths, owners; for (int i=0;i<m_installedList->count();++i) { if(m_installedList->item(i)->checkState()==Qt::Checked&&!m_installedList->item(i)->isHidden()){paths.append(m_installedList->item(i)->data(Qt::UserRole).toString());owners.append(m_installedList->item(i)->data(Qt::UserRole+1).toString());}} if(!paths.isEmpty())emit removeRequestedBatch(paths,owners); });

        QObject::connect(createBtn, &QPushButton::clicked, [this]() { QString p = QFileDialog::getExistingDirectory(this,"Select Folder",Settings::basePath()); if(!p.isEmpty())m_urlEdit->setText(p); });
        QObject::connect(installBtn, &QPushButton::clicked, [this]() { QString p = QFileDialog::getOpenFileName(this,"Select Package",Settings::basePath(),"Vex Package (*.vxpkg)"); if(!p.isEmpty())m_urlEdit->setText(p); });
        QObject::connect(m_urlEdit, &QLineEdit::textChanged, [this](const QString& t) { QFileInfo fi(t); if(fi.isFile()&&t.endsWith(".vxpkg"))showInstallMeta(t); else if(fi.isDir())showCreateMeta(t); else{m_rightIcon->setPixmap(Settings::resolveIcon("document-open").pixmap(96,96));m_rightName->setText("Drop .vxpkg or folder here");m_rightDesc->clear();m_rightActionBtn->hide();m_rightConfigBtn->hide();} });
        QObject::connect(m_rightActionBtn, &QPushButton::clicked, [this]() { QString p=m_urlEdit->text().trimmed(); if(p.isEmpty()){p=QFileDialog::getOpenFileName(this,"Select Package",Settings::basePath(),"Vex Package (*.vxpkg)");if(!p.isEmpty())m_urlEdit->setText(p);}else if(p.endsWith(".vxpkg"))emit installRequested(p);else if(QFileInfo(p).isDir())emit createRequested(p); });
        QObject::connect(m_rightConfigBtn, &QPushButton::clicked, [this]() { QString p=m_urlEdit->text().trimmed(); if(!p.isEmpty()&&QFileInfo(p).isDir()){DataEditorDialog dlg(p,this);dlg.exec();showCreateMeta(p);} });

        QObject::connect(m_installedList, &QListWidget::currentItemChanged, [this](QListWidgetItem* cur, QListWidgetItem*) {
            if (!cur || cur->isHidden()) { m_infoIcon->clear(); m_infoName->setText("Select an item"); m_infoDesc->clear(); m_editBtn->setVisible(false); m_configureBtn->setVisible(false); m_removeBtn->setEnabled(false); return; }
            QString path = cur->data(Qt::UserRole).toString(), owner = cur->data(Qt::UserRole + 1).toString();
            bool isPkg = !owner.isEmpty() && path.endsWith(".conf"), isPlg = path.endsWith(".so") || path.endsWith(".dll") || path.endsWith(".dylib");
            m_currentPath = path; m_currentOwner = owner; m_isPackage = isPkg; m_isPlugin = isPlg;

            if (isPkg && QFile::exists(path)) {
                QSettings conf(path, QSettings::IniFormat);
                QString iconVal = conf.value("vex.metadata/icon").toString();
                QIcon pkgIcon = PackageUtil::DeriveGenericIcon(iconVal, QFileInfo(path).absolutePath());
                m_infoIcon->setPixmap(pkgIcon.pixmap(96, 96));
                m_infoName->setText(conf.value("vex.metadata/name").toString());
                QString desc = conf.value("vex.metadata/version").toString() + "\n" +
                               conf.value("vex.metadata/maintainer").toString() + "\n" +
                               "OS: " + conf.value("vex.metadata/os", "any").toString().toUpper() + "\n\n" +
                               conf.value("vex.metadata/description").toString();
                m_infoDesc->setText(desc);
                m_editBtn->setVisible(false); m_configureBtn->setVisible(true); m_removeBtn->setEnabled(true);
            } else if (isPlg) {
                QPluginLoader ldr(path); QObject* inst = ldr.instance(); Uplugin* up = inst ? qobject_cast<Uplugin*>(inst) : nullptr;
                QString desc;
                if (up) { UpluginMetadata meta = up->metadata(); m_infoIcon->setPixmap(meta.icon.pixmap(96, 96)); m_infoName->setText(meta.name); desc = meta.version + "\n" + meta.maintainer + "\n\n" + meta.description; }
                else { QFileIconProvider ip; m_infoIcon->setPixmap(ip.icon(QFileInfo(path)).pixmap(96, 96)); m_infoName->setText(QFileInfo(path).fileName()); desc = path; }
                ldr.unload();
                if (!owner.isEmpty()) desc += "\n\nPackage: " + owner;
                m_infoDesc->setText(desc);
                m_editBtn->setVisible(false); m_configureBtn->setVisible(true); m_removeBtn->setEnabled(true);
            } else {
                QIcon ic;
                QString desc = path;
                if (QFileInfo(path).isDir()) {
                    ic = PackageUtil::DeriveGenericIcon("", path, true);
                } else {
                    QFileIconProvider ip;
                    ic = ip.icon(QFileInfo(path));
                }
                m_infoIcon->setPixmap(ic.pixmap(96, 96));
                m_infoName->setText(QFileInfo(path).fileName());
                if (!owner.isEmpty()) desc += "\n\nPackage: " + owner;
                m_infoDesc->setText(desc);
                bool ce = path.endsWith(".vxsyn") || path.endsWith(".qss");
                m_editBtn->setVisible(ce); m_configureBtn->setVisible(false); m_removeBtn->setEnabled(true);
            }
        });

        QObject::connect(m_editBtn, &QPushButton::clicked, [this]() { emit editRequested(m_currentPath); });
        QObject::connect(m_configureBtn, &QPushButton::clicked, [this]() { if (m_isPackage) { DataEditorDialog dlg(QFileInfo(m_currentPath).absolutePath(), this); dlg.exec(); } else if (m_isPlugin) emit configurePluginRequested(m_currentPath); });
        QObject::connect(m_removeBtn, &QPushButton::clicked, [this]() { emit removeRequested(m_currentPath, m_currentOwner); });

        refreshInstalled();
    }

    void showInstallMeta(const QString& path) {
        QTemporaryDir tmp;
        if (!tmp.isValid()) return;
        QString sz = PackageUtil::FindArchiver();
        if (sz.isEmpty()) return;

        QProcess p;
        p.start(sz, QStringList() << "x" << path << "-o" + tmp.path() << "-y" << "plugin.data");
        p.waitForFinished();

        QString dp = tmp.path() + "/plugin.data";
        if (QFile::exists(dp)) {
            QSettings pk(dp, QSettings::IniFormat);
            QString name = pk.value("vex.metadata/name").toString();
            QString iconVal = pk.value("vex.metadata/icon").toString();
            QString osTarget = pk.value("vex.metadata/os", "any").toString();
            QString version = pk.value("vex.metadata/version", "unknown").toString();

            // Extract icons from package for preview
            QIcon icon;
            if (iconVal == "true" || (!iconVal.isEmpty() && iconVal != "true")) {
                QProcess px;
                px.start(sz, QStringList() << "x" << path << "-o" + tmp.path() << "-y" << "*.png" << "*.svg" << "*.ico" << "*.icns" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.gif");
                px.waitForFinished();
                icon = PackageUtil::DeriveGenericIcon(iconVal, tmp.path());
            } else {
                icon = Settings::resolveIcon("package-x-generic");
            }

            if (name.isEmpty()) {
                m_rightIcon->setPixmap(icon.pixmap(96,96));
                m_rightName->setText(QFileInfo(path).fileName());
                m_rightDesc->setText("Invalid metadata");
                m_rightActionBtn->hide();
                m_rightConfigBtn->hide();
            } else {
                m_rightIcon->setPixmap(icon.pixmap(96,96));
                m_rightName->setText(name);
                m_rightDesc->setText("Version: " + version + "\n" +
                                     "Target OS: " + osTarget.toUpper() + "\n" +
                                     "Maintainer: " + pk.value("vex.metadata/maintainer").toString() + "\n\n" +
                                     pk.value("vex.metadata/description").toString());
                m_rightActionBtn->setText("Install");
                m_rightActionBtn->show();
                m_rightConfigBtn->hide();
            }
        } else {
            m_rightIcon->setPixmap(Settings::resolveIcon("package-x-generic").pixmap(96,96));
            m_rightName->setText(QFileInfo(path).fileName());
            m_rightDesc->setText("No package.data found");
            m_rightActionBtn->hide();
            m_rightConfigBtn->hide();
        }
    }

    void showCreateMeta(const QString& path) {
        bool hd = QFile::exists(path + "/plugin.data");
        if (hd) {
            QSettings pk(path + "/plugin.data", QSettings::IniFormat);
            QString name = pk.value("vex.metadata/name").toString();
            QString iconVal = pk.value("vex.metadata/icon").toString();
            QString osTarget = pk.value("vex.metadata/os", "any").toString();
            QIcon icon = PackageUtil::DeriveGenericIcon(iconVal, path);
            if (name.isEmpty()) {
                m_rightIcon->setPixmap(icon.pixmap(96,96));
                m_rightName->setText(QDir(path).dirName());
                m_rightDesc->setText("No name field");
                m_rightActionBtn->hide();
                m_rightConfigBtn->setText("Configure data...");
                m_rightConfigBtn->show();
            } else {
                m_rightIcon->setPixmap(icon.pixmap(96,96));
                m_rightName->setText(name);
                m_rightDesc->setText("Version: " + pk.value("vex.metadata/version").toString() + "\n" +
                                     "Target OS: " + osTarget.toUpper() + "\n" +
                                     "Maintainer: " + pk.value("vex.metadata/maintainer").toString() + "\n\n" +
                                     pk.value("vex.metadata/description").toString());
                m_rightActionBtn->setText("Create Package");
                m_rightActionBtn->show();
                m_rightConfigBtn->setText("Configure data...");
                m_rightConfigBtn->show();
            }
        } else {
            m_rightIcon->setPixmap(Settings::resolveIcon("folder").pixmap(96,96));
            m_rightName->setText(QDir(path).dirName());
            m_rightDesc->setText("No package.data found.\nAdd metadata to make it a package.");
            m_rightActionBtn->hide();
            m_rightConfigBtn->setText("Add metadata...");
            m_rightConfigBtn->show();
        }
    }

    void refreshInstalled() {
        if (!m_installedList) return;
        m_installedList->clear();
        QString bp = Settings::basePath(), ap = bp + "/plugins/assets";
        QFileIconProvider ip;

        auto add = [&](const QIcon& ic, const QString& tx, const QString& p, const QString& ow = QString()) {
            QListWidgetItem* i = new QListWidgetItem(ic, tx);
            i->setFlags(i->flags() | Qt::ItemIsUserCheckable);
            i->setCheckState(Qt::Unchecked);
            i->setData(Qt::UserRole, p); i->setData(Qt::UserRole + 1, ow); i->setToolTip(p);
            m_installedList->addItem(i);
        };

        QDir ad(ap);
        if (ad.exists()) {
            const QStringList pk = ad.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& pkg : pk) {
                QString cp = ap + "/" + pkg + "/data.conf";
                if (!QFile::exists(cp)) continue;
                QSettings c(cp, QSettings::IniFormat);
                QString iconVal = c.value("vex.metadata/icon").toString();
                QIcon icon = PackageUtil::DeriveGenericIcon(iconVal, ap + "/" + pkg);
                add(icon, c.value("vex.metadata/name").toString() + " (" + c.value("vex.metadata/version").toString() + ")", cp, pkg);
            }
        }

        auto scan = [&](const QString& dp, const QString& f, QDir::Filter xf = QDir::Files) {
            QDir d(dp); if (!d.exists()) return;
            QStringList fl; if (xf == QDir::Dirs) fl = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot); else fl = d.entryList({f}, QDir::Files);
            for (const QString& fn : std::as_const(fl)) {
                QString fp = dp + "/" + fn, ow = PackageUtil::ResolvePackageOwner(fp, ap);
                QIcon ic; QFileInfo fi(fp);
                if (fn.endsWith(".so") || fn.endsWith(".dll") || fn.endsWith(".dylib")) {
                    QPluginLoader l(fp); QObject* io = l.instance(); Uplugin* up = io ? qobject_cast<Uplugin*>(io) : nullptr;
                    ic = up ? up->metadata().icon : ip.icon(fi); if (ic.isNull()) ic = ip.icon(fi); l.unload();
                } else if (xf == QDir::Dirs) {
                    ic = PackageUtil::DeriveGenericIcon("", fp, true);
                } else {
                    ic = ip.icon(fi);
                }
                if (ic.isNull()) ic = Settings::resolveIcon("application-x-addon");
                add(ic, fn, fp, ow);
            }
        };

        scan(bp + "/plugins", "*.so");
#ifdef Q_OS_WIN
        scan(bp + "/plugins", "*.dll");
#elif defined(Q_OS_MAC)
        scan(bp + "/plugins", "*.dylib");
#endif
        scan(bp + "/themes/stylesheets", "*.qss");
        scan(bp + "/themes/icon", "*", QDir::Dirs);
        scan(bp + "/syntax", "*.vxsyn");
    }

    QLineEdit* urlEdit() { return m_urlEdit; }

signals:
    void installRequested(const QString&);
    void createRequested(const QString&);
    void removeRequested(const QString& path, const QString& owner);
    void removeRequestedBatch(const QStringList& paths, const QStringList& owners);
    void editRequested(const QString&);
    void configurePluginRequested(const QString&);

protected:
    void dragEnterEvent(QDragEnterEvent* e) override { if (e->mimeData()->hasUrls()) e->acceptProposedAction(); }
    void dropEvent(QDropEvent* e) override { const QList<QUrl> urls = e->mimeData()->urls(); for (const QUrl& u : urls) m_urlEdit->setText(u.toLocalFile()); }

private:
    UpluginLoader* m_loader;
    QLineEdit* m_urlEdit;
    QStackedWidget* m_rightStack;
    QLabel* m_infoIcon, *m_infoName, *m_infoDesc;
    QPushButton* m_editBtn, *m_configureBtn, *m_removeBtn;
    QLabel* m_rightIcon, *m_rightName, *m_rightDesc;
    QPushButton* m_rightActionBtn, *m_rightConfigBtn;
};

class UpluginCorePlugin : public QObject, public CorePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "vex.core/4.0")
    Q_INTERFACES(CorePlugin)
public:
    PluginMetadata meta() const override { PluginMetadata m; m.importance = PluginMetadata::Xylem; return m; }
    QMenu* m_extMenu = nullptr;

    bool initialize(MainWindow* w, Settings* s, CmdLine& cmdLine) override {
        Q_UNUSED(s)
        m_mainWindow = reinterpret_cast<QMainWindow*>(w);
        m_bridge = new UpluginBridge(m_mainWindow);
        cmdLine.addCommand({{"i","install"},"Install a .vxpkg package","package"});
        cmdLine.addCommand({{"c","create"},"Create a .vxpkg package","folder"});
        cmdLine.addCommand({{"l","list"},"List installed packages",""});
        cmdLine.addCommand({{"r","remove"},"Remove a package by name","name"});
        QString pp = Settings::basePath()+"/plugins";
        QDir().mkpath(pp);
        m_loader.ProbePluginDirectory(pp);
        setupMenubar();
        bool sk = Settings::instance().get<bool>("uplugin/skipTrustCheck",false);
        if(!sk){m_loader.PresentUntrustedVerificationDialog(m_mainWindow);}
        else{const QList<PluginInfo> all=m_loader.plugins;for(const PluginInfo& p:all)m_loader.SetPluginTrustLevel(p.filePath,true);}
        m_loader.ActivateTrustedPlugins(m_bridge, m_extMenu);
        m_mainWindow->statusBar()->showMessage("Uplugin loaded",2000);
        return true;
    }

    void afterParse(CmdLine& cmdLine) override {
        if(cmdLine.isSet("install")){DeployPackage(cmdLine.value("install"));QCoreApplication::quit();}
        if(cmdLine.isSet("create")){AssemblePackage(cmdLine.value("create"));QCoreApplication::quit();}
        if(cmdLine.isSet("list")){EnumeratePackages();QCoreApplication::quit();}
        if(cmdLine.isSet("remove")){PurgePackage(cmdLine.value("remove"));QCoreApplication::quit();}
    }

private:
    QMainWindow* m_mainWindow;
    UpluginBridge* m_bridge;
    UpluginLoader m_loader;

    void setupMenubar() {
        QMenu* fileMenu = nullptr;
        const QList<QAction*> acts = m_mainWindow->menuBar()->actions();
        for (QAction* a : acts) {
            if (a->text() == "&File" || a->text() == "File") { fileMenu = a->menu(); break; }
        }
        if (!fileMenu) fileMenu = m_mainWindow->menuBar()->addMenu("&File");

        m_extMenu = fileMenu->addMenu(Settings::resolveIcon("application-x-addon"), "E&xtensions");
        m_extMenu->addAction(Settings::resolveIcon("application-x-addon"),"Extension &Manager...",[this](){LaunchExtensionManager();});
        m_extMenu->addAction(Settings::resolveIcon("application-x-addon"),"&Plugin Manager...",[this](){PluginMan::LaunchPluginManager(&m_loader,m_bridge,QString(),m_mainWindow);});
        m_extMenu->addSeparator();
        m_extMenu->addAction(Settings::resolveIcon("view-refresh"),"&Refresh Plugins",[this](){m_loader.UnloadAllPlugins();m_loader.ActivateTrustedPlugins(m_bridge,m_extMenu);});
    }

    void LaunchExtensionManager() {
        ExtensionManagerDialog dlg(&m_loader,m_mainWindow);
        QObject::connect(&dlg,&ExtensionManagerDialog::installRequested,[this,&dlg](const QString& p){DeployPackage(p);dlg.urlEdit()->clear();dlg.refreshInstalled();});
        QObject::connect(&dlg,&ExtensionManagerDialog::createRequested,[this,&dlg](const QString& p){AssemblePackage(p);dlg.urlEdit()->clear();dlg.refreshInstalled();});
        QObject::connect(&dlg,&ExtensionManagerDialog::removeRequested,[this,&dlg](const QString& path,const QString& owner){ProcessAssetRemoval(path,owner,&dlg);});
        QObject::connect(&dlg,&ExtensionManagerDialog::removeRequestedBatch,[this,&dlg](const QStringList& paths,const QStringList& owners){for(int i=0;i<paths.size();++i)ProcessAssetRemoval(paths[i],owners[i],&dlg);dlg.refreshInstalled();});
        QObject::connect(&dlg,&ExtensionManagerDialog::editRequested,[](const QString& p){QString td=Settings::basePath()+"/.temp";QDir().mkpath(td);QString rf=td+"/"+QString::number(QCoreApplication::applicationPid())+".Req";QFile f(rf);if(f.open(QIODevice::WriteOnly|QIODevice::Append)){QTextStream o(&f);o.setEncoding(QStringConverter::Utf8);o<<p<<"\n";f.close();}});
        QObject::connect(&dlg,&ExtensionManagerDialog::configurePluginRequested,[this](const QString& p){PluginMan::LaunchPluginManager(&m_loader,m_bridge,QFileInfo(p).baseName(),m_mainWindow);});
        dlg.exec();
    }

    void ProcessAssetRemoval(const QString& path, const QString& owner, ExtensionManagerDialog* dlg) {
        if (path.isEmpty()) return;

        QFileInfo fi(path);

        if (!owner.isEmpty()) {
            auto r = QMessageBox::question(dlg, "Package Detected",
                                           QString("'%1'\n\nBelongs to package '%2'.\n\nRemove just this file (No) or entire package (Yes)?")
                                               .arg(fi.fileName()).arg(owner),
                                           QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (r == QMessageBox::Cancel) return;

            QString confPath = Settings::basePath() + "/plugins/assets/" + owner + "/data.conf";
            if (r == QMessageBox::No) {
                PackageUtil::StripConfigEntry(confPath, fi.fileName());
                if (QFile::exists(path)) {
                    if (fi.isDir()) {
                        QDir(path).removeRecursively();
                    } else {
                        QFile::remove(path);
                    }
                }
            } else {
                QSettings conf(confPath, QSettings::IniFormat);
                QStringList groups = {"vex.plugin", "vex.theme.qss", "vex.theme.icon", "vex.syntax"};
                for (const QString& g : groups) {
                    conf.beginGroup(g);
                    const QStringList keys = conf.childKeys();
                    for (const QString& key : keys) {
                        QString filePath = (g == "vex.plugin" ? Settings::basePath() + "/plugins/" :
                                                g == "vex.theme.qss" ? Settings::basePath() + "/themes/stylesheets/" :
                                                g == "vex.theme.icon" ? Settings::basePath() + "/themes/icon/" :
                                                Settings::basePath() + "/syntax/") + conf.value(key).toString();
                        if (QFile::exists(filePath)) {
                            QFileInfo fi2(filePath);
                            if (fi2.isDir()) QDir(filePath).removeRecursively();
                            else QFile::remove(filePath);
                        }
                    }
                    conf.endGroup();
                }
                QDir(Settings::basePath() + "/plugins/assets/" + owner).removeRecursively();
            }
        } else {
            if (!QFile::exists(path)) {
                QMessageBox::information(dlg, "Not Found", "File not found, skipping.");
            } else {
                if (fi.isDir()) {
                    QDir(path).removeRecursively();
                } else {
                    if (path.endsWith(".so") || path.endsWith(".dll") || path.endsWith(".dylib")) {
                        m_loader.UnloadSinglePlugin(path);
                    }
                    QFile::remove(path);
                }
            }
        }
        dlg->refreshInstalled();
    }

    void AssemblePackage(const QString& fp) {
        QDir d(fp);
        if(!d.exists())return;
        QString dataPath=fp+"/plugin.data";
        if(!QFile::exists(dataPath)){QMessageBox::warning(m_mainWindow,"Error","No plugin.data found");return;}

        QSettings pk(dataPath,QSettings::IniFormat);
        QString iconVal=pk.value("vex.metadata/icon").toString();
        QStringList imageExts={"png","svg","ico","icns","jpg","jpeg","bmp","gif"};

        if(iconVal.isEmpty()||iconVal=="true"){
            for(const QString& ext:imageExts){
                if(!d.entryList({"*."+ext},QDir::Files).isEmpty()){
                    pk.setValue("vex.metadata/icon","true");
                    break;
                }
            }
        } else if(iconVal.contains('.')){
            QString fn=QFileInfo(iconVal).fileName();
            if(QFile::exists(fp+"/"+fn)){
                pk.setValue("vex.metadata/icon","true");
            }else{
                bool found=false;
                for(const QString& ext:imageExts){
                    if(!d.entryList({"*."+ext},QDir::Files).isEmpty()){
                        pk.setValue("vex.metadata/icon","true");
                        found=true;
                        break;
                    }
                }
                if(!found)pk.remove("vex.metadata/icon");
            }
        }

        // Set uplugin-version if not already set
        if(!pk.contains("vex.metadata/uplugin-version")) {
            pk.setValue("vex.metadata/uplugin-version", "1.0");
        }

        // Set OS if not already set
        if(!pk.contains("vex.metadata/os")) {
            pk.setValue("vex.metadata/os", "any");
        }

        pk.sync();

        QString sz=PackageUtil::FindArchiver();
        if(sz.isEmpty()){PackageUtil::AlertArchiverMissing(m_mainWindow);return;}
        QString o=QFileDialog::getSaveFileName(m_mainWindow,"Save Package",QDir::homePath()+"/"+d.dirName()+".vxpkg","Vex Package (*.vxpkg)");
        if(o.isEmpty())return;
        if(QFile::exists(o))QFile::remove(o);
        QProcess c;c.setWorkingDirectory(fp);c.start(sz,QStringList()<<"a"<<"-tzip"<<o<<".");c.waitForFinished();
    }

    void DeployPackage(const QString& p) {
        if(!QFile::exists(p)) return;

        QString sz = PackageUtil::FindArchiver();
        if(sz.isEmpty()) {
            PackageUtil::AlertArchiverMissing(m_mainWindow);
            return;
        }

        QTemporaryDir t;
        if(!t.isValid()) return;

        QProcess pr;
        pr.start(sz, QStringList() << "x" << p << "-o" + t.path() << "-y");
        pr.waitForFinished();
        if(pr.exitCode() != 0) return;

        QString dp = t.path() + "/plugin.data";
        if(!QFile::exists(dp)) return;

        QSettings pk(dp, QSettings::IniFormat);
        QString n = pk.value("vex.metadata/name").toString();
        QString ic = pk.value("vex.metadata/icon").toString();

        // Check OS compatibility
        QString targetOS = pk.value("vex.metadata/os", "any").toString().toLower();
        QString currentOS = PackageUtil::GetCurrentOS();

        if(targetOS != "any" && targetOS != currentOS) {
            QMessageBox::warning(m_mainWindow, "OS Mismatch",
                                 QString("Package '%1' is built for %2, but you're running %3.\n\n"
                                         "Installation cancelled to prevent compatibility issues.")
                                     .arg(n)
                                     .arg(targetOS.toUpper())
                                     .arg(currentOS.toUpper()));
            return;
        }

        // Warn about plugin compatibility
        if(targetOS == "any" && pk.contains("vex.plugin/file")) {
            auto r = QMessageBox::warning(m_mainWindow, "OS-Independent Package",
                                          QString("Package '%1' claims to be OS-independent but contains plugins.\n\n"
                                                  "This may cause crashes if plugins aren't compiled for your OS.\n\n"
                                                  "Continue anyway?")
                                              .arg(n),
                                          QMessageBox::Yes | QMessageBox::No);
            if(r == QMessageBox::No) return;
        }

        DeployAssetGroup(pk, "vex.plugin", t.path(), Settings::basePath() + "/plugins/", false);
        DeployAssetGroup(pk, "vex.theme.qss", t.path(), Settings::basePath() + "/themes/stylesheets/", false);
        DeployAssetGroup(pk, "vex.theme.icon", t.path(), Settings::basePath() + "/themes/icon/", true);
        DeployAssetGroup(pk, "vex.syntax", t.path(), Settings::basePath() + "/syntax/", false);

        if(ic == "true") {
            ic = QDir(t.path()).entryList({"*.png", "*.svg", "*.ico", "*.icns", "*.jpg", "*.jpeg", "*.bmp", "*.gif"}, QDir::Files).value(0);
        }
        if(!ic.isEmpty() && ic != "true") {
            QString d = Settings::basePath() + "/plugins/assets/" + n;
            QDir().mkpath(d);
            QString src = t.path() + "/" + ic;
            if(QFile::exists(src)) {
                if(QFile::exists(d + "/" + ic)) QFile::remove(d + "/" + ic);
                QFile::copy(src, d + "/" + ic);
            }
        }

        QString ad = Settings::basePath() + "/plugins/assets/" + n;
        QDir().mkpath(ad);

        // Read the original plugin.data and strip unwanted fields
        QFile mf(dp);
        if(mf.open(QIODevice::ReadOnly)) {
            QString content = mf.readAll();
            mf.close();

            // Remove uplugin-version and os from vex.metadata section
            QStringList lines = content.split('\n');
            QStringList filteredLines;
            bool inMetadata = false;

            for(const QString& line : lines) {
                QString trimmed = line.trimmed();

                if(trimmed.startsWith("[vex.metadata]")) {
                    inMetadata = true;
                    filteredLines.append(line);
                    continue;
                }

                if(trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    inMetadata = false;
                    filteredLines.append(line);
                    continue;
                }

                if(inMetadata) {
                    if(trimmed.startsWith("uplugin-version=") || trimmed.startsWith("os=")) {
                        continue;  // Skip these fields
                    }
                }

                filteredLines.append(line);
            }

            // Write cleaned data.conf
            QFile cf(ad + "/data.conf");
            if(cf.open(QIODevice::WriteOnly)) {
                QTextStream out(&cf);
                out.setEncoding(QStringConverter::Utf8);
                out << filteredLines.join('\n');
                cf.close();
            }
        }

        // Refresh plugin loader for newly installed plugins
        QString pp = Settings::basePath() + "/plugins";
        m_loader.ProbePluginDirectory(pp);
        m_loader.ActivateTrustedPlugins(m_bridge, m_extMenu);
    }

    void DeployAssetGroup(QSettings& pk, const QString& g, const QString& sd, const QString& dd, bool isF) {
        pk.beginGroup(g);
        const QStringList k = pk.childKeys();
        for(const QString& key : k) {
            QString n = pk.value(key).toString();
            QString dst = dd + n;
            QDir().mkpath(dd);

            if(QFile::exists(dst)) {
                auto r = QMessageBox::question(m_mainWindow, "File Exists",
                                               QString("'%1' already exists.\nSkip or replace?").arg(n),
                                               QMessageBox::Yes | QMessageBox::No);
                if(r == QMessageBox::Yes) continue;
                if(isF) QDir(dst).removeRecursively();
                else QFile::remove(dst);
            }

            QString src = sd + "/" + n;
            if(isF) {
                if(QDir(src).exists()) {
                    PackageUtil::CopyDirectory(src, dst);
                }
            } else {
                if(QFile::exists(src)) {
                    QFile::copy(src, dst);
                }
            }
        }
        pk.endGroup();
    }

    void EnumeratePackages() {
        QString ap = Settings::basePath() + "/plugins/assets";
        QDir d(ap);
        if(!d.exists()) return;
        const QStringList pkgs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for(const QString& p : pkgs) {
            QString cp = ap + "/" + p + "/data.conf";
            if(QFile::exists(cp)) {
                QSettings c(cp, QSettings::IniFormat);
                qDebug().noquote() << c.value("vex.metadata/name").toString()
                                   << "(" << c.value("vex.metadata/version").toString() << ")"
                                   << "- OS:" << c.value("vex.metadata/os", "any").toString();
            }
        }
    }

    void PurgePackage(const QString& name) {
        QString ap = Settings::basePath() + "/plugins/assets";
        QDir d(ap);
        if(!d.exists()) return;
        const QStringList pkgs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for(const QString& p : pkgs) {
            QString cp = ap + "/" + p + "/data.conf";
            if(QFile::exists(cp)) {
                QSettings c(cp, QSettings::IniFormat);
                if(c.value("vex.metadata/name").toString() == name) {
                    QStringList groups = {"vex.plugin", "vex.theme.qss", "vex.theme.icon", "vex.syntax"};
                    for(const QString& g : groups) {
                        c.beginGroup(g);
                        const QStringList k = c.childKeys();
                        for(const QString& key : k) {
                            QString f = (g == "vex.plugin" ? Settings::basePath() + "/plugins/" :
                                             g == "vex.theme.qss" ? Settings::basePath() + "/themes/stylesheets/" :
                                             g == "vex.theme.icon" ? Settings::basePath() + "/themes/icon/" :
                                             Settings::basePath() + "/syntax/") + c.value(key).toString();
                            if(QFile::exists(f)) {
                                QFileInfo fi(f);
                                if(fi.isDir()) QDir(f).removeRecursively();
                                else QFile::remove(f);
                            }
                        }
                        c.endGroup();
                    }
                    QDir(ap + "/" + p).removeRecursively();
                    return;
                }
            }
        }
    }
};

#include "Uplugin.moc"
