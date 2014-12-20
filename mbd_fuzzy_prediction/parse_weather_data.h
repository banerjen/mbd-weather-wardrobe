#include "iostream"
#include "cstdlib"
#include "fstream"
#include "cstring"
#include "limits"
#include "exception"
#include "libxml/tree.h"
#include "libxml/parser.h"

namespace WeatherData
{
	enum WeatherParameter
	{
		TEMPERATURE = 50,	// Units - F
		SNOWFALL,			// Units - inches
		RAINFALL,			// Units - inches
		WIND				// Units - mph
	};
}

class WeatherParser
{
	protected:
		std::string 	weather_data_file_;
		double 			temperature_;	
		double			rainfall_;
		double			snowfall_;
		double			wind_;	

		bool parseWeatherData();
		void printXMLElementNames(xmlNode *root_element);

	public:
		WeatherParser();
		~WeatherParser();

		void getWeatherData(WeatherData::WeatherParameter parameter, double &value);		
		void setWeatherDataFile(std::string file_path);
};
