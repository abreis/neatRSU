#ifndef GENETIC_H_
#define GENETIC_H_

#include "neural.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <vector>

using namespace std;

/* Definitions
   ----------- */

/*
A Population has Species
Species have Genomes
*/


/* Functions
   --------- */




/* Classes and Structs
   ------------------- */


// A node gene.
class NodeGene
{
public:
	uint16_t	id;
	NodeType	type;

	NodeGene(){};
	NodeGene(uint16_t iid, NodeType ttype) 
		{ id = iid; type = ttype; }
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
	void PrintToGV(Genome gen, string filename);
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
