#include "neatRSU.h"
#include "genetic.h"

// Debug flag
bool gm_debug = false;

/* Number of inputs in the system and node names
 * g_inputs+1 -> output node
 * g_inputs+2 -> bias node
 * g_inputs+3 -> first hidden node
 */ 
const uint16_t g_inputs = 6;
map<uint16_t,string> g_nodeNames = 
{
	{1, "id"},
	{2, "time"},
	{3, "lat"},
	{4, "lon"},
	{5, "speed"},
	{6, "bearing"},
	{7, "output"},
	{8, "bias"}
};


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
	int 	m_seed			= 0;

	// Non-CLI-configurable options
	float 	m_p_mutate_addweight	= 0.50;
	float 	m_p_mutate_addconn		= 0.01;
	float 	m_p_mutate_addnode		= 0.01;
	// float	m_survival				= 0.20;

	// List of command line options
	boost::program_options::options_description cliOptDesc("Options");
	cliOptDesc.add_options()
		("train-data", 	boost::program_options::value<string>(), 	"location of training data CSV")
		("test-data", 	boost::program_options::value<string>(), 	"location of test data CSV")
		("seed", 		boost::program_options::value<int>(), 		"random number generator seed")
	    ("debug", 													"enable debug mode")
	    ("help", 													"give this help list")
	;

	// Parse options
	boost::program_options::variables_map varMap;
	store(parse_command_line(argc, argv, cliOptDesc), varMap);
	notify(varMap);

	if(argc==1) { cout << cliOptDesc; return 1; }

	// Process options
	if (varMap.count("debug")) 					gm_debug		= true;
	if (varMap.count("train-data"))				m_traindata	= varMap["train-data"].as<string>();
	if (varMap.count("test-data"))				m_testdata	= varMap["test-data"].as<string>();
	if (varMap.count("seed"))					m_seed		= varMap["seed"].as<int>();
	if (varMap.count("help")) 					{ cout << cliOptDesc; return 1; }


	/***
	 *** A1 Load training data
	 ***/


	// Database to store training data
	vector<DataEntry> TrainingDB;

	if(m_traindata.empty())
		{ cout << "ERROR\tPlease specify a file with training data." << endl; return 1; }
	else
	{
	 	cout << "INFO\tLoading training data on " << m_traindata << " into memory... " << flush;

		ifstream trainDataIn( m_traindata.c_str() );
		if (!trainDataIn.is_open()) { cout << "\nERROR\tFailed to open file." << endl; return 1; }

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

		trainDataIn.close();
		cout << "done." << endl;
		cout << "INFO\tLoaded " << TrainingDB.size() << " training entries into memory." << endl;
	}


	/***
	 *** A2 Load test data
	 ***/


	// Database to store training data
	vector<DataEntry> TestDB;

	// Check if a test data file was specified in the options.
	if(m_testdata.empty())
		{ cout << "INFO\tNo test data provided, evaluation to be performed on training data only." << endl; }
	else
	{
		// Load data
	 	cout << "INFO\tLoading test data on " << m_testdata << " into memory... " << flush;

		ifstream testDataIn( m_testdata.c_str() );
		if (!testDataIn.is_open()) { cout << "\nERROR\tFailed to open file." << endl; return 1; }


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

		testDataIn.close();
		cout << "done." << endl;
		cout << "INFO\tLoaded " << TestDB.size() << " testing entries into memory." << endl;

	}


	/***
	 *** A3 Initialize global Random Number Generators
	 ***/

	cout << "INFO\tInitializing random number generators with seed " << m_seed << ".\n";

	// We use boost::random and mt19937 as a source of randomness.
	// The seed can be specified thorugh CLI
	boost::random::mt19937 rng(m_seed);

	// Boolean (bernoulli) distributions
	boost::random::bernoulli_distribution<> rng_5050		( 0.5 );
	boost::random::bernoulli_distribution<> rng_mutweight	( m_p_mutate_addweight );
	boost::random::bernoulli_distribution<> rng_addconn		( m_p_mutate_addconn );
	boost::random::bernoulli_distribution<> rng_addnode		( m_p_mutate_addnode );

	// Source of Gaussian randomness (for weight mutations)
	// Inits: (mean,stdev)
	boost::random::normal_distribution<> rng_gauss			( 0.0, 1.0 );

	// To draw random integers (e.g. to randomly select a node), use:
	// boost::random::uniform_int_distribution<> dist(min, max);


	/***
	 *** B0 Setup neural network
	 ***/

	/* Set global number of inputs and input names at the start of the file */

	// Create a population
	Population population;

	// Create the first species
	Species firstSpecies;

	// Push the first genome
	Genome firstGenome(g_inputs);
	firstSpecies.genomes.push_back(firstGenome);
	population.species.push_back(firstSpecies);

	// TODO test
	string outfile = "testgv.gv";
	Genome gentest(g_inputs);
	gentest.connections[make_pair(1,g_inputs+1)].enabled=false;
	gentest.nodes[g_inputs+3] = NodeGene(g_inputs, NodeType::HIDDEN);
	gentest.connections[make_pair(1,g_inputs+3)] = ConnectionGene(1, 6+3, g_inputs+1);
	gentest.connections[make_pair(g_inputs+3,g_inputs+1)] = ConnectionGene(g_inputs+3, g_inputs+1, g_inputs+2);
	gentest.connections[make_pair(g_inputs+2,g_inputs+3)] = ConnectionGene(g_inputs+2, g_inputs+3, g_inputs+3);
	gentest.PrintToGV(gentest, outfile);

	/***
	 *** C0 Loop evolution until criteria match
	 ***/


	#define GENMAX 100 	// number of generations to try
	uint32_t generations = 0;
	do
	{


		generations++;
	} while( generations<GENMAX );	// Specify stopping criteria here


	/***
	 *** Z0 Wrap up
	 ***/



	return 0;
}


