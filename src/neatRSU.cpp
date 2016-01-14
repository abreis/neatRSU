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

float gm_compat_excess 	= 1.0;
float gm_compat_disjoint = 1.0;
float gm_compat_weight 	= 0.4;	// flip to 3.0 for a larger population (e.g. 1000)

boost::random::mt19937 						g_rng;
boost::random::bernoulli_distribution<> 	g_rnd_5050;
boost::random::bernoulli_distribution<> 	g_rnd_inheritDisabled;
boost::random::bernoulli_distribution<> 	g_rnd_perturbOrNew;
boost::random::normal_distribution<> 		g_rnd_gauss;


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
	string 		m_traindata			= "";
	string 		m_testdata 			= "";
	int 		m_seed				= 0;
	uint32_t 	m_genmax			= 1;
	uint16_t 	m_maxPop			= 150;

	// Non-CLI-configurable options
	float 	m_p_weight_perturb_or_new 	= 0.90;
	float	m_p_inherit_disabled		= 0.75;

	float 	m_p_mutate_weights 			= 0.80;
	float 	m_p_mutate_addnode			= 0.03;
	float 	m_p_mutate_addconn			= 0.05; // use 0.30 for large population size
	// float	m_survival				= 0.20;

	// List of command line options
	boost::program_options::options_description cliOptDesc("Options");
	cliOptDesc.add_options()
		("train-data", 	boost::program_options::value<string>(), 	"location of training data CSV")
		("test-data", 	boost::program_options::value<string>(), 	"location of test data CSV")
		("seed", 		boost::program_options::value<int>(), 		"random number generator seed")
		("generations", boost::program_options::value<uint32_t>(),	"number of generations to perform")
		("population-size", 	boost::program_options::value<uint16_t>(),	"maximum population size")
		("compat-excess",		boost::program_options::value<float>(),		"compatibility weight c1")
		("compat-disjoint",		boost::program_options::value<float>(),		"compatibility weight c2")
		("compat-weight",		boost::program_options::value<float>(),		"compatibility weight c3")
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
	if (varMap.count("train-data"))				m_traindata		= varMap["train-data"].as<string>();
	if (varMap.count("test-data"))				m_testdata		= varMap["test-data"].as<string>();
	if (varMap.count("seed"))					m_seed			= varMap["seed"].as<int>();
	if (varMap.count("generations"))			m_genmax		= varMap["generations"].as<uint32_t>();
	if (varMap.count("population-size"))		m_maxPop			= varMap["population-size"].as<uint16_t>();
	if (varMap.count("compat-excess"))			gm_compat_excess 	= varMap["compat-excess"].as<float>();
	if (varMap.count("compat-disjoint"))		gm_compat_disjoint 	= varMap["compat-disjoint"].as<float>();
	if (varMap.count("compat-weight"))			gm_compat_weight 	= varMap["compat-weight"].as<float>();
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
	g_rng.seed(m_seed);

	// Boolean (bernoulli) distributions
	g_rnd_5050.param( 				boost::random::bernoulli_distribution<>::param_type(0.5) );
	g_rnd_inheritDisabled.param( 	boost::random::bernoulli_distribution<>::param_type(m_p_inherit_disabled) );
	g_rnd_perturbOrNew.param(		boost::random::bernoulli_distribution<>::param_type(m_p_weight_perturb_or_new) );

	// Source of Gaussian randomness (for weight mutations)
	// Inits: (mean,stdev)
	g_rnd_gauss.param( boost::random::normal_distribution<>::param_type( 0.0, 1.0 ) );

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
	firstSpecies.genomes.push_back( Genome(g_inputs) );
	population.species.push_back(firstSpecies);

	// TODO: weird things may happen if the first genome's weights don't get randomized.


	/***
	 *** C0 Loop evolution until criteria match
	 ***/


	uint32_t generationNumber = 0;
	do
	{
		// Go through each species
		for(vector<Species>::iterator 
			iterSpecies = population.species.begin();
			iterSpecies != population.species.end();
			iterSpecies++)
		{
			// Go through each genome in this species
			for(vector<Genome>::iterator
				iterGenome = iterSpecies->genomes.begin();
				iterGenome != iterSpecies->genomes.end();
				iterGenome++)
			{
				// TODO: clone genome, mutate clone, push clone to species?

				// Mutate Perturb Weights?
				if(OneShotBernoulli(m_p_mutate_weights))
					iterGenome->MutatePerturbWeights();

				// Mutate Add Node?
				if(OneShotBernoulli(m_p_mutate_addnode));
					iterGenome->MutateAddNode();

				// Mutate Add Connection?
				if(OneShotBernoulli(m_p_mutate_addconn));
					iterGenome->MutateAddConnection();

			} // END GENOME ITERATION

			// Perform intra-species mating

		} // END SPECIES ITERATION


	/* Generation end post-processing
	 */


	// Generation loop control
	generationNumber++;
	} while( generationNumber < m_genmax );	// Specify stopping criteria here




	/***
	 *** Z0 Wrap up
	 ***/



	return 0;
}

bool OneShotBernoulli(float probability)
{
	boost::random::bernoulli_distribution<> oneShot(probability);
	return oneShot(g_rng);
}


// // Push a DataEntry through it
// // cout << "Activation " << generationNumber << ": " << gentest.Activate( *(TrainingDB.begin()+generationNumber) ) << endl;

// // Push the whole DB through
// cout << "Activation " << generationNumber << ", fitness: " << gentest.GetFitness(&TrainingDB) << endl;

// // Mutate add node
// gentest.MutateAddNode();
// // population.species.begin()->genomes.begin()->MutateAddNode();

// // Mutate add connection
// gentest.MutateAddConnection(rng_gauss);

// // Print
// string filename = "gentest" + to_string(generationNumber) + ".gv";
// gentest.PrintToGV(filename);




// // Test: create two genomes and mate them
// Genome gen1(g_inputs);
// Genome gen2(g_inputs);

// // Mutate them a bit
// gen1.MutatePerturbWeights();
// gen1.MutateAddNode();
// gen1.MutateAddConnection();
// gen1.MutateAddNode();

// gen2.MutatePerturbWeights();
// gen2.MutateAddNode();
// gen2.MutateAddConnection();
// gen2.MutateAddNode();
// gen2.MutateAddConnection();
// gen2.MutateAddNode();

// gen1.fitness = gen1.GetFitness(&TrainingDB);
// gen2.fitness = gen2.GetFitness(&TrainingDB);

// Genome gen3 = MateGenomes(&gen1, &gen2);

// cout << "\nFirst parent\n";
// gen1.Print();
// gen1.PrintToGV("gen1.gv");

// cout << "\nSecond parent\n";
// gen2.Print();
// gen2.PrintToGV("gen2.gv");

// cout << "\nOffspring\n";
// gen3.Print();
// gen3.PrintToGV("gen3.gv");