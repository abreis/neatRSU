#ifndef NEURAL_H_
#define NEURAL_H_


/* Definitions
   ----------- */

enum NodeType { sensor, hidden, output };




/* Functions
   --------- */

// Custom sigmoidal transfer function.
double ActivationSigmoid (double input)
{
	// TODO verify working
	/* The steepened sigmoid allows more fine tuning at extreme activations. 
	 * It is optimized to be close to linear during its steepest ascent 
	 * between activations −0.5 and 0.5.
	 */
	 return ( 1.0/( 1.0+exp(-4.9*input) ) );
}

// Output transfer function.
double ActivationOutput (double input)
{
	// Linear.
	return input;
}


// Feed data into a neural network and evaluate the fitness of the output.
// uint32_t EvaluateFitness(vector<DataEntry> data, -neural network-);


/* Classes and Structs
   ------------------- */



#endif /* NEURAL_H_ */
