/**
 ********************************************************************************************************
 * @file    main.cpp
 * @brief   Clothes predictor main program
 * @details Weather based clothes prediction using a Mamdani Fuzzy Inference Engine
 ********************************************************************************************************
 */

#include "fuzzy_weather_prediction.h"
#include "parse_weather_data.h"

// Temporary implementation
void getClothesData(std::vector< std::pair<int, std::string> > &clothes_database)
{
	clothes_database.push_back(std::pair<int, std::string>(0, "Half Sleeve Shirt"));
	clothes_database.push_back(std::pair<int, std::string>(1, "Full Sleeve Shirt"));
	clothes_database.push_back(std::pair<int, std::string>(2, "Thermal"));
	clothes_database.push_back(std::pair<int, std::string>(3, "Shorts"));
	clothes_database.push_back(std::pair<int, std::string>(4, "Half Sleeve Shirt"));
	clothes_database.push_back(std::pair<int, std::string>(5, "Casual Pants"));
	clothes_database.push_back(std::pair<int, std::string>(6, "Light Jacket"));
	clothes_database.push_back(std::pair<int, std::string>(7, "Full Sleeve Shirt"));
	clothes_database.push_back(std::pair<int, std::string>(8, "Heavy Jacket"));
	clothes_database.push_back(std::pair<int, std::string>(9, "Casual Pants"));
	clothes_database.push_back(std::pair<int, std::string>(10, "Coat"));
	clothes_database.push_back(std::pair<int, std::string>(11, "Jeans"));
	clothes_database.push_back(std::pair<int, std::string>(12, "Thermal"));
	clothes_database.push_back(std::pair<int, std::string>(13, "Light Jacket"));
	clothes_database.push_back(std::pair<int, std::string>(14, "Coat"));
	clothes_database.push_back(std::pair<int, std::string>(15, "Half Sleeve Shirt"));
}

int main(int argc, char *argv[])
{
    WWFuzzyPredictionPtr                        fuzzy_inference;
    AprilTagDetector                            tag_detector;
    WeatherParser                               weather_parser;
    double                                      value;
    char                                        c = 'y';
    std::vector<int>                            tags_list;
	std::vector< std::pair<int, std::string> >	clothes_database;

	// DATABASE FUNCTION GOES HERE	
	getClothesData(clothes_database);

	fuzzy_inference = new WWFuzzyPrediction();
	fuzzy_inference->initialiseFuzzy();	

	tag_detector.initialiseTagDetector();	

	weather_parser.setWeatherDataFile(std::string("../mbd_get_weather_data/weather_data.xml"));			

	while (c == 'y')
	{		
        tag_detector.detectTags();
        tags_list.clear();
        tag_detector.getDetectedTagIds(tags_list);
        printTagsCorrespondingClothes(clothes_database, tags_list);
        getClothesInCloset(clothes_database, tags_list);
		
		weather_parser.getWeatherData(WeatherData::TEMPERATURE, value);
        if (value == std::numeric_limits<double>::quiet_NaN())
            value = -200.0;
        // Conversion from Kelvin to Fahrenheit - Fuzzy engine input is in Fahrenheit.
        value = (9.0 / 5.0 * (value - 273.15)) + 32.0;
		fuzzy_inference->setInput(TEMPERATURE, value);
        std::cout << "Temperature: " << value << " F" <<std::endl;
        weather_parser.getWeatherData(WeatherData::SNOWFALL, value);
		fuzzy_inference->setInput(SNOWFALL, value);
        std::cout << "Snowfall: " << value << " inches" << std::endl;
        weather_parser.getWeatherData(WeatherData::RAINFALL, value);
		fuzzy_inference->setInput(RAINFALL, value);
        std::cout << "Rainfall: " << value << " inches" << std::endl;
        weather_parser.getWeatherData(WeatherData::WIND, value);
        std::cout << "Wind: " << value << " mph" << std::endl;
        // Wind not set up correctly in the Fuzzy engine
        value = 0.0;
		fuzzy_inference->setInput(WIND, value);		
		
		fuzzy_inference->startFuzzy();
		displayOutput(fuzzy_inference);
		displayClothes(fuzzy_inference, clothes_database);
		std::cout << "Do you want a new selection? (y/n): "; 
		std::cin >> c;
    }

    return 0;
}
