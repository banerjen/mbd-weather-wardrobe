// I HAVE NOTHING TO WEAR! GREAT!

#include "fl/Headers.h"

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

void WWFuzzyPrediction::startFuzzy()
{
	engine_->process();
}

bool WWFuzzyPrediction::setInput(Inputs input, double &value)
{
	try 
	{
		switch (input)
		{
			case TEMPERATURE:
				temperature_->setInputValue(value);
				break;
			case RAINFALL:
				rainfall_->setInputValue(value);
				break;
			case SNOWFALL:
				snowfall_->setInputValue(value);
				break;
			case GENDER:
				gender_->setInputValue(value);
				break;
			case WIND:
				wind_->setInputValue(value);
				break;
			default:
				std::cerr << "Could not find specified input variable.";
				return false;
		}
	}
	catch (...)
	{
		std::cerr << "WWFuzzyPrediction::setInput() error:\n"
					 "Could not set the specified input variable.";
		return false;
	}

	return true;
}

bool WWFuzzyPrediction::getOutput(Outputs output, double &value)
{
	try
	{
		switch (output)
		{
			case SHIRT:
				value = shirt_->getOutputValue();
				break;
			case PANT:
				value = pant_->getOutputValue();
				break;
			case JACKET:
				value = jacket_->getOutputValue();
				break;
			default:
				std::cerr << "Could not find specified output variable.";
				return false;		
		}
	}
	catch (...)
	{
		std::cerr << "WWFuzzyPrediction::getOutput() error:\n"
					 "Could not get the specified output variable.";
		return false;
	}

	return true;
}

WWFuzzyPrediction::WWFuzzyPrediction()
{
	// Constructor code goes here
}

WWFuzzyPrediction::~WWFuzzyPrediction()
{	
	delete engine_;
	delete temperature_;
	delete rainfall_;
	delete snowfall_;
	delete gender_;
	delete wind_;
	delete shirt_;
	delete pant_;
	delete jacket_;
	delete rule_block_;
}

