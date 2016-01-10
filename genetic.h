#ifndef GENETIC_H_
#define GENETIC_H_



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

class Genome
{
	// A genome has a set of nodes and a set of connections.
	// Include routines to manipulate the genome and verify its structure.

public:
	bool Verify(void);
};

// A node gene.
struct NodeGene
{
	uint16_t	id;
	NodeType	type;
};

// A connection gene.
struct ConnectionGene
{
	uint16_t	in_node;
	uint16_t	out_node;
	float		weight;
	bool		enabled;
	uint16_t	innovation;
};



#endif /* GENETIC_H_ */
