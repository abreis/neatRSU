#ifndef GENETIC_H_
#define GENETIC_H_

#include "neural.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>

using namespace std;

/* Definitions
   ----------- */




/* Functions
   --------- */
/* 	In the add node mutation, an existing connection is split 
	and the new node placed where the old connection used to be. 
	The old connection is disabled and two new connections are 
	added to the genome. The new connection leading into the new 
	node receives a weight of 1, and the new connection leading 
	out receives the same weight as the old connection. This 
	method of adding nodes was chosen in order to minimize the 
	initial effect of the mutation.
*/
// Genome MutateAddNode(Genome origin);

// Crossover two genomes to generate offspring.
// Genome MutateCrossover(Genome first, Genome second);




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
	// List of connections. Each connection's innovation must be unique
	map<uint16_t, ConnectionGene> connections;

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


#endif /* GENETIC_H_ */
