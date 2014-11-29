#include "wwcontrollerwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WWControllerWindow w;

    w.setConfigurationFilePath(QString("/home/nbanerjee/MBD_data/config.yaml"));
    w.readConfigurationFile();

    w.show();

    return a.exec();
}
