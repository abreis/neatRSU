#ifndef NEURAL_H_
#define NEURAL_H_

#include "neatRSU.h"
#include <cmath>


/* Definitions
   ----------- */

enum NodeType { sensor, hidden, output };




/* Functions
   --------- */

// Custom sigmoidal transfer function.
double ActivationSigmoid (double input);

// Output transfer function.
double ActivationOutput (double input);


// Feed data into a neural network and evaluate the fitness of the output.
// uint32_t EvaluateFitness(vector<DataEntry> data, -neural network-);


/* Classes and Structs
   ------------------- */



#endif /* NEURAL_H_ */
