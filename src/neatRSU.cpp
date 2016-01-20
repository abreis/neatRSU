#include "neatRSU.h"
#include "genetic.h"

// Debug flag
uint16_t gm_debug = 0;

// Global generation counter
uint32_t g_generationNumber = 0;

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
};/* Number of inputs in the system and node names
 * g_inputs+1 -> output node
 * g_inputs+2 -> bias node
 * g_inputs+3 -> first hidden node
 */
uint16_t d_outputnode = g_inputs+1;
uint16_t d_biasnode = g_inputs+2;
uint16_t d_firsthidnode = g_inputs+3;


float gm_compat_excess 		= 1.0;
float gm_compat_disjoint 	= 1.0;
float gm_compat_weight 		= 0.4;	// flip to 3.0 for a larger population (e.g. 1000)
bool  gm_limitInitialGrowth = false;

float g_m_p_mutate_weights 			= 0.80;
float g_m_p_weight_perturb_or_new 	= 0.90;
float g_m_p_mutate_addnode			= 0.03;
float g_m_p_mutate_addconn			= 0.05; // use 0.30 for large population size
float g_m_p_mutate_disnode			= 0.01;
float g_m_p_mutate_disconn			= 0.02;
float g_m_p_inherit_disabled		= 0.75;
float g_m_p_mutateOnly				= 0.25;
float g_m_p_mateOnly 				= 0.20;

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
	uint16_t	m_threads				= 2;
	string 		m_traindata				= "";
	string 		m_testdata 				= "";
	string 		m_genomeFile			= "";
	bool		m_testGenome			= false;
	int 		m_seed					= 0;
	uint32_t 	m_genmax				= 1;
	uint16_t 	m_maxPop				= 150;
	bool 		m_bestCompat			= false;
	bool		m_seedGenome			= false;
	bool 		m_printPopulation		= false;
	string 		m_printPopulationFile 	= "";
	string 		m_printSpeciesStackFile = "";
	string 		m_printSpeciesSizeFile  = "";
	string 		m_printFitnessFile		= "";
	bool		m_printSuperChampions	= false;
	float		m_weightPerturbStdev	= FLT_MAX;
	uint32_t	m_killStagnated			= 0;
	uint32_t	m_refocusStagnated		= 0;
	uint16_t	m_targetSpecies			= 0;

	// Non-CLI-configurable options
	float	m_survival_threshold		= 0.20;
	float	m_compat_threshold			= 3.00;

	// List of command line options
	boost::program_options::options_description cliOptDesc("Options");
	cliOptDesc.add_options()
		("threads", 				boost::program_options::value<uint16_t>(),	"number of threads to run concurrently")
		("train-data", 				boost::program_options::value<string>(), 	"location of training data CSV")
		("test-data", 				boost::program_options::value<string>(), 	"location of test data CSV")
		("genome-file", 			boost::program_options::value<string>(), 	"load a genome from a CSV file")
		("seed-genome", 														"uses the loaded genome as the first seed")
		("test-genome", 														"runs the loaded genome on the databases")
		("seed", 					boost::program_options::value<int>(), 		"random number generator seed")
		("generations", 			boost::program_options::value<uint32_t>(),	"number of generations to perform")
		("population-size", 		boost::program_options::value<uint16_t>(),	"maximum population size")
		("compat-excess",			boost::program_options::value<float>(),		"compatibility weight c1")
		("compat-disjoint",			boost::program_options::value<float>(),		"compatibility weight c2")
		("compat-weight",			boost::program_options::value<float>(),		"compatibility weight c3")
		("perturb-stdev",			boost::program_options::value<float>(),		"standard deviation of gaussian perturb weights")
		("best-compat", 														"enable best compatibility speciation")
		("limit-growth", 														"limits initial growth to 2*size(species)")
		("kill-stagnated", 			boost::program_options::value<uint32_t>(),	"removes species that stagnate after N generations")
		("refocus-stagnated", 		boost::program_options::value<uint32_t>(),	"refocuses species that stagnate after N generations")
		("target-species",	 		boost::program_options::value<uint16_t>(),	"targets N species with a self-adjusting threshold")
		("print-population", 													"print population statistics")
		("print-population-file", 	boost::program_options::value<string>(), 	"print population statistics to a file")
		("print-speciesstack-file", boost::program_options::value<string>(), 	"print graph of species size to a file")
		("print-speciessize-file", 	boost::program_options::value<string>(), 	"print CSV-formatted sizes of species per generation")
		("print-fitness-file", 		boost::program_options::value<string>(), 	"print best fitness to a file")
		("print-super-champions", 											 	"print every super champion to a file")
	    ("debug", 					boost::program_options::value<uint16_t>(),	"enable debug mode")
	    ("help", 																"give this help list")
	;

	// Parse options
	boost::program_options::variables_map varMap;
	store(parse_command_line(argc, argv, cliOptDesc), varMap);
	notify(varMap);

	if(argc==1) { cout << cliOptDesc; return 1; }

	// Process options
	if (varMap.count("debug")) 					gm_debug					= varMap["debug"].as<uint16_t>();
	if (varMap.count("threads")) 				m_threads					= varMap["threads"].as<uint16_t>();
	if (varMap.count("train-data"))				m_traindata					= varMap["train-data"].as<string>();
	if (varMap.count("test-data"))				m_testdata					= varMap["test-data"].as<string>();
	if (varMap.count("genome-file"))			m_genomeFile				= varMap["genome-file"].as<string>();
	if (varMap.count("seed"))					m_seed						= varMap["seed"].as<int>();
	if (varMap.count("generations"))			m_genmax					= varMap["generations"].as<uint32_t>();
	if (varMap.count("population-size"))		m_maxPop					= varMap["population-size"].as<uint16_t>();
	if (varMap.count("compat-excess"))			gm_compat_excess 			= varMap["compat-excess"].as<float>();
	if (varMap.count("compat-disjoint"))		gm_compat_disjoint 			= varMap["compat-disjoint"].as<float>();
	if (varMap.count("compat-weight"))			gm_compat_weight 			= varMap["compat-weight"].as<float>();
	if (varMap.count("best-compat")) 			m_bestCompat				= true;
	if (varMap.count("perturb-stdev"))			m_weightPerturbStdev 		= varMap["perturb-stdev"].as<float>();
	if (varMap.count("limit-growth")) 			gm_limitInitialGrowth		= true;
	if (varMap.count("kill-stagnated"))			m_killStagnated				= varMap["kill-stagnated"].as<uint32_t>();
	if (varMap.count("refocus-stagnated"))		m_refocusStagnated			= varMap["refocus-stagnated"].as<uint32_t>();
	if (varMap.count("target-species"))			m_targetSpecies				= varMap["target-species"].as<uint16_t>();
	if (varMap.count("seed-genome"))			m_seedGenome	 			= true;
	if (varMap.count("print-population"))			m_printPopulation 		= true;
	if (varMap.count("print-population-file"))		m_printPopulationFile 	= varMap["print-population-file"].as<string>();
	if (varMap.count("print-speciesstack-file"))	m_printSpeciesStackFile = varMap["print-speciesstack-file"].as<string>();
	if (varMap.count("print-speciessize-file"))		m_printSpeciesSizeFile 	= varMap["print-speciessize-file"].as<string>();
	if (varMap.count("print-fitness-file"))			m_printFitnessFile 		= varMap["print-fitness-file"].as<string>();
	if (varMap.count("print-super-champions"))		m_printSuperChampions 	= true;

	if (varMap.count("help")) 					{ cout << cliOptDesc; return 1; }

	if(m_refocusStagnated>m_killStagnated)
		{cout << "ERROR --refocus-stagnated must be inferior to --kill-stagnated."; exit(1); }

	if(m_seedGenome and m_genomeFile.empty())
		{cout << "ERROR --seed-genome requires --genome-file."; exit(1); }

	if( (m_threads<1) or (m_threads>32) )
		{cout << "ERROR --threads must be between 1 and 32."; exit(1); }


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
	 *** A3a Load genome from file
	 ***/

	Genome genomeFile;
	if(!m_genomeFile.empty())
	{
		// Load data
	 	cout << "INFO\tLoading genome from " << m_genomeFile << " into memory... " << flush;

		ifstream genomeIn( m_genomeFile.c_str() );
		if (!genomeIn.is_open()) { cout << "\nERROR\tFailed to open file." << endl; exit(1); }

		// Setup boost::tokenizer
		vector<string> fields; string line;
		while (getline(genomeIn,line))
		{
			// Tokenize the line into 'fields'
			boost::tokenizer< boost::escaped_list_separator<char> > tok(line);
			fields.assign(tok.begin(),tok.end());

			if(fields[0] == "id")
			{
				stringstream ss;
				ss << hex << fields[1];
				ss >> genomeFile.id;
			} 
			else if(fields[0] == "node")
			{
				uint16_t newNodeID = boost::lexical_cast<uint16_t>(fields[1]);
				NodeType newNodeType = NodeType::HIDDEN;

				if(fields[2] == "Sen") 		newNodeType = NodeType::SENSOR;
				else if(fields[2] == "Out") newNodeType = NodeType::OUTPUT;
				else if(fields[2] == "Bia") newNodeType = NodeType::BIAS;
				else if(fields[2] == "Hid") newNodeType = NodeType::HIDDEN;
				
				genomeFile.nodes[newNodeID] = NodeGene(newNodeID, newNodeType);
			}
			else if(fields[0] == "link")
			{
				ConnectionGene newConnection;
				newConnection.from_node 	= boost::lexical_cast<uint16_t>(fields[1]);
				newConnection.to_node 		= boost::lexical_cast<uint16_t>(fields[2]);
				newConnection.weight 		= boost::lexical_cast<double>(fields[3]);
				newConnection.enabled 		= boost::lexical_cast<bool>(fields[4]);
				newConnection.innovation 	= boost::lexical_cast<uint16_t>(fields[5]);

				// Update the innovations list
				if(m_seedGenome)
					g_innovationList[make_pair(newConnection.from_node,newConnection.to_node)] = newConnection.innovation;

				genomeFile.connections[newConnection.innovation] = newConnection;
			}
		}

		genomeIn.close();
		cout << "done." << endl;
	}




	/***
	 *** A3b If requested, test a loaded genome on the databases
	 ***/ 
	if(m_testGenome)
	{
		// Run the databases through the provided genome, and store the predictions.
		genomeFile.GetFitness(&TrainingDB, true);

		// File for writing.
		ofstream ofTraining("training.csv");

		// Output contact time and prediction
		for(vector<DataEntry>::iterator 
			iterDB = TrainingDB.begin();
			iterDB != TrainingDB.end();
			iterDB++)
			ofTraining << iterDB->contact_time << ',' << iterDB->prediction << '\n';

		// If a testing database was provided, repeat for the testing DB.
		if(!m_testdata.empty())
		{
			ofstream ofTest("test.csv");
			genomeFile.GetFitness(&TestDB, true);

			for(vector<DataEntry>::iterator 
				iterDB = TestDB.begin();
				iterDB != TestDB.end();
				iterDB++)
				ofTest << iterDB->contact_time << ',' << iterDB->prediction << '\n';
			
		}

		// Do nothing more.
		return 0;
	}




	/***
	 *** A4 Initialize global Random Number Generators
	 ***/

	cout << "INFO\tInitializing random number generators with seed " << m_seed << ".\n";

	// We use boost::random and mt19937 as a source of randomness.
	// The seed can be specified thorugh CLI
	g_rng.seed(m_seed);

	// To draw random integers (e.g. to randomly select a node), use:
	// boost::random::uniform_int_distribution<> dist(min, max);





	/***
	 *** A5 Prepare output streams
	 ***/

	static ofstream ofPopSummary, ofSpeciesStack, ofSpeciesSize, ofFitness;

	if(!m_printPopulationFile.empty()) 
		ofPopSummary.open(m_printPopulationFile.c_str());

	if(!m_printSpeciesStackFile.empty())
		ofSpeciesStack.open(m_printSpeciesStackFile.c_str());
	
	if(!m_printSpeciesSizeFile.empty())
		ofSpeciesSize.open(m_printSpeciesSizeFile.c_str());

	if(!m_printFitnessFile.empty())
		ofFitness.open(m_printFitnessFile.c_str());





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
	if(m_seedGenome)
		firstSpecies.genomes.push_back( genomeFile );	
	else
		firstSpecies.genomes.push_back( Genome(g_inputs) );
	population->species.push_back( firstSpecies );
	population->species.back().champion = &( population->species.back().genomes.back() );







	/***
	 *** C0 Loop evolution until criteria match
	 ***/

	// For tracking super champion evolution.
	double lastBestFitness = 0;

	// // For the self-adjusting compatibility threshold.
	bool f_reachedTargetSpecies = false;

	// Threads setup
	vector<pthread_t> threads(m_threads);

	do
	{
		if(gm_debug) cout << "DEBUG Generation " << g_generationNumber << endl;

		/* Initial setup for Generation loop.
		 */ 

		// Dynamic source of Gaussian randomness, for weight mutations.
		// Inits: (mean,stdev)
		static float adaptiveStDev;
		if(m_weightPerturbStdev != FLT_MAX)
			adaptiveStDev = m_weightPerturbStdev;
		else
		{
			// adaptiveStDev = 100/log(g_generationNumber+10)-10;
			// adaptiveStDev = (sin((float)g_generationNumber/20.0)+1.0)*49.0+1.0;
			adaptiveStDev = 1.0;
		}
		g_rnd_gauss.param( boost::random::normal_distribution<>::param_type( 0.0, adaptiveStDev ) );


		// Self-adjusting compatibility threshold
		float deltaThreshold = 0.01;
		if(m_targetSpecies)
		{
			if( (!f_reachedTargetSpecies) and (population->species.size() > m_targetSpecies ) )
				f_reachedTargetSpecies=true;
			if(f_reachedTargetSpecies)
			{
				if(population->species.size() > m_targetSpecies*1.20)
					m_compat_threshold += deltaThreshold;
				else if(population->species.size() < m_targetSpecies*0.80)
					m_compat_threshold -= deltaThreshold;
				if(m_compat_threshold < deltaThreshold) m_compat_threshold = deltaThreshold;
			}
		}



		/* C1 Go through every genome: update fitness.
		 * This is the most intensive process, so we thread it for performance.
		 */

		// Thread pointers
		threadDataUpdateGenomeFitness td;
		td.populationPointer = population;
		td.database = &TrainingDB;

		// Launch m_threads. 
		int rc, tId;
		for(tId=0; tId < m_threads; ++tId )
		{
			rc = pthread_create(&threads[tId], NULL, ThreadUpdateGenomeFitness, (void *)&td);
			assert (rc == 0);
		}

		// Wait for all threads to complete.
		for (tId = 0; tId < m_threads; ++tId) {
			// block until thread 'index' completes
			rc = pthread_join(threads[tId], NULL);
			assert(0 == rc);
		}

		// Put all iterSpecies->thread_processing back to false.
		for(list<Species>::iterator 
			iterSpecies = population->species.begin();
			iterSpecies != population->species.end();
			iterSpecies++)
			iterSpecies->thread_processing = false;





		/* C2 Eliminate the lowest performing members from the species.
		 * Requires: up-to-date fitness on all genomes.
		 */
		
		for(list<Species>::iterator 
			iterSpecies = population->species.begin();
			iterSpecies != population->species.end();
			iterSpecies++)
		{
			// Sort the vector of Genomes by fitness.
			// sort(iterSpecies->genomes.begin(), iterSpecies->genomes.end());
			iterSpecies->genomes.sort();

			// Sorting puts the lowest value (and therefore the highest fitness) last.
			// Determine how many Genomes we're pop-ing out, based on survival metric.
			uint16_t noSurvivors = iterSpecies->genomes.size() * m_survival_threshold;
			if(gm_debug) cout << "DEBUG Killing " << noSurvivors << " from species id " << iterSpecies->id << " size " << iterSpecies->genomes.size() << '\n';

			// Trim genome list
			if(noSurvivors>0)
				for(uint16_t killCount = 0; killCount < noSurvivors; killCount++)
					iterSpecies->genomes.pop_back();

		} // END SPECIES ITERATION





		/* C3 Update species and population statistics, track the champions.
		 * Kill stagnated, refocus stagnated species.
		 * Requires: up-to-date fitness on all genomes.
		 */
		
		// Update each species' champion, best fitness, generation update
		population->UpdateSpeciesAndPopulationStats();

		// Kill stale species
		if(m_killStagnated)
		{
			// Go through each species. If it has stagnated for more than m_killStagnated
			// generations, and has 3 genomes or less, kill it.
			
			list<Species>::iterator iterSpecies = population->species.begin();
			while(iterSpecies != population->species.end())
			{
				
				if( ((g_generationNumber - iterSpecies->lastImprovementGeneration) > m_killStagnated)
					and (iterSpecies->genomes.size()<=3) 
					and (&(*iterSpecies) != population->bestSpecies) ) // Don't kill the best species even if it stagnated
					iterSpecies = population->species.erase(iterSpecies);
				else
					iterSpecies++;
			}
		}


		// Refocus stagnated species. Kills off all but the top two genomes.
		if(m_refocusStagnated)
		{
			// Go through each species. If it has stagnated for more than m_refocusStagnated
			// generations, and has more than 2 genomes, keep only the top two genomes.

			for(list<Species>::iterator 
				iterSpecies = population->species.begin();
				iterSpecies != population->species.end();
				iterSpecies++)
				if( 	( (g_generationNumber - iterSpecies->lastImprovementGeneration) > m_refocusStagnated)
					and ( (g_generationNumber - iterSpecies->lastRefocusGeneration) > m_refocusStagnated)
					and (iterSpecies->genomes.size()>2) 
					and (&(*iterSpecies) != population->bestSpecies) ) 
				{
					// Keep the top two genomes, kill the rest.
					iterSpecies->genomes.sort();
					while(iterSpecies->genomes.size() > 2)
						iterSpecies->genomes.pop_back();

					// Reset their 'lastImprovementGeneration' counters or 
					// next loop will kill all children again
					iterSpecies->lastRefocusGeneration = g_generationNumber;
				}
		}





		/* C5pre Store the previous species and their champions.
		 * Requires: up-to-date champions on all species.
		 * Logically, this step should come before C5, but C4 wrecks the champions' pointers
		 * due to the trashing of the old species, so it's easier to store previous champions here.
		 */
		
		// Create the new population for step C4.
		Population* newPopulation = new Population();

		// Store the previous species and their champions
		for(list<Species>::iterator 
			iterSpecies = population->species.begin();
			iterSpecies != population->species.end();
			iterSpecies++)
		{
			// Clone each species with only the champion genome on them.
			assert(iterSpecies->bestFitness >= iterSpecies->champion->fitness);

			Species speciesCopy = Species(iterSpecies->id, iterSpecies->creation);
			speciesCopy.bestFitness = iterSpecies->bestFitness;
			speciesCopy.lastImprovementGeneration = iterSpecies->lastImprovementGeneration;
			speciesCopy.lastRefocusGeneration = iterSpecies->lastRefocusGeneration;

			speciesCopy.genomes.push_back( *(iterSpecies->champion) );

			// Push them to the new population
			newPopulation->species.push_back( speciesCopy );
			newPopulation->species.back().champion = &( newPopulation->species.back().genomes.back() );
		}





		/* C4 Intra-species mating and reproduction. 
		 * Requires: updated fitness values.
		 * Provides: explicit fitness sharing.
		 */
		
 		// Calculate adjusted fitness values.
 		map<Species*, double> sumAdjFitness;
 		// Go through all the species and genomes
		for(list<Species>::iterator 
			iterSpecies = population->species.begin();
			iterSpecies != population->species.end();
			iterSpecies++)
		{
			double speciesSumAdjFitness = 0.0;
			for(list<Genome>::iterator
				iterGenome = iterSpecies->genomes.begin();
				iterGenome != iterSpecies->genomes.end();
				iterGenome++)
			{
				iterGenome->adjFitness = iterGenome->fitness / iterSpecies->genomes.size();
				speciesSumAdjFitness += iterGenome->adjFitness;
			}

			// Store the sum(adjFitness) for this species
			sumAdjFitness[&(*iterSpecies)] = speciesSumAdjFitness;
		}

		// Sum of species' sums
		double totalAdjFitness=0.0;
		for(map<Species*, double>::const_iterator 
			iterSumAdjFitness = sumAdjFitness.begin();
			iterSumAdjFitness != sumAdjFitness.end();
			iterSumAdjFitness++)
			totalAdjFitness += iterSumAdjFitness->second;

		// Each species now gets sumAdjFitness/totalAdjFitness*m_maxPop allowed children.

 		// Reproduce each species.
		for(map<Species*, double>::const_iterator 
			iterSpecies = sumAdjFitness.begin();
			iterSpecies != sumAdjFitness.end();
			iterSpecies++)
			iterSpecies->first->Reproduce( sumAdjFitness[iterSpecies->first]/totalAdjFitness*m_maxPop );





		/* C5 Go through every genome and place it in a species according to its compatibility.
		 * Requires: Knowing champions of each species, pre-mating.
		 */


		// Go through each species
		for(list<Species>::iterator 
			iterSpecies = population->species.begin();
			iterSpecies != population->species.end();
			iterSpecies++)
			// Go through each genome
			for(list<Genome>::iterator
				iterGenome = iterSpecies->genomes.begin();
				iterGenome != iterSpecies->genomes.end();
				iterGenome++)
			{
				/* We have two algorithms:
				 * firstCompat: place genomes in the first species where they match the champion
				 * bestCompat: place genomes in the species with the best compatibility
				 */
				if(m_bestCompat)
				{
					// Compute compatibility of this genome to each species
					// This stores <speciesPointer,compatibility> pairs
					map<Species*,double> compatibilityMatch;

					// Track compatibilities. This looks for matches in the *new* population,
					// which was populated with empty species + champions on step C5pre.
					for(list<Species>::iterator 
						iterSpeciesCompat = newPopulation->species.begin();
						iterSpeciesCompat != newPopulation->species.end();
						iterSpeciesCompat++)
						compatibilityMatch[ &(*iterSpeciesCompat) ] = Compatibility(&(*iterGenome), iterSpeciesCompat->champion);

					// Locate min distance (compatibility)
					double minDistance = DBL_MAX; 
					Species* mostCompatSpecies = 0;
					for(map<Species*,double>::const_iterator 
						iterCompat = compatibilityMatch.begin();
						iterCompat != compatibilityMatch.end();
						iterCompat++ )
						if(iterCompat->second < minDistance)
							{ minDistance = iterCompat->second; mostCompatSpecies = iterCompat->first; }

					if(gm_debug >= 2) 
						cout 	<< "DEBUG Determined best compatibility " << setprecision(2) << fixed << minDistance 
								<< " with species " << mostCompatSpecies->id
								<< " for genome " << hex << iterGenome->id << dec
								<< endl;

					// Now place the genome in the new population. 
					// If compatibility==0, we compared with ourselves, do nothing.
					// If compatibility < m_compat_threshold, we fit in species 'mostCompatSpecies', put it there.
					// If compatibility > m_compat_threshold, create a new species, put it there, mark it as the champion.
					if(minDistance!=0)
					{
						if(minDistance < m_compat_threshold)
						{
							// Place the genome in 'mostCompatSpecies'
							mostCompatSpecies->genomes.push_back( *iterGenome );
						}
						else
						{
							// Create a new species for the genome.
							Species newSpecies(++g_newSpeciesId, g_generationNumber);

							// Push the genome as the champion
							newSpecies.genomes.push_back( *iterGenome );
							newPopulation->species.push_back( newSpecies );
							newPopulation->species.back().champion = &( newPopulation->species.back().genomes.back() );
						}					
					}
				} // END BEST COMPAT ALGO
				else
				{
					// Find the first compatible match instead.
					bool f_found = false;
					double distance = 0;
					list<Species>::iterator iterSpeciesCompat;
					for(iterSpeciesCompat = newPopulation->species.begin();
						(iterSpeciesCompat != newPopulation->species.end() ) and !f_found;
						iterSpeciesCompat++)
					{
						distance = Compatibility(&(*iterGenome), iterSpeciesCompat->champion);
						if( distance < m_compat_threshold )
							f_found = true;
					}

					// If we found a species that is compatible, place the genome there.
					// Else: create a new species for the genome.
					if(f_found)
					{
						advance(iterSpeciesCompat,-1); // Note that the above for() incremented one extra time after flipping f_found;
						iterSpeciesCompat->genomes.push_back( *iterGenome );

						if(gm_debug >= 2) 
							cout 	<< "DEBUG Match distance " << setprecision(2) << fixed << distance 
									<< " with species " << iterSpeciesCompat->id
									<< " for genome " << hex << iterGenome->id << dec
									<< endl;
					}
					else
					{
						// Create a new species for the genome.
						Species newSpecies(++g_newSpeciesId, g_generationNumber);

						// Push the genome as the champion
						newSpecies.genomes.push_back( *iterGenome );
						newPopulation->species.push_back( newSpecies );
						newPopulation->species.back().champion = &( newPopulation->species.back().genomes.back() );
						newPopulation->species.back().bestFitness = newPopulation->species.back().champion->fitness;

						if(gm_debug) 
							cout 	<< "DEBUG Created new species for genome " << hex << iterGenome->id << dec
									<< " distance " << setprecision(2) << fixed << distance 
									<< " speciesID " << g_newSpeciesId << endl;
					}

				} // END FIRST COMPAT ALGO
			} // END GO THROUGH EACH GENOME

		// We now have a new, sorted population.
		// Replace population with new population.
		delete population;
		population = newPopulation;
		
		// Update statistics on the new population
		population->UpdateSpeciesAndPopulationStats();





		/* CZ Output requested statistics.
		 */

		if(m_printPopulation)
			population->PrintSummary(cout);

		if(!m_printPopulationFile.empty()) 
			{ population->PrintSummary(ofPopSummary); ofPopSummary.flush(); }

		if(!m_printSpeciesStackFile.empty())
			{ population->PrintVerticalSpeciesStack(ofSpeciesStack); ofSpeciesStack.flush(); }

		if(!m_printSpeciesSizeFile.empty())
			{ population->PrintSpeciesSize(ofSpeciesSize); ofSpeciesSize.flush(); }		


		if(!m_printFitnessFile.empty())
		{
			if(!m_testdata.empty())
				{ population->PrintFitness(ofFitness, &TrainingDB, &TestDB); ofFitness.flush(); }
			else
				{ population->PrintFitness(ofFitness, &TrainingDB); ofFitness.flush(); }
		}

		if( m_printSuperChampions and (population->bestFitness != lastBestFitness) )
		{
			// Every generation, if the superchampion changed, print it
			lastBestFitness = population->bestFitness;
			stringstream ssfilenameGV, ssfilenameCSV;
			ssfilenameGV 	<< "superChampion" 
							<< "_gen" << setfill('0') << setw(6) << g_generationNumber 
							<< "_species" << setfill('0') << setw(3) << population->bestSpecies->id 
							<< ".gv";
			const string& filenameGV = ssfilenameGV.str();
			population->superChampion->PrintToGV(filenameGV.c_str());

			ssfilenameCSV 	<< "superChampion" 
				<< "_gen" << setfill('0') << setw(6) << g_generationNumber 
				<< "_species" << setfill('0') << setw(3) << population->bestSpecies->id 
				<< ".csv";
			const string& filenameCSV = ssfilenameCSV.str();
			population->superChampion->SaveToFile(filenameCSV.c_str());
		}

	// Generation loop control
	g_generationNumber++; 
	} while( g_generationNumber < m_genmax );	// Specify stopping criteria here


	/***
	 *** Z0 Wrap up
	 ***/

	// Print the super champion.
	population->UpdateSpeciesAndPopulationStats();

	population->superChampion->Print(cout);
	population->superChampion->PrintToGV("superChampion.gv");
	population->superChampion->SaveToFile("superChampion.csv");

	delete population;

	return 0;
}


void *ThreadUpdateGenomeFitness(void *threadarg)
{
	threadDataUpdateGenomeFitness* threadPointers;
	threadPointers = (threadDataUpdateGenomeFitness *) threadarg;
	
	for(list<Species>::iterator 
		iterSpecies = threadPointers->populationPointer->species.begin();
		iterSpecies != threadPointers->populationPointer->species.end();
		iterSpecies++)
		if(!iterSpecies->thread_processing)
		{
			iterSpecies->thread_processing = true;
			iterSpecies->UpdateGenomeFitness(threadPointers->database);
		}
	pthread_exit(NULL);
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

