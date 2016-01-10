#include "neatRSU.h"
#include "neural.h"
#include "genetic.h"

#define TRAIN_DATA	"./node_week4_day23.csv"
#define TEST_DATA	"./node_week3_day16.csv"

#define P_MUTATE_WEIGHT		0.50
#define P_MUTATE_ADD_CONN	0.01
#define P_MUTATE_ADD_NODE	0.01

int main(int argc, char *argv[])
{
	/* Overall structure:
	 * - Load data into memory, from CSV files, using Boost::tokenizer
	 * - Set up neural network scaffolds for genetic evolution
	 * - Evolve network on training data, until criteria met or user input
	 * - Output resulting network structure and weights
	 * - Test on both training data and test data, and output
	 */

	/* 01 Load training data */
 	cout << "Loading training data on " << TRAIN_DATA << " into memory... " << flush;

	ifstream trainDataIn(TRAIN_DATA);
	if (!trainDataIn.is_open()) return 1;

	// Database to store training data
	vector<DataEntry> TrainingDB;

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
	cout << "done." << endl;
	cout << "Loaded " << TrainingDB.size() << " entries into memory." << endl;


	/* 02 Load test data data */
	// TODO

	


	return 0;
}