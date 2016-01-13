#ifndef GENETIC_H_
#define GENETIC_H_

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

#include <map>
#include <vector>
#include <string>

#include "neatRSU.h"

using namespace std;

/* Definitions
   ----------- */
enum NodeType { SENSOR, HIDDEN, OUTPUT, BIAS };


/* Functions
   --------- */
// Custom sigmoidal transfer function.
double ActivationSigmoid (double input);

// Output transfer function.
double ActivationOutput (double input);



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
	ConnectionGene(uint16_t in, uint16_t out, uint16_t innov)
		{ from_node = in; to_node = out, innovation = innov; }
};


// A genome has a set of nodes and a set of connections.
// Include routines to manipulate the genome and verify its structure.
class Genome
{
public:
	// List of nodes. Each nodeID is unique, so we use an std::map
	map<uint16_t, NodeGene> nodes;
	// List of connections. We use the from->to nodeIDs as keys
	map<pair<uint16_t,uint16_t>, ConnectionGene> connections;

	// The current fitness of this genome
	uint32_t fitness = 0;

	// Set up a new genome with a specific number of inputs.
	Genome(uint16_t n_inputs);

	// Push a set of inputs through a genome, and return the value of the output node.
	double Activate(DataEntry entry);

	// Run a complete DB through this genome, compute every prediction, and return fitness.
	// If store==true, store each prediction in the database at database[i]->prediction;
	double GetFitness(vector<DataEntry>* database, bool store=false);

	/* Mutations
	 */
	// Add a single new connection gene with a specified weight.
	void MutateAddConnection(double weight);

	// Split a connection gene into two and add a node in the middle.
	void MutateAddNode(void);


	/* Auxiliary
	 */
	// Verify the integrity of the Genome. Returns false if there were inconsistencies.
	bool Verify(void);

	// Print the contents of this Genome
	void Print();

	// Print the contents of this Genome as Graphviz language to a file
	void PrintToGV(string filename);
};

// A species, a collection of genomes.
class Species
{
public:
	vector<Genome> genomes;
};

// A population.
class Population
{
public:
	vector<Species> species;

	// How many species we aim to get
	uint16_t targetNumber=10;
};

#endif /* GENETIC_H_ */
