#ifndef GENETIC_H_
#define GENETIC_H_

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <cfloat>
#include <limits>

#include <map>
#include <vector>
#include <list>
#include <string>

#include "neatRSU.h"

using namespace std;

/* Definitions
   ----------- */
enum NodeType { SENSOR, HIDDEN, OUTPUT, BIAS };

// Global innovation number
extern uint16_t g_innovations;
// List of innovations (pair(fromNode,toNode),innov#)
extern map<pair<uint16_t,uint16_t>,uint16_t> g_innovationList; 


/* Functions
   --------- */
// Custom sigmoidal transfer function.
double ActivationSigmoid (double input);

// MATLAB's tansig transfer function.
double ActivationMatlabTanSig (double input);

// Output transfer function.
double ActivationOutput (double input);

// Mates two genomes and returns the resulting offspring.
// Uses each genome's fitness, so be sure it is up to date.
class Genome;
Genome MateGenomes(Genome* const firstParent, Genome* const secondParent);

// Get the measure of compatibility between two nodes
double Compatibility(Genome* const gen1, Genome* const gen2);



/* Classes and Structs
   ------------------- */

/*
 * A Population has Species
 * Species have Genomes
 * Genomes have Nodes and Connections
 */


// A node gene.
class NodeGene
{
public:
	uint16_t	id;
	NodeType	type;

	// The following holds values throughout activations with recurrency
	double		valueLast=0;
	double		valueNow=0;

	NodeGene(){};
	NodeGene(uint16_t iid, NodeType ttype) 
		{ 
			id = iid; type = ttype; 
			if(type==NodeType::BIAS) {valueNow=1; valueLast=1;}
		}
};


// A connection gene.
class ConnectionGene
{
public:
	uint16_t	from_node;
	uint16_t	to_node;
	double		weight = 1.0;
	bool		enabled = true;
	uint16_t	innovation;

	ConnectionGene(){};
	ConnectionGene(uint16_t from, uint16_t to, uint16_t innov, double weightin=1.0)
		{ from_node = from; to_node = to, innovation = innov; weight=weightin;}

	// Sorting performed by innovation (lowest value to highest -> earliest to latest)
	bool operator < (const ConnectionGene& conn) const
		{ return (innovation < conn.innovation); }
};


// A genome has a set of nodes and a set of connections.
// Include routines to manipulate the genome and verify its structure.
class Genome
{
public:
	// List of nodes. Each nodeID is unique, so we use an std::map
	map<uint16_t, NodeGene> nodes;
	// List of connections. We use the innovation as key.
	map<uint16_t, ConnectionGene> connections;

	// The current fitness and adjusted fitness of this genome
	double fitness = 0;
	double adjFitness = 0;

	// A unique random identifier for this genome.
	uint64_t id = 0; 

	/* Essentials
	 */ 
	// A blank genome, for copies and mating, with a random ID.
	Genome();

	// Set up a new genome with a specific number of inputs.
	Genome(uint16_t n_inputs);

	// Assigns a new random ID to the genome.
	void RandomizeID(void);

	// Adds a new node to the genome. Checks if it already exists. If no ID is specified, finds and increments.
	uint16_t AddNode(NodeType type, uint16_t id=UINT16_MAX);

	// Add a connection to the Genome. Automatically tracks innovation numbers.
	// Returns false if the connection already exists. If reenable==true, replaces a disabled connection, if it exists.
	bool AddConnection(uint16_t from, uint16_t to, bool reenable=false, double inWeight=DBL_MAX);

	// Push a set of inputs through a genome, and return the value of the output node.
	double Activate(DataEntry entry);

	// Run a complete DB through this genome, compute every prediction, and return fitness.
	// If store==true, store each prediction in the database at database[i]->prediction;
	double GetFitness(vector<DataEntry>* database, bool store=false);

	// Wipe the values inside the nodes.
	void WipeMemory(void);

	/* Mutations1
	 */
	// Add a random value to the weights of this genome.
	void MutatePerturbWeights(void);

	// Add a single new connection gene with a random weight.
	void MutateAddConnection(void);

	// Split a connection gene into two and add a node in the middle.
	void MutateAddNode(void);

	// Disables a random connection.
	void MutateDisableConnection(void);

	// Disables all connections going from and to a random hidden node.
	void MutateDisableNode(void);



	/* Auxiliary
	 */
	// Count the number of ConnectionGenes that are not disabled.
	pair<uint16_t,uint16_t> CountEnabledGenes(void);

	// Resets the nodes' memories.
	void ResetNodes();

	// Print the contents of this Genome
	void Print(ostream& outstream);

	// Print the contents of this Genome as Graphviz language to a file
	void PrintToGV(string filename);

	// Save the contents of the genome to a file, in CSV format.
	void SaveToFile(string filename);

	// Sorting performed by fitness (highest value to lowest -> highest fitness to lowest)
	bool operator < (const Genome& gen) const
		{ return (fitness > gen.fitness); }
};



// A species, a collection of genomes.
class Species
{
public:
	// Species ID, species generation of creation.
	uint16_t id = UINT16_MAX; 
	uint32_t creation = UINT32_MAX;
	
	double bestFitness = 0;	// Best possible fitness is=1
	uint32_t lastImprovementGeneration;
	uint32_t lastRefocusGeneration;

	// For thread tracking.
	bool thread_processing = false;

	// For tracking the species champion.
	Genome* champion;

	// Main vector of this species' genomes.
	// Must be a list so pointers to champions are kept safe.
	list<Genome> genomes;

	// Constructor, require speciesID
	Species(uint16_t iid, uint32_t generation)
		{ id = iid; creation = generation; 
			lastImprovementGeneration = generation;
			lastRefocusGeneration = generation;}

	// Perform mating on this species, up to the maximum of 'count'.
	void Reproduce(uint16_t targetSpeciesSize);

	// Finds the best genome in the species and returns a pointer to it.
	Genome* FindChampion(void);

	// Update the fitness on all genomes. Ideal for pthreading.
	void UpdateGenomeFitness(vector<DataEntry>* database);

	// Prints a summary of the species statistics and its genomes.
	void Print(ostream& outstream);
};

// A population.
class Population
{
public:
	double bestFitness = 0;	// Best possible fitness is=1
	Species* bestSpecies;
	Genome* superChampion;

	// Main vector of this populations' species.
	// Must be a list so pointers to species and super champion are kept safe.
	list<Species> species;

	// How many species we aim to get
	// uint16_t targetNumber=10;

	// Go through every species, update its fitness value,
	// counters, champions, and then the population's own best.
	void UpdateSpeciesAndPopulationStats(void);

	// Prints a summary of the population: best fitness so far, list of species, statistics.
	void PrintSummary(ostream& outstream);

	// Prints a vertical stacked graph of the % size each species occupies. 
	void PrintVerticalSpeciesStack(ostream& outstream);

	// Prints a (generation,bestFitness) pair.
	void PrintFitness(ostream& outstream, vector<DataEntry>* database1, vector<DataEntry>* database2=0);
};

// For carrying pointers to the thread routine.
struct threadDataUpdateGenomeFitness
{
	Population* populationPointer;
	vector<DataEntry>* database;
};


#endif /* GENETIC_H_ */
