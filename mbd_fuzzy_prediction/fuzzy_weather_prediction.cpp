#include "fuzzy_weather_prediction.h"
#include "apriltag_detector.h"
#include "utility"
#include "string"
#include "cstring"

typedef WWFuzzyPrediction*	WWFuzzyPredictionPtr;

void getInput(WWFuzzyPredictionPtr&);

void getInput(WWFuzzyPredictionPtr &fuzzy_inference)
{
	double value;

	std::cout << "\nInput the current temperature(F): ";
    std::cin >> value;
    fuzzy_inference->setInput(TEMPERATURE, value);

    std::cout << "Input the current rainfall(in): ";
    std::cin >> value;        
    fuzzy_inference->setInput(RAINFALL, value);

    std::cout << "Input the current snowfall(in): ";
    std::cin >> value;
    fuzzy_inference->setInput(SNOWFALL, value);
        
    std::cout << "Input the current wind(mph): ";
    std::cin >> value;
    fuzzy_inference->setInput(WIND, value);
}

void displayOutput(WWFuzzyPredictionPtr &fuzzy_inference)
{
	double value;

	fuzzy_inference->getOutput(SHIRT, value);
	std::cout << "Shirt: " << value << std::endl;

	fuzzy_inference->getOutput(PANT, value);
	std::cout << "Pant: " << value << std::endl;	

	fuzzy_inference->getOutput(JACKET, value);
	std::cout << "Jacket: " << value << std::endl;
}

void displayClothes(WWFuzzyPredictionPtr &fuzzy_inference,
					std::vector< std::pair<int, std::string> > &clothes_database)
{
	double value;
	std::vector<int> clothing;

	fuzzy_inference->getOutput(SHIRT, value);
	if ((value >= 0) && (value <= 0.3))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{

			if (((std::string) clothes_database[i].second).compare("Half Sleeve Shirt") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}
	}
	else if ((value >= 0.25) && (value <= 0.5))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Full Sleeve Shirt") == 0)
			{
				clothing.push_back(i);
				break;
			}	
		}
	}
	else if ((value >= 0.45) && (value <= 0.75))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Full Sleeve Shirt") == 0)
			{
				clothing.push_back(i);
				break;
			}	
		}

		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Thermal") == 0)
			{
				clothing.push_back(i);
				break;
			}	
		}
	}
	else if ((value >= 0.7) && (value <= 1.0))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Full Sleeve Shirt") == 0)
			{
				clothing.push_back(i);
				break;
			}	
		}

		for (int i = 0; i < clothes_database.size(); i++)
		{
			if ((((std::string) clothes_database[i].second).compare("Full Sleeve Shirt") == 0) ||
				(((std::string) clothes_database[i].second).compare("Half Sleeve Shirt") == 0))
			{
				clothing.push_back(i);
				break;
			}	
		}		
	}

	fuzzy_inference->getOutput(PANT, value);
	if ((value >= 0.0) && (value <= 0.2))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Shorts") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}
	}
	else if ((value >= 0.15) && (value <= 0.5))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Casual Pants") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}
	}
	else if ((value >= 0.45) && (value <= 0.7))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Casual Pants") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}

		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Thermal") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}		
	}
	else if ((value >= 0.65) && (value <= 1.0))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Jeans") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}

		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Thermal") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}		
	}

	fuzzy_inference->getOutput(JACKET, value);
	if ((value >= 0.18) && (value <= 0.3))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Light Jacket") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}
	}
	else if ((value >= 0.25) && (value <= 0.65))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Coat") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}
	}
	else if ((value >= 0.6) && (value <= 1))
	{
		for (int i = 0; i < clothes_database.size(); i++)
		{
			if (((std::string) clothes_database[i].second).compare("Heavy Jacket") == 0)
			{
				clothing.push_back(i);
				break;
			}
		}
	}

	std::cout << "Clothes suggested: " << std::endl;

	for (int i = 0; i < clothing.size(); i++)
	{		
		std::cout << clothes_database[clothing[i]].second << std::endl;
	}
}

void getInputHardCode(WWFuzzyPredictionPtr &fuzzy_inference)
{
	double value;

	std::cout << "Downloading weather data ..." << std::endl;
	std::cout << "Parsing weather data XML file ..." << std::endl;

	value = 32;
	std::cout << "Temperature: " << value << std::endl;
	fuzzy_inference->setInput(TEMPERATURE, value);
	value = 0;
	std::cout << "Rainfall: " << value << std::endl;
	fuzzy_inference->setInput(RAINFALL, value);
	value = 0;
	std::cout << "Snowfall: " << value << std::endl;
	fuzzy_inference->setInput(SNOWFALL, value);
	value = 0;
	std::cout << "Wind: " << value << std::endl;
	fuzzy_inference->setInput(WIND, value);	
}

void printTagsHardCode(std::vector< std::pair<int, std::string> > &clothes_database,
					   std::vector<int> &tags_list)
{
	std::cout << "Tags detected: " << std::endl;

	for (int i = 0; i < tags_list.size(); i++)
	{
		std::cout << "Detected Tag Id: " << tags_list[i] << " -- " << clothes_database[tags_list[i]].second << std::endl;
	}
}

int main(int argc, char *argv[])
{
	WWFuzzyPredictionPtr	fuzzy_inference;
	AprilTagDetector        tag_detector;
	double 					value;
	char					c = 'y';
	std::vector<int>		tags_list;
	std::vector< std::pair<int, std::string> >	clothes_database;

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

	fuzzy_inference = new WWFuzzyPrediction();
	fuzzy_inference->initialiseFuzzy();	

	tag_detector.initialiseTagDetector();
	
	while (c == 'y')
	{		
		tag_detector.detectTags();
		tags_list.clear();
		tag_detector.getDetectedTagIds(tags_list);
		printTagsHardCode(clothes_database, tags_list);
		//getInput(fuzzy_inference);
		getInputHardCode(fuzzy_inference);
    	fuzzy_inference->startFuzzy();
    	displayOutput(fuzzy_inference);
    	displayClothes(fuzzy_inference, clothes_database);
    	//std::cout << "Do you want a new selection? (y/n): "; 
    	std::cin >> c;
    }

    return 0;
}