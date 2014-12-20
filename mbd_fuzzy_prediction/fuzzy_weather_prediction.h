// I HAVE NOTHING TO WEAR! GREAT!

/**
 ********************************************************************************************************
 * @file    fuzzy_weather_prediction.h
 * @brief   WWFuzzyPrediction class
 * @details Weather based clothes prediction using a Mamdani Fuzzy Inference Engine
 ********************************************************************************************************
 */

#include "fl/Headers.h"
#include "apriltag_detector.h"
#include "utility"
#include "string"
#include "cstring"

using namespace fl;

enum Inputs
{
	TEMPERATURE = 0,
	RAINFALL,
	SNOWFALL,
	GENDER,
	WIND
};

enum Outputs
{
	SHIRT = 10,
	PANT,
	JACKET
};

class WWFuzzyPrediction
{
	protected:
		Engine*				engine_;
		InputVariable*		temperature_;
		InputVariable*		rainfall_;
		InputVariable*		snowfall_;
		InputVariable*		gender_;
		InputVariable*		wind_;
		OutputVariable*		shirt_;
		OutputVariable*		pant_;
		OutputVariable*		jacket_;
		RuleBlock*			rule_block_;
		std::string			status_;

	public:
		WWFuzzyPrediction();
		~WWFuzzyPrediction();
		void initialiseFuzzy();
		void startFuzzy();

		bool setInput(Inputs input, double &value);
		bool getOutput(Outputs output, double &value);
		
};

typedef WWFuzzyPrediction*	WWFuzzyPredictionPtr;

void getInput(WWFuzzyPredictionPtr&);
void displayOutput(WWFuzzyPredictionPtr &fuzzy_inference);
void displayClothes(WWFuzzyPredictionPtr &fuzzy_inference,
					std::vector< std::pair<int, std::string> > &clothes_database);
void getInputHardCode(WWFuzzyPredictionPtr &fuzzy_inference);
void printTagsCorrespondingClothes(std::vector< std::pair<int, std::string> > &clothes_database,
                                   std::vector<int> &tags_list);
void getClothesInCloset(std::vector<std::pair<int, std::string> > &clothes_database, std::vector<int> &tags_list);
