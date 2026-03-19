#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include "Settings.H"
#include "Plugvex.H"

class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    QLabel* m_loadingIcon;

public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("Vex :/");
        setWindowIcon(Settings::resolveIcon("vex"));
        resize(1000, 700);

        m_loadingIcon = new QLabel(this);
        m_loadingIcon->setPixmap(Settings::resolveIcon("vex").pixmap(200, 200));
        m_loadingIcon->setAlignment(Qt::AlignCenter);
        setCentralWidget(m_loadingIcon);
    }

    void setMainUI(QWidget* mainUI) {
        if (m_loadingIcon) {
            m_loadingIcon->deleteLater();
            m_loadingIcon = nullptr;
        }
        setCentralWidget(mainUI);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow vex;

    CmdLine& cmdLine = CmdLine::instance();
    PluginLoader::loadAndInitialize(&vex, &Settings::instance(), cmdLine, argc, argv);

    vex.show();

    int result = app.exec();
    PluginLoader::cleanup();
    return result;
}

#include "main.moc"
