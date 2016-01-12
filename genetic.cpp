#include "genetic.h"

// Global innovation number
uint16_t g_innovNumber = 0;

// List of innovations (pair(fromNode,toNode),innov#)
map<pair<uint16_t,uint16_t>,uint16_t> g_innovations; 

extern uint16_t g_inputs;




double ActivationSigmoid (double input)
{
	/* The steepened sigmoid allows more fine tuning at extreme activations. 
	 * It is optimized to be close to linear during its steepest ascent 
	 * between activations −0.5 and 0.5.
	 */
	 return ( 1.0/( 1.0+exp(-4.9*input) ) );
}


double ActivationOutput (double input)
{
	// Linear.
	return input;
}


// double Activate(Genome* gen, DataEntry data)
// {
// 	return (gen->nodes)[g_inputs+1].value_now;
// }

Genome::Genome(uint16_t n_inputs)
{
	// Add an output node. Output node ID will always be #inputs+1
	// NodeGene test(n_inputs+1, NodeType::output); 
	nodes[n_inputs+1] = NodeGene(n_inputs+1, NodeType::OUTPUT);

	// Fill the NodeGene vector with input nodes, and fill the ConnectionGene
	// vector with a connection from each node to the output.
	for(uint16_t 
		n = 1;	
		n <= n_inputs; 
		n++)
	{
		nodes[n] = NodeGene(n, NodeType::SENSOR);
		connections[make_pair(n,n_inputs+1)] = ConnectionGene(n, n_inputs+1, n); // Initial innovation matches sensor node ID 
	}
}


/* Mutations
 */


// Add a single new connection gene with a specified weight.
void MutateAddConnection(double weight)
{

}

// Split a connection gene into two and add a node in the middle.
/* An old connection is disabled and two new connections are added to the genome. 
 * The new connection leading into the new node receives a weight of 1, and 
 * the new connection leading out receives the same weight as the old connection. 
 * This method of adding nodes minimizes the initial effect of the mutation.
 */
void MutateAddNode(void)
{

}


bool Genome::Verify(void)
{	
	// Every connection must point to a valid node ID
	// TODO

	return true; 
}


/* Auxiliary
 */


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
		cout << ( (iterNode->second.type==NodeType::SENSOR)?"Sen ":( (iterNode->second.type==NodeType::HIDDEN)?"Hid ":"Out " ) ); cout << '\n';
	for(uint16_t i = nodes.size(); i>0; i--)
		cout << "----"; cout << '\n';

	// Print a list of connections
	cout 	<< "-------------------------------------" << '\n'
			<< "Path   Enable   Weight          Innov" << '\n'
			<< "-------------------------------------" << '\n';

	for(map<pair<uint16_t,uint16_t>, ConnectionGene>::const_iterator 
		iterConn = connections.begin();
		iterConn != connections.end();
		iterConn++)
	{
		cout 	<< setfill(' ')	<< right << setw(2) << iterConn->second.from_node 
				<< "->" 		<< left << setw(2) << iterConn->second.to_node
				<< right
				<< '\t' << ( (iterConn->second.enabled)?"":"DIS" )
				<< '\t' << scientific << setprecision(2) << setfill(' ') << iterConn->second.weight
				<< '\t' << iterConn->second.innovation
				<< '\n';
	}

}


void Genome::PrintToGV(Genome gen, string filename)
{
	// Open file
	ofstream gvout(filename.c_str());
	if (!gvout.is_open()) { cout << "\nERROR\tFailed to open file for writing." << endl; exit(1); }

	// Output standard header
	gvout << "digraph finite_state_machine {\n\trankdir=LR;\n\tsize=\"8,5\"\n";

	// Input node cluster
	gvout 	<< "\tsubgraph cluster_0 {\n"
			<< "\t\tlabel = \"inputs\";\n"
			<< "\t\tnode [shape=circle,style=filled,color=lightgrey];\n"
			<< "\t\t";
	for(map<uint16_t, NodeGene>::const_iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
		if(iterNode->second.type==NodeType::SENSOR)
			gvout << ' ' << iterNode->first;
	gvout << ";\n\t}\n";

	// Output node cluster
	gvout 	<< "\tsubgraph cluster_1 {\n"
			<< "\t\tlabel = \"output\";\n"
			<< "\t\tstyle = filled;\n"
			<< "\t\tcolor = lightgrey;\n"
			<< "\t\tnode [shape=circle,style=filled,color=white];\n"
			<< "\t\t";
	for(map<uint16_t, NodeGene>::const_iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
		if(iterNode->second.type==NodeType::OUTPUT)
			gvout << ' ' << iterNode->first;
	gvout << ";\n\t}\n";

	// Hidden nodes
	gvout << "\tnode [shape=circle];";
	for(map<uint16_t, NodeGene>::const_iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
			if(iterNode->second.type==NodeType::HIDDEN)
			gvout << ' ' << iterNode->first;
	gvout << ";\n";

	// Output connections
	for(map<pair<uint16_t,uint16_t>, ConnectionGene>::const_iterator 
		iterConn = connections.begin();
		iterConn != connections.end();
		iterConn++)
		if(iterConn->second.enabled)
			gvout 	<< '\t' << iterConn->second.from_node << " -> " << iterConn->second.to_node
					<< " [ label = \""
					<< fixed << setprecision(1) << iterConn->second.weight
					<< "\" ];\n";

	// Close file
	gvout << "}\n";
	gvout.close();
}