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
		this->AddConnection(n, d_outputnode);
	}
}


uint16_t Genome::AddNode(NodeType type, uint16_t id)
{
	if(id==UINT16_MAX)
	{
		// Create the next free node
		id = nodes.rbegin()->first + 1;
		nodes[id] = NodeGene(id, type);
		return id;
	}		
	else
	{	
		// Check if the node already exists.
		auto iterFindNode = nodes.find(id);
		if(iterFindNode != nodes.end())
			{ cout << "ERROR: Tried to add a node that already exists." << endl; exit(1); }
		// Else create the node.
		else
			{ nodes[id] = NodeGene(id, type); return id; }
	}
}


bool Genome::AddConnection(uint16_t from, uint16_t to, bool reenable, double inWeight)
{
	// 01 See if the pair exists.
	// 02 If it exists, is disabled, and reenable==true, enable the pair and return true. 
	// 03 If it doesn't exist, create it and return true.
	// 04 Else return false.

	// 01 See if the pair exists.
	auto iterFindConn = connections.find(make_pair(from,to));
	if( iterFindConn != connections.end() )
	{
		// 02 If it exists, is disabled, and reenable==true, enable the pair and return true. 
		if(!iterFindConn->second.enabled and reenable)
		{
			iterFindConn->second.enabled = true;
			// If a weight was specified, replace it too.
			if(inWeight != DBL_MAX) iterFindConn->second.weight = inWeight;
			return true;
		}
	}
	// 03 If it doesn't exist, create it and return true.
	else 
	{
		// TODO innovation
		// Innovation must satisfy initial node creations:
		// // connections[make_pair(n,d_outputnode)] = ConnectionGene(n, d_outputnode, n); // Initial innovation matches sensor node ID 
		connections[make_pair(from,to)] = ConnectionGene(from, to, 0,  ( (inWeight==DBL_MAX)?1.0:inWeight )  );
		return true;
	}
	// 04 Else return false.
	return false;
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
		iterNode->second.valueNow = 0.0;
	}
	nodes[d_biasnode].valueNow = 1.0; // Don't touch the bias

	// Run through each connection
	for(map<pair<uint16_t,uint16_t>, ConnectionGene>::const_iterator 
		iterConn = connections.begin();
		iterConn != connections.end();
		iterConn++)
	{
		// Add the connection's effect to the destination node's 'valueNow'
		if(iterConn->second.enabled)
			nodes[iterConn->second.to_node].valueNow += 
				nodes[iterConn->second.from_node].valueLast * iterConn->second.weight;
	}

	// On hidden nodes, this value must now go through the activation sigmoid
	map<uint16_t, NodeGene>::iterator iterNode = nodes.begin();
	for( advance(iterNode, d_firsthidnode-1);
	iterNode != nodes.end();
	iterNode++)
		if(iterNode->second.type==NodeType::HIDDEN)
			iterNode->second.valueNow = ActivationSigmoid(iterNode->second.valueNow);

	// Same for the output node
	nodes[d_outputnode].valueNow = ActivationOutput(nodes[d_outputnode].valueNow);

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


void Genome::WipeMemory(void)
{
	// Clean every node's memory.
	for(map<uint16_t, NodeGene>::iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
	{ iterNode->second.valueLast = 0; iterNode->second.valueNow = 0; }

	// Set bias node back to '1'
	nodes[d_biasnode].valueNow = 1.0;
	nodes[d_biasnode].valueLast = 1.0;
}


/* Mutations
 */


void Genome::MutatePerturbWeights(boost::random::normal_distribution<> randomDistribution)
{
	// Go through each connection, perturb its weight.
		for(map<pair<uint16_t,uint16_t>, ConnectionGene>::iterator 
		iterConn = connections.begin();
		iterConn != connections.end();
		iterConn++)
			iterConn->second.weight += randomDistribution(rng);
}


void Genome::MutateAddConnection(boost::random::normal_distribution<> randomDistribution)
{
	// Get two random node numbers. 
	// 'from' can be anything, 'to' must exclude inputs and bias, but not output
	// We must use a trick here, to be able to pick the output node
	boost::random::uniform_int_distribution<> randomSrcNode(0, nodes.size()-1);
	boost::random::uniform_int_distribution<> randomDstNode(d_firsthidnode, nodes.size());
	
	// Try pairs until we find one that either doesn't exist or is disabled
	map<uint16_t, NodeGene>::const_iterator iterSrcNode, iterDstNode;
	uint16_t srcNode, dstNode;
	double newWeight = randomDistribution(rng);
	do
	{
		// Draw a source node. Anything goes.
		uint16_t randomNodeId = randomSrcNode(rng);
		iterSrcNode = nodes.begin();
		advance(iterSrcNode, randomNodeId);
		srcNode = iterSrcNode->first;

		// Get the destination node. If we draw nodes.size() (which is last+1, invalid), select the output node.
		randomNodeId = randomDstNode(rng);
		if(randomNodeId == nodes.size()) 
			dstNode = d_outputnode;
		else
		{
			iterDstNode = nodes.begin();
			advance(iterDstNode, randomNodeId);
			dstNode = iterDstNode->first;
		}
	// Repeat draws until AddConnection reports success
	} while( not this->AddConnection(srcNode, dstNode, true, newWeight) );
}


void Genome::MutateAddNode(void)
{
	/* An old connection is disabled and two new connections are added to the genome. 
	 * The new connection leading into the new node receives a weight of 1, and 
	 * the new connection leading out receives the same weight as the old connection. 
	 * This method of adding nodes minimizes the initial effect of the mutation.
	 */

	// Pick a random connection.
	// Create a random variable that runs from 0 to #nconnections-1
	// A [0,n-1] range lets us use std::advance more easily later on
	boost::random::uniform_int_distribution<> randomConnection(0, connections.size()-1);

	// Get a connection that isn't disabled
	map<pair<uint16_t,uint16_t>, ConnectionGene>::iterator iterConnection;
	do
	{
		// Pull a random number
		uint16_t rConnId = randomConnection(rng);

		// Get an iterator to the connection
		iterConnection = connections.begin();
		advance(iterConnection, rConnId);
	} while (iterConnection->second.enabled != true);

	// Disable it
	iterConnection->second.enabled = false;

	// Create a new node
	uint16_t newNodeId = this->AddNode(NodeType::HIDDEN);

	// Create connections from and to the new node
	this->AddConnection(iterConnection->second.from_node, newNodeId);
	this->AddConnection(newNodeId, iterConnection->second.to_node, false, iterConnection->second.weight);	
}


/* Auxiliary
 */


void Genome::Print()
{
	// Print a list of nodes
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

	// Print each node's values
	for(map<uint16_t, NodeGene>::const_iterator 
		iterNode = nodes.begin();
		iterNode != nodes.end();
		iterNode++)
		cout << iterNode->first << '\t' << iterNode->second.valueLast << '\t' << iterNode->second.valueNow << '\n';

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


void Genome::PrintToGV(string filename)
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
	gvout << "\tnode [shape=circle,style=filled,fillcolor=white,fontcolor=black];\n";

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