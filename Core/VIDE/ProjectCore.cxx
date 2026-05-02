/****************************************************************
*                         Apache 2.0                           *
*     Copyright Zynomon aelius <zynomon@proton.me>  2026       *
*               Project         :        Vex                   *
*               Version         :        4.3 (Cytoplasm)       *
****************************************************************/

#include <QObject>
#include <QtPlugin>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QComboBox>
#include <QStackedWidget>
#include <QSettings>
#include <QDesktopServices>
#include <QApplication>
#include <QFileIconProvider>
#include <QKeyEvent>
#include <QPixmap>
#include <QUrl>
#include <QClipboard>
#include <QShortcut>
#include <QInputDialog>
#include "Uplugin.H"

struct ClipEntry { QString path; bool cut; };
static QList<ClipEntry> s_clipboard;

static QIcon ResolveProjectIcon(const QString& iconVal, const QString& projectDir) {
    if (iconVal.isEmpty()) return QFileIconProvider().icon(QFileIconProvider::Folder);
    QString fullPath = QFileInfo(iconVal).isRelative() ? projectDir + "/" + iconVal : iconVal;
    if (QFile::exists(fullPath)) return QFileIconProvider().icon(QFileInfo(fullPath));
    return QFileIconProvider().icon(QFileIconProvider::Folder);
}

class PathBar : public QWidget {
    Q_OBJECT
public:
    explicit PathBar(QWidget* parent = nullptr) : QWidget(parent) {
        QHBoxLayout* l = new QHBoxLayout(this);
        l->setContentsMargins(2, 2, 2, 2);
        m_edit = new QLineEdit();
        m_edit->setPlaceholderText("Path...");
        l->addWidget(m_edit);
        connect(m_edit, &QLineEdit::returnPressed, this, &PathBar::pathSubmitted);
    }
    void setPath(const QString& p) { m_edit->setText(QDir::toNativeSeparators(p)); }
    QString path() const { return m_edit->text(); }
signals:
    void pathSubmitted();
private:
    QLineEdit* m_edit;
};

class ProjectBar : public QWidget {
    Q_OBJECT
public:
    explicit ProjectBar(QWidget* parent = nullptr) : QWidget(parent) {
        QHBoxLayout* l = new QHBoxLayout(this);
        l->setContentsMargins(2, 2, 2, 2);
        m_combo = new QComboBox();
        m_combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        l->addWidget(m_combo);
        connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProjectBar::projectSelected);
    }
    void setProjects(const QStringList& paths, const QString& current) {
        m_combo->blockSignals(true);
        m_combo->clear();
        m_combo->addItem("NONE");
        for (const QString& p : std::as_const(paths))
            m_combo->addItem(QDir(p).dirName(), p);
        if (!current.isEmpty()) {
            int idx = paths.indexOf(current);
            m_combo->setCurrentIndex(idx >= 0 ? idx + 1 : 0);
        } else {
            m_combo->setCurrentIndex(0);
        }
        m_combo->blockSignals(false);
    }
    bool isNone() const { return m_combo->currentIndex() == 0; }
    QString currentPath() const { return isNone() ? QString() : m_combo->currentData().toString(); }
signals:
    void projectSelected(int index);
private:
    QComboBox* m_combo;
};

class ProjectTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit ProjectTreeView(QWidget* parent = nullptr) : QTreeView(parent) {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QTreeView::customContextMenuRequested, this, &ProjectTreeView::onContextMenu);
    }
    void setFSModel(QFileSystemModel* m) { m_fsModel = m; }
    QString pathFromIndex(const QModelIndex& idx) {
        if (!idx.isValid()) return QString();
        QAbstractItemModel* m = model();
        if (m == m_fsModel && m_fsModel) return m_fsModel->filePath(idx);
        return m->data(idx, Qt::UserRole).toString();
    }
    bool isFileFromIndex(const QModelIndex& idx) {
        if (!idx.isValid()) return false;
        QAbstractItemModel* m = model();
        if (m == m_fsModel && m_fsModel) return QFileInfo(m_fsModel->filePath(idx)).isFile();
        return m->data(idx, Qt::UserRole + 1).toBool();
    }
signals:
    void contextMenu(const QPoint& pos, const QString& path, bool isFile);
    void middleClick(const QString& path);
protected:
    void keyPressEvent(QKeyEvent* e) override {
        if (e->key() == Qt::Key_F2) { QModelIndex idx = currentIndex(); if (idx.isValid()) edit(idx); return; }
        QTreeView::keyPressEvent(e);
    }
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::MiddleButton) {
            QModelIndex idx = indexAt(e->pos());
            if (idx.isValid()) { QString p = pathFromIndex(idx); if (!p.isEmpty()) emit middleClick(p); return; }
        }
        QTreeView::mousePressEvent(e);
    }
