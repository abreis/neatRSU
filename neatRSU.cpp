#include "neatRSU.h"
#include "neural.h"
#include "genetic.h"

// Extern-able flags
bool m_debug = false;

int main(int argc, char *argv[])
{
	/* Overall structure:
	 * - Load data into memory, from CSV files, using Boost::tokenizer
	 * - Set up neural network scaffolds for genetic evolution
	 * - Evolve network on training data, until criteria met or user input
	 * - Output resulting network structure and weights
	 * - Test on both training data and test data, and output
	 */


	/***
	 *** A0 Process command-line options
	 ***/


	/* Process command-line options
	 * Using boost::program_options
	 */

	// Defaults
	string 	m_traindata		= "";
	string 	m_testdata 		= "";
	// float 	m_p_mutate_addweight	= 0.50;
	// float 	m_p_mutate_addconn		= 0.01;
	// float 	m_p_mutate_addnode		= 0.01;

	// List of command line options
	boost::program_options::options_description cliOptDesc("Options");
	cliOptDesc.add_options()
		("train-data", boost::program_options::value<string>(), "location of training data CSV")
		("test-data", boost::program_options::value<string>(), "location of test data CSV")
	    ("debug", "enable debug mode")
	    ("help", "give this help list")
	;

	// Parse options
	boost::program_options::variables_map varMap;
	store(parse_command_line(argc, argv, cliOptDesc), varMap);
	notify(varMap);

	if(argc==1) { cout << cliOptDesc; return 1; }

	// Process options
	if (varMap.count("debug")) 					m_debug		= true;
	if (varMap.count("train-data"))				m_traindata	= varMap["train-data"].as<string>();
	if (varMap.count("test-data"))				m_testdata	= varMap["test-data"].as<string>();
	if (varMap.count("help")) 					{ cout << cliOptDesc; return 1; }


	/***
	 *** A1 Load training data
	 ***/


	if(m_traindata.empty())
		{ cout << "ERROR: Please specify a file with training data." << endl; return 1; }
	else
	{
	 	cout << "Loading training data on " << m_traindata << " into memory... " << flush;

		ifstream trainDataIn( m_traindata.c_str() );
		if (!trainDataIn.is_open()) { cout << "\nERROR: Failed to open file." << endl; return 1; }

		// Database to store training data
		vector<DataEntry> TrainingDB;

		// Setup boost::tokenizer
		vector<string> fields; string line;
		while (getline(trainDataIn,line))
		{
			// Tokenize the line into 'fields'
			boost::tokenizer< boost::escaped_list_separator<char> > tok(line);
			fields.assign(tok.begin(),tok.end());

	        // Create an entry from the tokenized string vector
	        // DataEntry constructor automatically converts strings to the correct types 
			DataEntry entry(fields);
			TrainingDB.push_back(entry);
		}
		cout << "done." << endl;
		cout << "Loaded " << TrainingDB.size() << " entries into memory." << endl;
	}

	/***
	 *** A2 Load test data
	 ***/


	// Check if a test data file was specified in the options.
	if(m_testdata.empty())
		{ cout << "No test data provided, evaluation to be performed on training data." << endl; }
	else
	{
		// Load data
	 	cout << "Loading test data on " << m_testdata << " into memory... " << flush;

		ifstream testDataIn( m_testdata.c_str() );
		if (!testDataIn.is_open()) { cout << "\nERROR: Failed to open file." << endl; return 1; }

		// Database to store training data
		vector<DataEntry> TestDB;

		// Setup boost::tokenizer
		vector<string> fields; string line;
		while (getline(testDataIn,line))
		{
			// Tokenize the line into 'fields'
			boost::tokenizer< boost::escaped_list_separator<char> > tok(line);
			fields.assign(tok.begin(),tok.end());

	        // Create an entry from the tokenized string vector
	        // DataEntry constructor automatically converts strings to the correct types 
			DataEntry entry(fields);
			TestDB.push_back(entry);
		}
		cout << "done." << endl;
		cout << "Loaded " << TestDB.size() << " entries into memory." << endl;

	}


	/***
	 *** B0 Setup neural network
	 ***/

	//


	/***
	 *** Z0 Wrap up
	 ***/



	return 0;
}