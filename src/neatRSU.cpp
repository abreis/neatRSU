#include "neatRSU.h"
#include "genetic.h"

// Debug flag
bool gm_debug = false;

// Global generation counter
uint32_t g_generationNumber = 0;

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

float gm_compat_excess 		= 1.0;
float gm_compat_disjoint 	= 1.0;
float gm_compat_weight 		= 0.4;	// flip to 3.0 for a larger population (e.g. 1000)

float g_m_p_mutate_weights 			= 0.80;
float g_m_p_mutate_addnode			= 0.03;
float g_m_p_mutate_addconn			= 0.05; // use 0.30 for large population size
float g_m_p_weight_perturb_or_new 	= 0.90;
float g_m_p_inherit_disabled		= 0.75;

boost::random::mt19937 						g_rng;
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

	bool 		m_printPopulation		= false;
	string 		m_printPopulationFile 	= "";
	string 		m_printSpeciesStackFile = "";
	string 		m_printFitnessFile		= "";

	// Non-CLI-configurable options

	float	m_survival_threshold		= 0.20;
	float	m_compat_threshold			= 3.00;

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
		("print-population", 													"print population statistics")
		("print-population-file", 	boost::program_options::value<string>(), 	"print population statistics to a file")
		("print-speciesstack-file", boost::program_options::value<string>(), 	"print graph of species size to a file")
		("print-fitness-file", 		boost::program_options::value<string>(), 	"print best fitness to a file")
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

	if (varMap.count("print-population"))			m_printPopulation 		= true;
	if (varMap.count("print-population-file"))		m_printPopulationFile 	= varMap["print-population-file"].as<string>();
	if (varMap.count("print-speciesstack-file"))	m_printSpeciesStackFile = varMap["print-speciesstack-file"].as<string>();
	if (varMap.count("print-fitness-file"))			m_printFitnessFile 		= varMap["print-fitness-file"].as<string>();

	if (varMap.count("help")) 					{ cout << cliOptDesc; return 1; }


	/***
	 *** A1 Load training data
	 ***/


	// Database to store training data
	vector<DataEntry> TrainingDB;

	if(m_traindata.empty())
		{ cout << "ERROR\tPlease specify a file with training data." << endl; exit(1); }
	else
	{
	 	cout << "INFO\tLoading training data on " << m_traindata << " into memory... " << flush;

		ifstream trainDataIn( m_traindata.c_str() );
		if (!trainDataIn.is_open()) { cout << "\nERROR\tFailed to open file." << endl; exit(1); }

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

		// Sort TrainingData by NodeID, then Time.
		sort(TrainingDB.begin(), TrainingDB.end(), sortIdThenTime);
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
		if (!testDataIn.is_open()) { cout << "\nERROR\tFailed to open file." << endl; exit(1); }


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

		// Sort TestDB by NodeID, then Time.
		sort(TestDB.begin(), TestDB.end(), sortIdThenTime);
	}


	/***
	 *** A3 Initialize global Random Number Generators
	 ***/

	cout << "INFO\tInitializing random number generators with seed " << m_seed << ".\n";

	// We use boost::random and mt19937 as a source of randomness.
	// The seed can be specified thorugh CLI
	g_rng.seed(m_seed);

	// Source of Gaussian randomness (for weight mutations)
	// Inits: (mean,stdev)
	g_rnd_gauss.param( boost::random::normal_distribution<>::param_type( 0.0, 1.0 ) );

	// To draw random integers (e.g. to randomly select a node), use:
	// boost::random::uniform_int_distribution<> dist(min, max);


	/***
	 *** B0 Setup neural network
	 ***/

	/* Set global number of inputs and input names at the start of the file */

	// A counter for species IDs
	static uint16_t g_newSpeciesId = 0;

	// Create a population
	Population* population = new Population();

	// Create the first species. First ID, generation=0.
	Species firstSpecies(++g_newSpeciesId, g_generationNumber);

	// Push the first genome
	firstSpecies.genomes.push_back( Genome(g_inputs) );
	population->species.push_back(firstSpecies);


	/***
	 *** C0 Loop evolution until criteria match
	 ***/


	do
	{
		if(gm_debug) cout << "DEBUG Generation " << g_generationNumber << endl;
	
		/* Generation loop initial setup
		 */ 
		double l_totalFitness = 0.0;
		double l_meanModifiedFitness = 0.0;

		/* Iterate for mutations
		 */

		// Go through each species
		for(vector<Species>::iterator 
			iterSpecies = population->species.begin();
			iterSpecies != population->species.end();
			iterSpecies++)
		{
			// Go through each genome in this species.
			// Using a reverse iterator lets us avoid processing new genomes we tack on.
			for(vector<Genome>::reverse_iterator
				iterGenome = iterSpecies->genomes.rbegin();
				iterGenome != iterSpecies->genomes.rend();
				iterGenome++)
			{


			} // END GENOME ITERATION (MUTATION)

			// Compute and store the fitness of each Genome
			for(vector<Genome>::iterator
				iterGenome = iterSpecies->genomes.begin();
				iterGenome != iterSpecies->genomes.end();
				iterGenome++)
				iterGenome->fitness = iterGenome->GetFitness(&TrainingDB);

			// TODO Convert fitness to adjustedFitness




			/* Eliminate the lowest performing members from the population.
			 */

			// Sort the vector of Genomes by fitness.
			sort(iterSpecies->genomes.begin(), iterSpecies->genomes.end());
			// Sorting puts the lowest value, and therefore the highest fitness, last.
			// Determine how many Genomes we're pop-ing out, based on survival metric.
			uint16_t noSurvivors = iterSpecies->genomes.size() * m_survival_threshold;
			if(gm_debug) cout << "DEBUG Killing " << noSurvivors << " from species id " << iterSpecies->id << " size " << iterSpecies->genomes.size() << '\n';

			// Trim genome list
			if(noSurvivors>0)
				for(uint16_t killCount = 0; killCount < noSurvivors; killCount++)
					iterSpecies->genomes.pop_back();


			/* TODO Perform intra-species mating
			 */

			// Determine how many offspring we can have
			// TODO use a ratio of sum(adjustedFitness)
			// May need to do a separate Species cycle where all the fitness is known.
			// TODO fitness is "lowest is best", so the smallest sum of fitness is the best species
			uint16_t newSpeciesPopulation = m_maxPop * 1;

			// Reproduce
			iterSpecies->Reproduce(newSpeciesPopulation);


		} // END SPECIES ITERATION

		/* Speciation
		 */

			// TODO need an updateSpecies routine that updates the best fitness of all species and keeps track of last time fitness improved.

		// Create a new Population with all of the species, but with a single champion genome on each species.
		// Run through all Genomes on all species, matching their compatibility to the champion of each species.
		// Champions are always the first genome in the vector<species>, species.begin()
		// Create a new species if compatibility>m_compat_threshold for all existing species
		// If compatibility == 0, do nothing, it's a clone or the champion itself.
		// Clear out empty species (how do species extinguish themselves?)
		// Replace main Population* pointer. Delete old Population.

		// TODO may need to use species' ages to boost adjFitness to allow young species to take hold. lookfor species::adjust_fitness()


		/* Generation end post-processing
		 */


		if(m_printPopulation)
			population->PrintSummary(cout);

		if(!m_printPopulationFile.empty()) 
		{
			static ofstream ofPopSummary(m_printPopulationFile.c_str());
			population->PrintSummary(ofPopSummary);
		}

		if(!m_printSpeciesStackFile.empty())
		{
			static ofstream ofSpeciesStack(m_printSpeciesStackFile.c_str());
			population->PrintVerticalSpeciesStack(ofSpeciesStack);
		}

		if(!m_printFitnessFile.empty())
		{
			static ofstream ofFitness(m_printFitnessFile.c_str());
			population->PrintFitness(ofFitness);
		}


	// Generation loop control
	g_generationNumber++;
	} while( g_generationNumber < m_genmax );	// Specify stopping criteria here




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


bool sortIdThenTime( DataEntry const &first, DataEntry const &second )
{
	if(first.node_id < second.node_id)
		return true;
	else
		if(first.node_id > second.node_id)
			return false;
		else 
			if(first.relative_time < second.relative_time)
				return true;
			else 
				return false;
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




// Species s2(++g_newSpeciesId, g_generationNumber);
// s2.genomes.push_back( Genome(g_inputs) );
// s2.genomes.push_back( Genome(g_inputs) );
// population->species.push_back(s2);