private:
    QFileSystemModel* m_fsModel = nullptr;
    void onContextMenu(const QPoint& pos) {
        QModelIndex idx = indexAt(pos);
        QString path; bool isFile = false;
        if (idx.isValid()) { path = pathFromIndex(idx); isFile = isFileFromIndex(idx); }
        emit contextMenu(pos, path, isFile);
    }
};

class ProjectTreeModel : public QStandardItemModel {
public:
    explicit ProjectTreeModel(QObject* parent = nullptr) : QStandardItemModel(parent) { setHorizontalHeaderLabels({"Name"}); }

    void LoadFromProx(const QString& proxPath) {
        clear();
        QSettings prox(proxPath, QSettings::IniFormat);
        QFileIconProvider ip;
        QString rootDir = QFileInfo(proxPath).absolutePath();
        QString projName = prox.value("project/name", QDir(rootDir).dirName()).toString();
        QIcon projIcon = ResolveProjectIcon(prox.value("project/icon", "").toString(), rootDir);

        QStandardItem* root = new QStandardItem(projIcon, projName);
        root->setData(rootDir, Qt::UserRole);
        root->setData(proxPath, Qt::UserRole + 1);
        root->setData(false, Qt::UserRole + 2);
        root->setData(true, Qt::UserRole + 3);
        invisibleRootItem()->appendRow(root);

        prox.beginGroup("files");
        const QStringList fkeys = prox.childKeys();
        for (const QString& k : std::as_const(fkeys)) {
            QString fn = prox.value(k).toString();
            QString fp = rootDir + "/" + fn;
            QStandardItem* item = new QStandardItem(ip.icon(QFileInfo(fp)), fn);
            item->setData(fp, Qt::UserRole);
            item->setData(true, Qt::UserRole + 1);
            item->setData(false, Qt::UserRole + 2);
            item->setData(false, Qt::UserRole + 3);
            root->appendRow(item);
        }
        prox.endGroup();

        prox.beginGroup("subdir");
        const QStringList skeys = prox.childKeys();
        for (const QString& k : std::as_const(skeys)) {
            QString sub = prox.value(k).toString();
            if (sub.endsWith('/')) sub.chop(1);
            QString sd = rootDir + "/" + sub;
            QStandardItem* si = new QStandardItem(ip.icon(QFileIconProvider::Folder), sub);
            si->setData(sd, Qt::UserRole);
            si->setData(false, Qt::UserRole + 1);
            si->setData(false, Qt::UserRole + 2);
            si->setData(true, Qt::UserRole + 3);
            root->appendRow(si);
            CrawlDir(si, sd, ip);
        }
        prox.endGroup();
    }

private:
    void CrawlDir(QStandardItem* parent, const QString& path, QFileIconProvider& ip) {
        QDir d(path);
        if (!d.exists()) return;
        const QStringList entries = d.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& e : std::as_const(entries)) {
            if (e.startsWith('.')) continue;
            QString fp = path + "/" + e;
            QFileInfo fi(fp);
            QStandardItem* item = new QStandardItem(ip.icon(fi), e);
            item->setData(fp, Qt::UserRole);
            item->setData(fi.isFile(), Qt::UserRole + 1);
            item->setData(false, Qt::UserRole + 2);
            item->setData(fi.isDir(), Qt::UserRole + 3);
            parent->appendRow(item);
            if (fi.isDir()) CrawlDir(item, fp, ip);
        }
    }
};

class ProjectCreateDialog : public QDialog {
public:
    QString projectName, projectPath, selectedTemplate;
    explicit ProjectCreateDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Create Project");
        setMinimumSize(500, 400);
        QVBoxLayout* l = new QVBoxLayout(this);
        QFormLayout* f = new QFormLayout();
        QLineEdit* nameEdit = new QLineEdit();
        QLineEdit* pathEdit = new QLineEdit(QDir::homePath());
        QPushButton* browse = new QPushButton("Browse...");
        QHBoxLayout* pl = new QHBoxLayout(); pl->addWidget(pathEdit); pl->addWidget(browse);
        f->addRow("Name:", nameEdit);
        f->addRow("Location:", pl);
        l->addLayout(f);
        l->addWidget(new QLabel("Templates:"));
        QListWidget* tplList = new QListWidget();
        QDir td(Settings::basePath() + "/project/templates");
        if (td.exists()) {
            const QStringList tpls = td.entryList(QStringList() << "*.prox", QDir::Files);
            for (const QString& t : std::as_const(tpls)) {
                QListWidgetItem* item = new QListWidgetItem(QFileInfo(t).baseName());
                item->setData(Qt::UserRole, td.absoluteFilePath(t));
                tplList->addItem(item);
            }
        }
        if (tplList->count() == 0) tplList->addItem("Default (all files)");
        l->addWidget(tplList);
        QHBoxLayout* bl = new QHBoxLayout();
        QPushButton* create = new QPushButton("Create");
        QPushButton* cancel = new QPushButton("Cancel");
        bl->addStretch(); bl->addWidget(create); bl->addWidget(cancel);
        l->addLayout(bl);
        connect(browse, &QPushButton::clicked, [pathEdit, this]() {
            QString p = QFileDialog::getExistingDirectory(this, "Select Location");
            if (!p.isEmpty()) pathEdit->setText(p);
        });
        auto acceptFn = [this, nameEdit, pathEdit, tplList]() {
            if (nameEdit->text().trimmed().isEmpty()) { QMessageBox::warning(this, "Error", "Project name required."); return; }
            projectName = nameEdit->text().trimmed();
            projectPath = pathEdit->text().trimmed();
            if (tplList->currentItem() && tplList->currentItem()->data(Qt::UserRole).isValid())
                selectedTemplate = tplList->currentItem()->data(Qt::UserRole).toString();
            QDialog::accept();
        };
        connect(create, &QPushButton::clicked, acceptFn);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(tplList, &QListWidget::itemDoubleClicked, acceptFn);
    }
};

