#ifndef SETTINGS_H
#define SETTINGS_H
#include <QSettings>
#include <QDir>
#include <QIcon>
#include <QVariant>
#include <QFile>
#include <functional>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class Settings {
public:
    using IconResolver = std::function<QIcon(const QString&)>;

    static Settings& instance() {
        static Settings _instance;
        return _instance;
    }

    void setValue(const QString& key, const QVariant& value) {
        settings.setValue(key, value);
    }

    template<typename T>
    T get(const QString& key, const T& defaultValue = T()) {
        return settings.value(key, defaultValue).template value<T>();
    }

    void remove(const QString& key) {
        settings.remove(key);
    }

    bool contains(const QString& key) {
        return settings.contains(key);
    }

    QString configFilePath() const {
        return basePath() + "/vex.conf";
    }

    void setIconResolver(IconResolver resolver) {
        m_iconResolver = std::move(resolver);
    }

    static QIcon resolveIcon(const QString &baseName) {
        static const QStringList exts = {"svg", "png", "ico", "icns", "svgz"};
        QString userIconPath = instance().get<QString>("selected_user_icon_path");
        if (!userIconPath.isEmpty()) {
            for (const QString &ext : exts) {
                QString f = userIconPath + "/" + baseName + "." + ext;
                if (QFile::exists(f)) return QIcon(f);
            }
        }

        for (const QString &ext : exts) {
            const QString qrcPath = ":/" + baseName + "." + ext;
            if (QFile::exists(qrcPath)) return QIcon(qrcPath);
        }

        return QIcon::fromTheme(baseName);
    }


    static QString basePath() {
        QString home = QDir::homePath();
        QString path = home + "/.vex";
        QDir().mkpath(path);
#ifdef Q_OS_WIN
        SetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()), FILE_ATTRIBUTE_HIDDEN);
#endif
        return path;
    }

private:
    Settings()
        : settings(configFilePath(), QSettings::IniFormat) {
        QDir().mkpath(basePath());
        QString confFile = configFilePath();
        if (!QFile::exists(confFile)) {
            QFile file(confFile);
            if (file.open(QIODevice::WriteOnly))
                file.close();
        }
    }

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    QSettings settings;
    IconResolver m_iconResolver;
};

#endif // SETTINGS_H
