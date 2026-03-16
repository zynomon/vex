#include <QApplication>
#include <QMainWindow>
#include "Settings.H"
#include "Plugvex.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowIcon(Settings::resolveIcon("vex"));
        resize(1000, 700); // initial look
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow vex;


    vex.show();
    CmdLine& cmdLine = CmdLine::instance();
    PluginLoader::loadAndInitialize(&vex, &Settings::instance(), cmdLine, argc, argv);

    int result = app.exec();

    PluginLoader::cleanup();
    return result;
}

#include "main.moc"
