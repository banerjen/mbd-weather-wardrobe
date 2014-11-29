#include "wwcontrollerwindow.h"
#include "ui_wwcontrollerwindow.h"

WWControllerWindow::WWControllerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WWControllerWindow)
{
    ui->setupUi(this);    

    connect(&timer_, SIGNAL(timeout()), this, SLOT(on_timer_timeout()));

    timer_.setInterval(1000);
    timer_.start();
}

WWControllerWindow::~WWControllerWindow()
{
    delete ui;
}

bool WWControllerWindow::readConfigurationFile()
{
    try
    {
        std::ifstream   fin(configuration_file_path_.toLocal8Bit());
        YAML::Parser    parser(fin);
        YAML::Node      config;

        while (parser.GetNextDocument(config))
        {
            std::string data_folder;
            std::string weather_data_file;
            std::string ip_addr_galileo;

            config["data_folder"]           >> data_folder;
            config["weather_data_file"]     >> weather_data_file;
            config["IP_address_galileo"]    >> ip_addr_galileo;

            data_folder_        = QString::fromStdString(data_folder);
            weather_data_file_  = QString::fromStdString(weather_data_file);
            ip_addr_galileo_    = QString::fromStdString(ip_addr_galileo);
        }
    }
    catch (YAML::Exception &ex)
    {
        qDebug() << "YAML Exception: " << ex.what();
        return false;
    }
    catch (std::exception &ex)
    {
        qDebug() << "std Exception: " << ex.what();
        return false;
    }
    catch (...)
    {
        qDebug() << "Unknown Exception.";
        return false;
    }

    return true;
}

bool WWControllerWindow::readWeatherData()
{
    try
    {
        std::ifstream   fin(weather_data_file_.toLocal8Bit());
        YAML::Parser    parser(fin);
        YAML::Node      config;

        while (parser.GetNextDocument(config))
        {
            std::string temperature;
            std::string rainfall;
            std::string snowfall;

            config["temperature"]   >> temperature;
            config["rainfall"]      >> rainfall;
            config["snowfall"]      >> snowfall;

            temperature_    = QString::fromStdString(temperature);
            rainfall_       = QString::fromStdString(rainfall);
            snowfall_       = QString::fromStdString(snowfall);
        }
    }
    catch (YAML::Exception &ex)
    {
        qDebug() << "YAML Exception: " << ex.what();
        return false;
    }
    catch (std::exception &ex)
    {
        qDebug() << "std Exception: " << ex.what();
        return false;
    }
    catch (...)
    {
        qDebug() << "Unknown Exception.";
        return false;
    }

    return true;
}

void WWControllerWindow::printXMLElementNames(xmlNode *root_element)
{
    xmlNode*  cur_node = NULL;

    for (cur_node = root_element; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            ui->textBrowser->append(QString("Node type: Element, Name: ") + QString((char*) cur_node->name));
            //ui->textBrowser->append(QString((char*) xmlNodeGetContent(cur_node)));
            if ((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "id") != NULL)
                ui->textBrowser->append(QString("id: ") + QString((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "id")));
            if ((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "name") != NULL)
                ui->textBrowser->append(QString("name: ") + QString((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "name")));
            if ((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value") != NULL)
                ui->textBrowser->append(QString("value: ") + QString((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value")));
        }

        printXMLElementNames(cur_node->children);
    }
}

// Temperature. Precipitation. Wind.
bool WWControllerWindow::readWeatherData2()
{
    try
    {
        LIBXML_TEST_VERSION

        xmlDoc*   doc = NULL;
        xmlNode*  root_element = NULL;

        doc = xmlReadFile((weather_data_file_.toStdString()).c_str(), NULL, 0);
        if (doc == NULL)
        {
            qDebug() << "Failed to parse filename. " << weather_data_file_;
            return false;
        }

        root_element = xmlDocGetRootElement(doc);

        printXMLElementNames(root_element);

        xmlFreeDoc(doc);
        xmlCleanupParser();
    }
    catch (std::exception &ex)
    {
        qDebug() << "std Exception: " << ex.what();
        return false;
    }
    catch (...)
    {
        qDebug() << "Unknown Exception.";
        return false;
    }

    return true;
}

void WWControllerWindow::setConfigurationFilePath(QString path)
{
    configuration_file_path_ = path;
}

void WWControllerWindow::getConfigurationFilePath(QString &path)
{
    path = configuration_file_path_;
}

void WWControllerWindow::on_timer_timeout()
{
//    if (readWeatherData() == true)
//        ui->textBrowser->setText("Temperature: " + temperature_ + "\n" +
//                                 "Rainfall:    " + rainfall_ + "\n" +
//                                 "Snowfall:    " + snowfall_);
}

//xmlDocPtr   doc = NULL;
//xmlNodePtr  root_node = NULL;
//xmlDtdPtr   dtd = NULL;
//char        buff[256];
//int         i, j;

//LIBXML_TEST_VERSION;

//doc = xmlNewDoc(BAD_CAST "1.0");
//root_node = xmlNewNode(NULL, BAD_CAST "root");
//xmlDocSetRootElement(doc, root_node);
//dtd = xmlCreateIntSubset(doc, BAD_CAST "root", NULL, BAD_CAST "tree2.dtd");
//xmlNewChild(root_node, NULL, BAD_CAST "node1", BAD_CAST "contents of node 1");
//xmlNewChild(root_node, NULL, BAD_CAST "node2", NULL);
//node = xmlNewChild(root_node, NULL, BAD_CAST "node3", BAD_CAST "this node has attributes");
//xmlNewProp(node, BAD_CAST "attribute", BAD_CAST "yes");
//xmlNewProp(node, BAD_CAST, "foo", BAD_CAST "bar");

void WWControllerWindow::on_btnReadWeather_clicked()
{
    readWeatherData2();
}
