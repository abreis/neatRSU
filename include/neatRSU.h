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
extern uint32_t g_generationNumber;
extern const uint16_t g_inputs;
extern map<uint16_t,string> g_nodeNames;
// Warning: these are IDs (suitable for e.g. std::map), not vector indices
extern uint16_t d_outputnode;
extern uint16_t d_biasnode;
extern uint16_t d_firsthidnode;

extern float gm_compat_excess;
extern float gm_compat_disjoint; 
extern float gm_compat_weight; 

extern float g_m_p_mutate_weights;
extern float g_m_p_mutate_addnode;
extern float g_m_p_mutate_addconn;
extern float g_m_p_weight_perturb_or_new; 	
extern float g_m_p_inherit_disabled;
extern float g_m_p_mutateOnly;
extern float g_m_p_mateOnly;


extern boost::random::mt19937 					g_rng;
extern boost::random::normal_distribution<> 	g_rnd_gauss;



/* Functions
   --------- */
// Given a probability, returns a Bernoulli outcome.
bool OneShotBernoulli(float probability);

// Auxiliary for std::sort, will sort a database by nodeID, then time.
class DataEntry;
bool sortIdThenTime( DataEntry const &first, DataEntry const &second );


/* Classes and Structs
   ------------------- */

// An entry in the database.
// '<' overloaded w.r.t. time
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
