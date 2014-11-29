#ifndef WWCONTROLLERWINDOW_H
#define WWCONTROLLERWINDOW_H



#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QMutex>
#include <fstream>
#include <typeinfo>
#include <yaml-cpp/yaml.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

//#include "clothes_representation.h"

namespace Ui {
class WWControllerWindow;
}

class WWControllerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WWControllerWindow(QWidget *parent = 0);
    ~WWControllerWindow();

    bool readConfigurationFile();
    bool readWeatherData();
    bool readWeatherData2();

    void printXMLElementNames(xmlNode *root_element);

    void setConfigurationFilePath(QString path);
    void getConfigurationFilePath(QString &path);

private:
    Ui::WWControllerWindow *ui;

    QTimer      timer_;
    QString     configuration_file_path_;

    // Config parameters
    QString     weather_data_file_;
    QString     data_folder_;
    QString     ip_addr_galileo_;

    // Current weather data
    QString     temperature_;
    QString     rainfall_;
    QString     snowfall_;

private slots:
    void on_timer_timeout();
    void on_btnReadWeather_clicked();
};

#endif // WWCONTROLLERWINDOW_H