void WWFuzzyPrediction::initialiseFuzzy()
{
	try
	{
		engine_ = new Engine;
		engine_->setName("WWFuzzyPrediction_Engine");

		temperature_ = new InputVariable;
	    temperature_->setEnabled(true);
	    temperature_->setName("Temperature");
	    temperature_->setRange(-60.000, 125.000);       // setting the range of inputs to deal with
	    temperature_->addTerm(new Triangle("freezing", -80.000, -60.000, 20.000));   // setting individual range for each membership function
	    temperature_->addTerm(new Triangle("cold", 10.000, 27.500, 45.000));
	    temperature_->addTerm(new Triangle("pleasant", 40.000, 55.000, 70.000));
	    temperature_->addTerm(new Triangle("warm", 65.000, 75.000, 85.000));
	    temperature_->addTerm(new Triangle("hot", 80.000, 125.000, 140.000));
	    engine_->addInputVariable(temperature_);

	    rainfall_ = new InputVariable;
	    rainfall_->setEnabled(true);
	    rainfall_->setName("Rainfall");
	    rainfall_->setRange(0.000, 90.000);
	    rainfall_->addTerm(new Triangle("no_rain", -0.100, 0.000, 0.100));
	    rainfall_->addTerm(new Triangle("light", 0.075, 12.500, 25.000));
	    rainfall_->addTerm(new Triangle("moderate", 20.000, 35.000, 50.000));
	    rainfall_->addTerm(new Triangle("heavy", 45.000, 60.000, 75.000));
	    rainfall_->addTerm(new Triangle("storm", 70.000, 90.000, 110.000));
	    engine_->addInputVariable(rainfall_);

	    snowfall_ = new InputVariable;
	    snowfall_->setEnabled(true);
	    snowfall_->setName("Snowfall");
	    snowfall_->setRange(0.000, 115.000);
	    snowfall_->addTerm(new Triangle("no_snow", -0.100, 0.000, 0.100));
	    snowfall_->addTerm(new Triangle("light", 0.075, 7.500, 15.000));
	    snowfall_->addTerm(new Triangle("moderate", 10.000, 25.000, 40.000));
	    snowfall_->addTerm(new Triangle("heavy", 35.000, 60.000, 85.000));
	    snowfall_->addTerm(new Triangle("storm", 80.000, 115.000, 150.000));
	    engine_->addInputVariable(snowfall_);

	    gender_ = new InputVariable;
	    gender_->setEnabled(true);
	    gender_->setName("Gender");
	    gender_->setRange(0.000, 1.000);
	    gender_->addTerm(new Triangle("male", -0.100, 0.200, 0.500));
	    gender_->addTerm(new Triangle("female", 0.500, 0.800, 1.100));
	    engine_->addInputVariable(gender_);

	    wind_ = new InputVariable;
	    wind_->setEnabled(true);
	    wind_->setName("Wind");
	    wind_->setRange(0.000, 65.000);
	    wind_->addTerm(new Triangle("no_wind", -0.100, 0.000, 0.100));
	    wind_->addTerm(new Triangle("light", 0.075, 10.000, 20.000));
	    wind_->addTerm(new Triangle("moderate", 15.000, 32.500, 50.000));
	    wind_->addTerm(new Triangle("strong", 45.000, 65.000, 85.000));
	    engine_->addInputVariable(wind_);

	    shirt_ = new OutputVariable;
	    shirt_->setEnabled(true);
	    shirt_->setName("Shirt");
	    shirt_->setRange(0.000, 1.000);
	    shirt_->fuzzyOutput()->setAccumulation(new Maximum);        // setting computation methods fo matching i/p to o/p
	    shirt_->setDefuzzifier(new Centroid(200));                  // using centroid  of max to calculate o/p
	    shirt_->setDefaultValue(fl::nan);
	    shirt_->setLockPreviousOutputValue(false);
	    shirt_->setLockOutputValueInRange(false);
	    shirt_->addTerm(new Triangle("half_sleeve", -0.100, 0.000, 0.300));
	    shirt_->addTerm(new Triangle("full_sleeve", 0.250, 0.375, 0.500));
	    shirt_->addTerm(new Triangle("full_sleeve_&_thermal", 0.450, 0.600, 0.750));
	    shirt_->addTerm(new Triangle("2_shirts", 0.700, 1.000, 1.100));
	    engine_->addOutputVariable(shirt_);

	    pant_ = new OutputVariable;
	    pant_->setEnabled(true);
	    pant_->setName("Pant");
	    pant_->setRange(0.000, 1.000);
	    pant_->fuzzyOutput()->setAccumulation(new Maximum);
	    pant_->setDefuzzifier(new Centroid(200));
	    pant_->setDefaultValue(fl::nan);
	    pant_->setLockPreviousOutputValue(false);
	    pant_->setLockOutputValueInRange(false);
	    pant_->addTerm(new Triangle("shorts", -0.100, 0.000, 0.250));
	    pant_->addTerm(new Triangle("casual_pant", 0.200, 0.350, 0.500));
	    pant_->addTerm(new Triangle("pants_thermal", 0.450, 0.600, 0.850));
	    pant_->addTerm(new Triangle("jeans_thermal", 0.800, 1.000, 1.200));
	    engine_->addOutputVariable(pant_);

	    jacket_ = new OutputVariable;
	    jacket_->setEnabled(true);
	    jacket_->setName("Jacket");
	    jacket_->setRange(0.000, 1.000);
	    jacket_->fuzzyOutput()->setAccumulation(new Maximum);
	    jacket_->setDefuzzifier(new Centroid(200));
	    jacket_->setDefaultValue(fl::nan);
	    jacket_->setLockPreviousOutputValue(false);
	    jacket_->setLockOutputValueInRange(false);
	    jacket_->addTerm(new Triangle("no_jacket", -0.100, 0.000, 0.200));
	    jacket_->addTerm(new Triangle("light_jacket", 0.180, 0.250, 0.350));
	    jacket_->addTerm(new Triangle("coat", 0.300, 0.475, 0.650));
	    jacket_->addTerm(new Triangle("heavy_jacket", 0.600, 0.800, 1.000));
	    engine_->addOutputVariable(jacket_);	    

	    rule_block_ = new RuleBlock;	    
	    rule_block_->setEnabled(true);	   
	    rule_block_->setName("");
	    rule_block_->setConjunction(new Minimum); // computation is calculated using max(min(i/p's))
	    rule_block_->setDisjunction(new Maximum);
	    rule_block_->setActivation(new Minimum);	    
	    rule_block_->addRule(fl::Rule::parse("if Temperature is freezing then Shirt is 2_shirts and Pant is jeans_thermal and Jacket is heavy_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is hot and Rainfall is no_rain and Snowfall is no_snow and Gender is male and Wind is no_wind then Shirt is half_sleeve and Pant is shorts and Jacket is no_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is hot and Rainfall is no_rain and Snowfall is no_snow and Gender is female and Wind is no_wind then Shirt is half_sleeve and Pant is shorts and Jacket is no_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is cold and Rainfall is no_rain and Snowfall is heavy then Shirt is 2_shirts and Pant is jeans_thermal and Jacket is heavy_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Snowfall is storm then Shirt is 2_shirts and Pant is jeans_thermal and Jacket is heavy_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Rainfall is storm and Wind is moderate then Shirt is full_sleeve_&_thermal and Pant is pants_thermal and Jacket is coat", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is cold and Rainfall is moderate and Snowfall is moderate then Shirt is full_sleeve_&_thermal and Pant is pants_thermal and Jacket is light_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is cold and Rainfall is light and Snowfall is no_snow and Wind is moderate then Shirt is full_sleeve and Pant is casual_pant and Jacket is light_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is pleasant and Rainfall is light and Snowfall is no_snow and Wind is light then Shirt is full_sleeve and Pant is casual_pant and Jacket is light_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is pleasant and Rainfall is no_rain and Snowfall is light and Wind is light then Shirt is full_sleeve and Pant is casual_pant and Jacket is light_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is pleasant and Rainfall is no_rain and Snowfall is no_snow and Wind is strong then Shirt is full_sleeve and Pant is pants_thermal and Jacket is coat", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is cold and Rainfall is no_rain and Snowfall is no_snow and Wind is no_wind then Shirt is full_sleeve and Pant is pants_thermal and Jacket is heavy_jacket", engine_));
	    rule_block_->addRule(fl::Rule::parse("if Temperature is pleasant and Rainfall is no_rain and Snowfall is no_snow and Wind is strong then Shirt is full_sleeve and Pant is pants_thermal and Jacket is coat", engine_));
	    engine_->addRuleBlock(rule_block_);		

	    if (not engine_->isReady(&status_))
	    	throw Exception("Engine not ready."
	    					"The following errors were encountered:\n" + status_, FL_AT);
	}
	// catch (std::Exception &ex)
	// {
	// 	std::cout << "Fuzzy prediction system could not be initialised. " << ex.what(); 
	// }
	catch (...)
	{
		std::cout << "Fuzzy prediction system could not be initialised.";
	}
}
