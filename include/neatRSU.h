#ifndef NEATRSU_H_
#define NEATRSU_H_

#include <iostream>
#include <iomanip>
#include <fstream>

#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/bernoulli_distribution.hpp>

using namespace std;


/* Definitions
   ----------- */
extern bool gm_debug;
extern const uint16_t g_inputs;
extern map<uint16_t,string> g_nodeNames;

extern float gm_compat_excess;
extern float gm_compat_disjoint; 
extern float gm_compat_weight; 

extern boost::random::mt19937 					g_rng;
extern boost::random::bernoulli_distribution<> 	g_rnd_5050;
extern boost::random::bernoulli_distribution<> 	g_rnd_mutWeights;
extern boost::random::bernoulli_distribution<> 	g_rnd_addConn;
extern boost::random::bernoulli_distribution<> 	g_rnd_addNode;
extern boost::random::bernoulli_distribution<> 	g_rnd_inheritDisabled;
extern boost::random::normal_distribution<> 	g_rnd_gauss;

#define d_outputnode g_inputs+1
#define d_biasnode g_inputs+2
#define d_firsthidnode g_inputs+3

/* Functions
   --------- */



/* Classes and Structs
   ------------------- */

// An entry in the database.
// '<' overloaded w.r.t. relative_time
class DataEntry
{
public:
	uint16_t	node_id;
	uint32_t	relative_time;
	float		latitude;
	float		longitude;
	uint16_t	speed;
	uint16_t	heading;
	// uint16_t	rsu_id;
	uint32_t	contact_time;
	uint32_t	prediction=0;

	DataEntry(vector<string> in)
	{
		node_id 		= boost::lexical_cast<uint16_t>	(in[0]) ;
		relative_time	= boost::lexical_cast<uint32_t>	(in[1]) ;
		latitude		= boost::lexical_cast<float>	(in[2]) ;
		longitude		= boost::lexical_cast<float>	(in[3]) ;
		speed			= boost::lexical_cast<uint16_t>	(in[4]) ;
		heading			= boost::lexical_cast<uint16_t>	(in[5]) ;
		// rsu_id			= boost::lexical_cast<uint16_t>	(in[6]) ;
        contact_time    = boost::lexical_cast<uint32_t>	(in[7]) ;
	}

    bool operator < (const DataEntry& entry) const
        { return (relative_time < entry.relative_time); }
};


#endif /* NEATRSU_H_ */