class ProjectCore : public QObject, public Uplugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "vex.uplugin/4.0")
    Q_INTERFACES(Uplugin)

public:
    UpluginMetadata metadata() const override {
        UpluginMetadata m;
        m.name = "ProjectCore"; m.maintainer = "Vex Team"; m.version = "1.0";
        m.description = "Project management system"; m.icon = Settings::resolveIcon("folder");
        return m;
    }
    QList<QDockWidget*> dockWidgets() const override { return m_fileDock ? QList<QDockWidget*>{m_fileDock} : QList<QDockWidget*>{}; }
    QList<QAction*> toolbarActions() const override { return m_toolbarActions; }
    QList<QAction*> menuActions() const override { return {}; }
    void setBridge(UpluginBridge* bridge) override {
        m_bridge = bridge;
        BuildUI();
        PopulateRecentProjects();
        m_bridge->addMenu(m_projectMenu);
    }

private:
    UpluginBridge* m_bridge = nullptr;
    QDockWidget* m_fileDock = nullptr;
    QComboBox* m_viewMode = nullptr;
    ProjectTreeView* m_treeView = nullptr;
    QFileSystemModel* m_fsModel = nullptr;
    ProjectTreeModel* m_projModel = nullptr;
    PathBar* m_pathBar = nullptr;
    ProjectBar* m_projectBar = nullptr;
    QStackedWidget* m_barStack = nullptr;
    QStackedWidget* m_contentStack = nullptr;
    QWidget* m_noProjectPage = nullptr;
    QMenu* m_projectMenu = nullptr;
    QAction *m_runAction = nullptr, *m_stopAction = nullptr, *m_buildAction = nullptr;
    QAction *m_syncAction = nullptr, *m_reloadAction = nullptr, *m_configureAction = nullptr;
    QAction *m_newProjAct = nullptr, *m_openProjAct = nullptr, *m_closeProjAct = nullptr;
    QMenu* m_recentMenu = nullptr;
    QString m_currentProx;
    QProcess* m_runProcess = nullptr;
    QList<QAction*> m_toolbarActions;
    QStringList m_recentPaths;

    void UpdateActions() {
        bool has = !m_projectBar->isNone() && !m_currentProx.isEmpty();
        m_closeProjAct->setVisible(has);
        m_runAction->setVisible(has);
        m_stopAction->setVisible(has);
        m_buildAction->setVisible(has);
        m_syncAction->setVisible(has);
        m_reloadAction->setVisible(has);
        m_configureAction->setVisible(has);
    }

    void BuildUI() {
        m_fileDock = new QDockWidget("Files");
        m_fileDock->setObjectName("ProjectFilesDock");
        QWidget* w = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);

        m_barStack = new QStackedWidget();
        m_pathBar = new PathBar();
        m_projectBar = new ProjectBar();
        m_viewMode = new QComboBox();
        m_viewMode->addItem(Settings::resolveIcon("folder"), "Project Structure");
        m_viewMode->addItem(Settings::resolveIcon("drive-harddisk"), "Filesystem");
        m_viewMode->setCurrentIndex(1);

        QHBoxLayout* hl = new QHBoxLayout();
        hl->addWidget(m_viewMode);
        m_barStack->addWidget(m_projectBar);
        m_barStack->addWidget(m_pathBar);
        m_barStack->setCurrentIndex(1);
        hl->addWidget(m_barStack, 1);
        l->addLayout(hl);

        m_contentStack = new QStackedWidget();

        m_noProjectPage = new QWidget();
        QVBoxLayout* npl = new QVBoxLayout(m_noProjectPage);
        npl->addStretch();
        QLabel* noIcon = new QLabel();
        noIcon->setPixmap(Settings::resolveIcon("folder-open").pixmap(96, 96));
        noIcon->setAlignment(Qt::AlignCenter);
        npl->addWidget(noIcon);
        QLabel* noLabel = new QLabel("No Project Selected");
        noLabel->setAlignment(Qt::AlignCenter);
        noLabel->setStyleSheet("color: #888; font-size: 14px;");
        npl->addWidget(noLabel);
        npl->addStretch();

        m_treeView = new ProjectTreeView();
        m_treeView->setHeaderHidden(true);
        m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_fsModel = new QFileSystemModel();
        m_fsModel->setRootPath(QDir::rootPath());
        m_treeView->setFSModel(m_fsModel);
        m_projModel = new ProjectTreeModel(this);

        m_contentStack->addWidget(m_noProjectPage);
        m_contentStack->addWidget(m_treeView);
        m_contentStack->setCurrentIndex(1);
        l->addWidget(m_contentStack, 1);
        m_fileDock->setWidget(w);

        m_projectMenu = new QMenu("Project");

        m_newProjAct = m_projectMenu->addAction(Settings::resolveIcon("document-new"), "New Project...\tCtrl+Shift+N");
        m_openProjAct = m_projectMenu->addAction(Settings::resolveIcon("document-open"), "Open Project...\tCtrl+Shift+O");
        m_recentMenu = m_projectMenu->addMenu(Settings::resolveIcon("document-open-recent"), "Recent Projects");
        m_projectMenu->addSeparator();
        m_closeProjAct = m_projectMenu->addAction(Settings::resolveIcon("window-close"), "Close Project");
        m_projectMenu->addSeparator();
        m_runAction = m_projectMenu->addAction(Settings::resolveIcon("media-playback-start"), "Run");
        m_stopAction = m_projectMenu->addAction(Settings::resolveIcon("media-playback-stop"), "Stop");
        m_buildAction = m_projectMenu->addAction(Settings::resolveIcon("run-build"), "Build");
        m_projectMenu->addSeparator();
        m_syncAction = m_projectMenu->addAction(Settings::resolveIcon("view-refresh"), "Sync");
        m_reloadAction = m_projectMenu->addAction(Settings::resolveIcon("view-refresh"), "Reload");
        m_configureAction = m_projectMenu->addAction(Settings::resolveIcon("document-properties"), "Configure...");

        m_stopAction->setEnabled(false);
        UpdateActions();

        m_toolbarActions << m_runAction << m_stopAction << m_buildAction << m_syncAction << m_reloadAction;

        connect(new QShortcut(QKeySequence::Copy, m_treeView), &QShortcut::activated, this, &ProjectCore::CopySelected);
        connect(new QShortcut(QKeySequence::Paste, m_treeView), &QShortcut::activated, this, &ProjectCore::PasteToCurrent);
        connect(new QShortcut(QKeySequence::Cut, m_treeView), &QShortcut::activated, this, &ProjectCore::CutSelected);
        connect(new QShortcut(QKeySequence("Ctrl+Shift+N"), m_treeView), &QShortcut::activated, this, &ProjectCore::AddNew);
        connect(new QShortcut(QKeySequence::Delete, m_treeView), &QShortcut::activated, this, &ProjectCore::DeleteSelected);
        connect(new QShortcut(QKeySequence("F5"), m_treeView), &QShortcut::activated, this, [this]() { RefreshView(); });

        connect(m_runAction, &QAction::triggered, this, &ProjectCore::ExecuteRun);
        connect(m_stopAction, &QAction::triggered, this, &ProjectCore::HaltProcess);
        connect(m_buildAction, &QAction::triggered, this, &ProjectCore::ExecuteBuild);
        connect(m_syncAction, &QAction::triggered, this, &ProjectCore::SyncProject);
        connect(m_reloadAction, &QAction::triggered, this, [this]() { if (!m_currentProx.isEmpty()) OpenProx(m_currentProx); });
        connect(m_configureAction, &QAction::triggered, this, [this]() { if (!m_currentProx.isEmpty()) m_bridge->openDoc(m_currentProx); });
        connect(m_newProjAct, &QAction::triggered, this, &ProjectCore::CreateProject);
        connect(m_openProjAct, &QAction::triggered, this, &ProjectCore::OpenProject);
        connect(m_closeProjAct, &QAction::triggered, this, [this]() {
            m_projectBar->setProjects(m_recentPaths, QString());
        });

        connect(m_pathBar, &PathBar::pathSubmitted, [this]() {
            QString p = m_pathBar->path();
            if (p.isEmpty() || !QDir(p).exists()) return;
            m_treeView->setRootIndex(m_fsModel->index(p));
        });

        connect(m_projectBar, &ProjectBar::projectSelected, [this](int) {
            if (m_projectBar->isNone()) {
                m_currentProx.clear();
                m_projModel->clear();
                m_contentStack->setCurrentIndex(0);
                UpdateActions();
                return;
            }
            QString p = m_projectBar->currentPath();
            QDir d(p);
            const QStringList pf = d.entryList(QStringList() << ".*.prox", QDir::Files | QDir::Hidden);
            if (!pf.isEmpty()) OpenProx(p + "/" + pf.first());
        });

        connect(m_viewMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
            if (idx == 0) {
                m_barStack->setCurrentIndex(0);
                if (m_projectBar->isNone()) {
                    m_contentStack->setCurrentIndex(0);
                } else {
                    m_contentStack->setCurrentIndex(1);
                    m_treeView->setModel(m_projModel);
                    m_treeView->expandAll();
                }
            } else {
                m_barStack->setCurrentIndex(1);
                m_contentStack->setCurrentIndex(1);
                m_treeView->setModel(m_fsModel);
                QString target = m_projectBar->isNone() ? QDir::currentPath() : QFileInfo(m_currentProx).absolutePath();
                m_treeView->setRootIndex(m_fsModel->index(target));
                m_pathBar->setPath(target);
            }
        });

        connect(m_treeView, &QTreeView::doubleClicked, [this](const QModelIndex& idx) {
            if (!idx.isValid()) return;
            QString path = m_treeView->pathFromIndex(idx);
            if (m_treeView->isFileFromIndex(idx) && !path.isEmpty()) m_bridge->openDoc(path);
        });

        connect(m_treeView, &ProjectTreeView::middleClick, [this](const QString& path) {
            if (QFileInfo(path).isFile()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        });

        connect(m_treeView, &ProjectTreeView::contextMenu, this, &ProjectCore::ShowContextMenu);
    }

    QString CurrentDir() {
        if (!m_projectBar->isNone() && m_viewMode->currentIndex() == 0) return QFileInfo(m_currentProx).absolutePath();
        return m_pathBar->path();
    }

    QStringList SelectedPaths() {
        QStringList paths;
        const QModelIndexList sel = m_treeView->selectionModel()->selectedRows();
        for (const QModelIndex& idx : sel)
            paths << m_treeView->pathFromIndex(idx);
        return paths;
    }

    void CopySelected() {
        s_clipboard.clear();
        const QStringList paths = SelectedPaths();
        for (const QString& p : std::as_const(paths))
            s_clipboard.append({p, false});
    }

    void CutSelected() {
        s_clipboard.clear();
        const QStringList paths = SelectedPaths();
        for (const QString& p : std::as_const(paths))
            s_clipboard.append({p, true});
    }

    void PasteToCurrent() {
        QString dest = CurrentDir();
        const QStringList sel = SelectedPaths();
        if (sel.size() == 1 && QFileInfo(sel[0]).isDir()) dest = sel[0];
        QStringList errors;
        for (const ClipEntry& ce : std::as_const(s_clipboard)) {
            QString name = QFileInfo(ce.path).fileName();
            QString dp = dest + "/" + name;
            if (ce.cut) {
                if (ce.path == dp) continue;
                if (QFile::exists(dp)) { errors << "Exists: " + name; continue; }
                if (!QFile::rename(ce.path, dp)) errors << "Move failed: " + name;
            } else {
                if (QFileInfo(ce.path).isDir()) {
                    if (!copyDir(ce.path, dp)) errors << "Copy failed: " + name;
                } else {
                    if (!QFile::copy(ce.path, dp)) errors << "Copy failed: " + name;
                }
            }
        }
        s_clipboard.clear();
        if (!errors.isEmpty()) QMessageBox::warning(nullptr, "Errors", errors.join("\n"));
        RefreshView();
    }

    bool copyDir(const QString& src, const QString& dst) {
        QDir().mkpath(dst);
        QDir s(src);
        const QStringList entries = s.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& e : std::as_const(entries)) {
            QString se = src + "/" + e, de = dst + "/" + e;
            if (QFileInfo(se).isDir()) { if (!copyDir(se, de)) return false; }
            else { if (!QFile::copy(se, de)) return false; }
        }
        return true;
    }

    void DeleteSelected() {
        const QStringList paths = SelectedPaths();
        if (paths.isEmpty()) return;
        QStringList names;
        for (const QString& p : std::as_const(paths)) names << QFileInfo(p).fileName();
        if (QMessageBox::question(nullptr, "Delete", "Delete:\n" + names.join("\n") + "\n\nThis cannot be undone.",
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        QStringList errors;
        for (const QString& p : std::as_const(paths)) {
            if (QFileInfo(p).isDir()) { if (!QDir(p).removeRecursively()) errors << "Failed: " + QFileInfo(p).fileName(); }
            else { if (!QFile::remove(p)) errors << "Failed: " + QFileInfo(p).fileName(); }
        }
        if (!errors.isEmpty()) QMessageBox::warning(nullptr, "Errors", errors.join("\n"));
        RefreshView();
    }

    void AddNew() {
        QString name = QInputDialog::getText(nullptr, "New", "Name (trailing / for folder):");
        if (name.isEmpty()) return;
        QString dest = CurrentDir();
        const QStringList sel = SelectedPaths();
        if (sel.size() == 1 && QFileInfo(sel[0]).isDir()) dest = sel[0];
        if (name.endsWith('/')) {
            name.chop(1);
            if (!QDir().mkpath(dest + "/" + name)) QMessageBox::warning(nullptr, "Error", "Failed to create folder: " + name);
        } else {
            QFile f(dest + "/" + name);
            if (!f.open(QIODevice::WriteOnly)) QMessageBox::warning(nullptr, "Error", "Failed to create file: " + name);
            f.close();
        }
        RefreshView();
    }

    void ShowContextMenu(const QPoint& pos, const QString& path, bool isFile) {
        QMenu menu;
        if (isFile) {
            menu.addAction(Settings::resolveIcon("document-open"), "Open in New Tab", [this, path]() { m_bridge->openDoc(path); });
            menu.addAction(Settings::resolveIcon("application-x-executable"), "Open in Default App", [path]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            });
            menu.addSeparator();
        }
        menu.addAction(Settings::resolveIcon("document-new"), "Add New...\tCtrl+N", this, &ProjectCore::AddNew);
        menu.addAction(Settings::resolveIcon("edit-copy"), "Copy\tCtrl+C", this, &ProjectCore::CopySelected);
        menu.addAction(Settings::resolveIcon("edit-cut"), "Cut\tCtrl+X", this, &ProjectCore::CutSelected);
        QAction* paste = menu.addAction(Settings::resolveIcon("edit-paste"), "Paste\tCtrl+V", this, &ProjectCore::PasteToCurrent);
        if (s_clipboard.isEmpty()) paste->setEnabled(false);
        QAction* del = menu.addAction(Settings::resolveIcon("edit-delete"), "Delete\tDel", this, &ProjectCore::DeleteSelected);
        if (path.isEmpty()) del->setEnabled(false);
        menu.addSeparator();
        menu.addAction(Settings::resolveIcon("view-refresh"), "Refresh\tF5", this, &ProjectCore::RefreshView);
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    }

    void RefreshView() {
        if (!m_projectBar->isNone() && m_viewMode->currentIndex() == 0) OpenProx(m_currentProx);
    }

    void PopulateRecentProjects() {
        m_recentMenu->clear();
        QSettings s(Settings::basePath() + "/project/recent.conf", QSettings::IniFormat);
        m_recentPaths = s.value("projects").toStringList();
        m_recentPaths.erase(std::remove_if(m_recentPaths.begin(), m_recentPaths.end(),
                                           [](const QString& p) { return !QDir().exists(p); }), m_recentPaths.end());

        for (const QString& r : std::as_const(m_recentPaths)) {
            QAction* act = m_recentMenu->addAction(QDir(r).dirName());
            connect(act, &QAction::triggered, [this, p = r]() {
                QDir d(p);
                const QStringList pf = d.entryList(QStringList() << ".*.prox", QDir::Files | QDir::Hidden);
                if (!pf.isEmpty()) OpenProx(p + "/" + pf.first());
            });
        }
        if (!m_recentPaths.isEmpty()) {
            m_recentMenu->addSeparator();
            m_recentMenu->addAction("Clear Recent", [this]() {
                QSettings s2(Settings::basePath() + "/project/recent.conf", QSettings::IniFormat);
                s2.remove("projects"); s2.sync();
                m_recentPaths.clear();
                PopulateRecentProjects();
            });
        }
        m_projectBar->setProjects(m_recentPaths, m_currentProx.isEmpty() ? QString() : QFileInfo(m_currentProx).absolutePath());
    }

    void CreateProject() {
        ProjectCreateDialog dlg;
        if (dlg.exec() != QDialog::Accepted) return;
        QString fullPath = dlg.projectPath + "/" + dlg.projectName;
        QString proxPath = fullPath + "/." + dlg.projectName + ".prox";
        QDir().mkpath(fullPath);

        if (!dlg.selectedTemplate.isEmpty()) {
            QSettings tpl(dlg.selectedTemplate, QSettings::IniFormat);
            QFileInfo fi(dlg.selectedTemplate);
            QSettings newProx(proxPath, QSettings::IniFormat);
            const QStringList groups = tpl.childGroups();
            for (const QString& g : std::as_const(groups)) {
                tpl.beginGroup(g);
                const QStringList keys = tpl.childKeys();
                for (const QString& k : std::as_const(keys)) {
                    if (k.startsWith("start.") || k.startsWith("end.")) continue;
                    QString val = tpl.value(k).toString();
                    val.replace("${NAME}", dlg.projectName);
                    newProx.setValue(g + "/" + k, val);
                }
                tpl.endGroup();
            }
            newProx.setValue("project/name", dlg.projectName);
            newProx.sync();

            tpl.beginGroup("files");
            const QStringList fkeys = tpl.childKeys();
            for (const QString& k : std::as_const(fkeys)) {
                QString fn = tpl.value(k).toString();
                fn.replace("${NAME}", dlg.projectName);
                QString dst = fullPath + "/" + fn;
                QDir().mkpath(QFileInfo(dst).absolutePath());
                QString start = tpl.value("start." + k).toString();
                QString end = tpl.value("end." + k).toString();
                if (!start.isEmpty() || !end.isEmpty()) {
                    start.replace("${NAME}", dlg.projectName);
                    end.replace("${NAME}", dlg.projectName);
                    QFile f(dst);
                    if (f.open(QIODevice::WriteOnly)) {
                        f.write(start.toUtf8());
                        if (!end.isEmpty()) f.write(("\n" + end).toUtf8());
                        f.close();
                    }
                } else {
                    QString src = fi.absolutePath() + "/" + fn;
                    if (QFile::exists(src)) QFile::copy(src, dst);
                    else { QFile f(dst); f.open(QIODevice::WriteOnly); f.close(); }
                }
            }
            tpl.endGroup();
        } else {

            QSettings prox(proxPath, QSettings::IniFormat);
            QString projName = QDir(fullPath).dirName();
            prox.beginGroup("project");
            prox.setValue("name", projName);
            prox.setValue("root", "./");
            prox.endGroup();

            prox.beginGroup("files");
            QDir d(fullPath);
            QStringList files = d.entryList(QDir::Files | QDir::NoDotAndDotDot);
            files.erase(std::remove_if(files.begin(), files.end(), [](const QString& s) { return s.startsWith('.'); }), files.end());
            int idx = 1;
            for (const QString& f : std::as_const(files)) prox.setValue(QString::number(idx++), f);
            prox.endGroup();

            prox.beginGroup("subdir");
            static const QStringList skip = {".git", "build", "__pycache__", "node_modules", ".vs", ".vscode", "obj", "bin", "Debug", "Release"};
            QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            dirs.erase(std::remove_if(dirs.begin(), dirs.end(), [](const QString& s) {
                           return s.startsWith('.') || skip.contains(s);
                       }), dirs.end());
            idx = 1;
            for (const QString& dir : std::as_const(dirs)) prox.setValue(QString::number(idx++), dir + "/");
            prox.endGroup();
            prox.sync();
        }

        OpenProx(proxPath);
        RecordRecent(fullPath);
    }

    void OpenProject() {
        QString p = QFileDialog::getExistingDirectory(nullptr, "Open Project");
        if (p.isEmpty()) return;
        QDir d(p);
        const QStringList pf = d.entryList(QStringList() << ".*.prox", QDir::Files | QDir::Hidden);
        QString proxPath = pf.isEmpty() ? p + "/." + d.dirName() + ".prox" : p + "/" + pf.first();
        if (pf.isEmpty()) {
            QSettings prox(proxPath, QSettings::IniFormat);
            prox.setValue("project/name", d.dirName());
            prox.setValue("project/root", "./");
            prox.sync();
        }
        OpenProx(proxPath);
        RecordRecent(p);
    }

    void SyncProject() {
        if (m_currentProx.isEmpty()) return;
        QSettings prox(m_currentProx, QSettings::IniFormat);
        QString dirPath = QFileInfo(m_currentProx).absolutePath();
        QDir d(dirPath);
        prox.beginGroup("files");
        prox.remove("");
        QStringList files = d.entryList(QDir::Files | QDir::NoDotAndDotDot);
        files.erase(std::remove_if(files.begin(), files.end(), [](const QString& s) { return s.startsWith('.'); }), files.end());
        int idx = 1;
        for (const QString& f : std::as_const(files)) prox.setValue(QString::number(idx++), f);
        prox.endGroup();
        prox.sync();
        OpenProx(m_currentProx);
    }

    void OpenProx(const QString& proxPath) {
        m_currentProx = proxPath;
        QSettings prox(proxPath, QSettings::IniFormat);
        prox.beginGroup("config");
        const QStringList ckeys = prox.childKeys();
        for (const QString& k : std::as_const(ckeys)) {
            if (Settings::instance().contains("config/" + k))
                Settings::instance().setValue("config/" + k, prox.value(k).toString());
        }
        prox.endGroup();

        m_projModel->LoadFromProx(proxPath);
        m_viewMode->setCurrentIndex(0);
        m_barStack->setCurrentIndex(0);
        m_contentStack->setCurrentIndex(1);
        m_treeView->setModel(m_projModel);
        m_treeView->expandAll();
        m_projectBar->setProjects(m_recentPaths, QFileInfo(proxPath).absolutePath());
        UpdateActions();
    }

    void ExecuteRun() {
        if (m_currentProx.isEmpty()) return;
        QSettings prox(m_currentProx, QSettings::IniFormat);
        QString wd = QFileInfo(m_currentProx).absolutePath();

        QString exec = prox.value("run/exec").toString();
        QString exterm = prox.value("run/exterm").toString();
        QString xopen = prox.value("run/xopen").toString();

        qDebug() << "[ProjectCore] ExecuteRun - working dir:" << wd;
        qDebug() << "[ProjectCore]   exec:" << exec;
        qDebug() << "[ProjectCore]   exterm:" << exterm;
        qDebug() << "[ProjectCore]   xopen:" << xopen;

        if (!exterm.isEmpty()) {
            qDebug() << "[ProjectCore] Starting process with terminal:" << exterm;
            m_runProcess = new QProcess(this);
            m_runProcess->setWorkingDirectory(wd);
            m_runProcess->start(exterm);
            m_runAction->setEnabled(false);
            m_stopAction->setEnabled(true);
        }
        if (!exec.isEmpty()) {
            qDebug() << "[ProjectCore] Starting detached process:" << exec;
            bool ok = QProcess::startDetached(exec, {}, wd);
            qDebug() << "[ProjectCore]   started:" << ok;
        }
        if (!xopen.isEmpty()) {
            QString path = QFileInfo(xopen).isAbsolute() ? xopen : wd + "/" + xopen;
            qDebug() << "[ProjectCore] Opening with default app:" << path;
            if (QFile::exists(path)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                qDebug() << "[ProjectCore]   file exists, opened";
            } else {
                qDebug() << "[ProjectCore]   file not found:" << path;
            }
        }
    }

    void ExecuteBuild() {
        if (m_currentProx.isEmpty()) return;
        QSettings prox(m_currentProx, QSettings::IniFormat);
        QString wd = QFileInfo(m_currentProx).absolutePath();
        QString exec = prox.value("build/exec").toString();
        bool runAfter = prox.value("build/run_after_build", false).toBool();

        qDebug() << "[ProjectCore] ExecuteBuild - working dir:" << wd;
        qDebug() << "[ProjectCore]   exec:" << exec;
        qDebug() << "[ProjectCore]   run_after_build:" << runAfter;

        if (exec.isEmpty()) {
            qDebug() << "[ProjectCore]   no build command, skipping";
            return;
        }

        QProcess* bp = new QProcess(this);
        bp->setWorkingDirectory(wd);
        connect(bp, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, bp, runAfter](int code, QProcess::ExitStatus status) {
                    qDebug() << "[ProjectCore] Build finished - exit code:" << code << "status:" << status;
                    m_bridge->status(code == 0 ? "Build succeeded" : "Build failed", 5000);
                    if (code == 0 && runAfter) {
                        qDebug() << "[ProjectCore]   run_after_build=true, executing run...";
                        ExecuteRun();
                    }
                    bp->deleteLater();
                });
        qDebug() << "[ProjectCore] Starting build:" << exec;
        bp->start(exec);
    }

    void HaltProcess() {
        qDebug() << "[ProjectCore] HaltProcess_ process:" << m_runProcess << "state:" << (m_runProcess ? m_runProcess->state() : -1);
        if (m_runProcess && m_runProcess->state() != QProcess::NotRunning) {
            m_runProcess->terminate();
            qDebug() << "[ProjectCore]   terminated, waiting...";
            bool finished = m_runProcess->waitForFinished(3000);
            qDebug() << "[ProjectCore]   finished:" << finished;
            if (!finished) {
                m_runProcess->kill();
                qDebug() << "[ProjectCore]   killed";
            }
        }
        m_runAction->setEnabled(true);
        m_stopAction->setEnabled(false);
    }

    void RecordRecent(const QString& path) {
        QSettings s(Settings::basePath() + "/project/recent.conf", QSettings::IniFormat);
        m_recentPaths.removeAll(path);
        m_recentPaths.prepend(path);
        if (m_recentPaths.size() > 10) m_recentPaths = m_recentPaths.mid(0, 10);
        s.setValue("projects", m_recentPaths);
        s.sync();
        PopulateRecentProjects();
    }
};

#include "ProjectCore.moc"
