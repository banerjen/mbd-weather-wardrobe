#include "parse_weather_data.h"

WeatherParser::WeatherParser()
{
    temperature_    = std::numeric_limits<double>::quiet_NaN();
    rainfall_       = 0.0;
    snowfall_       = 0.0;
    wind_           = 0.0;
}

WeatherParser::~WeatherParser()
{
    // Destructor code goes here
}

void WeatherParser::setWeatherDataFile(std::string file_path)
{
    weather_data_file_ = file_path;
}

void WeatherParser::getWeatherData(WeatherData::WeatherParameter parameter, double &value)
{
    parseWeatherData();

	switch (parameter)
	{
		case WeatherData::TEMPERATURE:
            value = temperature_;
			break;
		case WeatherData::SNOWFALL:            
            value = snowfall_;
			break;
		case WeatherData::RAINFALL:
            value = rainfall_;
			break;
		case WeatherData::WIND:
            value = wind_;
			break;
		default:
			value = std::numeric_limits<double>::quiet_NaN();		
	}
}

void WeatherParser::printXMLElementNames(xmlNode *root_element)
{
	xmlNode*  cur_node = NULL;    

    for (cur_node = root_element; cur_node; cur_node = cur_node->next)
    {        
        if (cur_node->type == XML_ELEMENT_NODE)
        {            
            if (std::strcmp((char*) cur_node->name, "temperature") == 0)
            {
                temperature_ = std::atof((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value"));
                continue;
            }
            else if (std::strcmp((char*) cur_node->name, "precipitation") == 0)
            {
                if ((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value") != NULL)
                    rainfall_ = std::atof((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value"));
                else
                    rainfall_ = 0.0;
                continue;
            }
            else if (std::strcmp((char*) cur_node->name, "snow") == 0)
            {
                if ((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value") != NULL)
                    snowfall_ = std::atof((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value"));
                else
                    snowfall_ = 0.0;
                continue;
            }
            else if (std::strcmp((char*) cur_node->name, "speed") == 0)
            {
                if ((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value") != NULL)
                    wind_ = std::atof((char*) xmlGetNoNsProp(cur_node, (const xmlChar*) "value"));
                else
                    wind_ = 0.0;
                continue;
            }
        }

        printXMLElementNames(cur_node->children);
    }    
}

bool WeatherParser::parseWeatherData()
{
try
    {
        LIBXML_TEST_VERSION

        xmlDoc*   doc = NULL;
        xmlNode*  root_element = NULL;

        doc = xmlReadFile(weather_data_file_.c_str(), NULL, 0);
        if (doc == NULL)
        {
            std::cerr << "Failed to parse filename. " << weather_data_file_;
            return false;
        }

        root_element = xmlDocGetRootElement(doc);

        temperature_    = std::numeric_limits<double>::quiet_NaN();
        rainfall_       = 0;//std::numeric_limits<double>::quiet_NaN();
        snowfall_       = 0;//std::numeric_limits<double>::quiet_NaN();
        wind_           = 0;//std::numeric_limits<double>::quiet_NaN();    

        printXMLElementNames(root_element);

        xmlFreeDoc(doc);
        xmlCleanupParser();
    }
    catch (std::exception &ex)
    {
        std::cerr << "std Exception: " << ex.what();
        return false;
    }
    catch (...)
    {
        std::cerr << "Unknown Exception.";
        return false;
    }

    return true;
}
