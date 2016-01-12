#include "genetic.h"

// Global innovation number
uint16_t g_innovNumber = 0;

// List of innovations (pair(fromNode,toNode),innov#)
map<pair<uint16_t,uint16_t>,uint16_t> g_innovations; 


/* Functions
 */


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


/* CLASS Genome
 */


Genome::Genome(uint16_t n_inputs)
{
	// Add an output node. Output node ID will always be #inputs+1
	nodes[d_outputnode] = NodeGene(d_outputnode, NodeType::OUTPUT);

	// Add a bias node. Bias node ID will always be #inputs+2
	// Constructor ensures adequate node value = 1 for type BIAS
	nodes[d_biasnode] = NodeGene(d_biasnode, NodeType::BIAS);	

	// Fill the NodeGene vector with input nodes, and fill the ConnectionGene
	// vector with a connection from each node to the output.
	for(uint16_t 
		n = 1;	
		n <= n_inputs; 
		n++)
	{
		nodes[n] = NodeGene(n, NodeType::SENSOR);
		connections[make_pair(n,d_outputnode)] = ConnectionGene(n, d_outputnode, n); // Initial innovation matches sensor node ID 
	}
}


double Genome::Activate(DataEntry data)
{
	// Put the DataEntry at the inputs
	nodes[1].valueNow=data.node_id;
	nodes[2].valueNow=data.relative_time;
	nodes[3].valueNow=data.latitude;
	nodes[4].valueNow=data.longitude;
	nodes[5].valueNow=data.speed;
	nodes[6].valueNow=data.heading;

	// Move all valueNow to valueLast, reset valueNow
	for(map<uint16_t, NodeGene>::iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
	{
		iterNode->second.valueLast = iterNode->second.valueNow;
		iterNode->second.valueNow = 0;
	}

	// Run through each connection
	for(map<pair<uint16_t,uint16_t>, ConnectionGene>::const_iterator 
		iterConn = connections.begin();
		iterConn != connections.end();
		iterConn++)
	{
		// Add the connection's effect to the destination node's 'valueNow'
		nodes[iterConn->second.to_node].valueNow += 
			nodes[iterConn->second.from_node].valueLast * iterConn->second.weight;
	}

	// Return the value at the output node
	return nodes[d_outputnode].valueNow;
}


double Genome::GetFitness(vector<DataEntry>* database, bool store)
{
	double fitness = 0.0;

	/* Go through every entry. Perform an activation, get the prediction.
	 * Sum the square of errors.
	 * IMPORTANT: the entry database must be sorted logically for recurrent networks to make sense 
	 */
	for(vector<DataEntry>::iterator 
		iterDB = database->begin();
		iterDB != database->end();
		iterDB++)
	{
		double prediction = this->Activate(*iterDB);
		fitness += pow(prediction-iterDB->contact_time, 2);
		if(store) iterDB->prediction=prediction;
	}
	return fitness;
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
	gvout << "digraph finite_state_machine {\n\trankdir=LR;\n";

	// Input node cluster
	gvout 	<< "\tsubgraph cluster_0 {\n"
			<< "\t\tlabel = \"inputs\";\n"
			<< "\t\tnode [shape=circle,style=filled,color=lightgrey,fixedsize=true,width=1];\n"
			<< "\t\t";
	for(map<uint16_t, NodeGene>::const_iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
		if(iterNode->second.type==NodeType::SENSOR)
			gvout << ' ' << g_nodeNames[iterNode->first];
	gvout << ";\n\t}\n";

	// Output node cluster (assumes single node at d_outputnode)
	gvout 	<< "\tsubgraph cluster_1 {\n"
			<< "\t\tlabel = \"\";\n"
			<< "\t\tstyle = filled;\n"
			<< "\t\tcolor = lightgrey;\n"
			<< "\t\tnode [shape=circle,style=filled,color=white];\n"
			<< "\t\t" << ' ' << g_nodeNames[d_outputnode] << ";\n\t}\n";

	// Bias node
	gvout 	<< "\tnode [shape=circle,style=filled,color=dimgrey,fontcolor=white];"
			<< ' ' << g_nodeNames[d_biasnode] << ";\n";

	// Hidden nodes
	gvout << "\tnode [shape=circle,style=filled,fillcolor=white,fontcolor=black];";
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
			gvout 	<< '\t' 
					<< ( (iterConn->second.from_node <= g_nodeNames.size())?g_nodeNames[iterConn->second.from_node]:to_string(iterConn->second.from_node) )
					<< " -> " 
					<< ( (iterConn->second.to_node <= g_nodeNames.size())?g_nodeNames[iterConn->second.to_node]:to_string(iterConn->second.to_node) )
					<< " [ label = \""
					<< fixed << setprecision(1) << iterConn->second.weight
					<< "\" ];\n";

	// Close file
	gvout << "}\n";
	gvout.close();
}