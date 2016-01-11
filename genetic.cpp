#include "genetic.h"

Genome::Genome(uint16_t n_inputs)
{
	// Add an output node. Output node ID will always be #inputs+1
	// NodeGene test(n_inputs+1, NodeType::output); 
	nodes[n_inputs+1] = NodeGene(n_inputs+1, NodeType::output);

	// Fill the NodeGene vector with input nodes, and fill the ConnectionGene
	// vector with a connection from each node to the output.
	for(uint16_t 
		n = 1;	
		n <= n_inputs; 
		n++)
	{
		nodes[n] = NodeGene(n, NodeType::sensor);
		connections[n] = ConnectionGene(n, n_inputs+1, n); // Initial innovation matches sensor node ID 
	}
}


bool Genome::Verify(void)
{	
	// Every connection must point to a valid node ID
	// TODO

	return true; 
}


void Genome::Print()
{
	for(uint16_t i = nodes.size(); i>0; i--)
		cout << "----"; cout << '\n';
	for(map<uint16_t, NodeGene>::const_iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
		cout << left << setw(4) << setfill(' ') << iterNode->first; cout << '\n';
	for(map<uint16_t, NodeGene>::const_iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
		cout << ( (iterNode->second.type==NodeType::sensor)?"Sen ":( (iterNode->second.type==NodeType::hidden)?"Hid ":"Out " ) ); cout << '\n';
	for(uint16_t i = nodes.size(); i>0; i--)
		cout << "----"; cout << '\n';

	// Print a list of connections
	cout 	<< "-------------------------------------" << '\n'
			<< "Path   Enable   Weight          Innov" << '\n'
			<< "-------------------------------------" << '\n';

	for(map<uint16_t, ConnectionGene>::const_iterator 
		iterConn = connections.begin();
		iterConn != connections.end();
		iterConn++)
	{
		cout 	<< setfill(' ')	<< right << setw(2) << iterConn->second.in_node 
				<< "->" 		<< left << setw(2) << iterConn->second.out_node
				<< right
				<< '\t' << ( (iterConn->second.enabled)?"":"DIS" )
				<< '\t' << scientific << setprecision(2) << setfill(' ') << iterConn->second.weight
				<< '\t' << iterConn->second.innovation
				<< '\n';
	}

}
