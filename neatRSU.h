#ifndef NEATRSU_H_
#define NEATRSU_H_

#include <iomanip>
#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/bernoulli_distribution.hpp>

#include <string>
#include <vector>


using namespace std;



/* Definitions
   ----------- */



/* Functions
   --------- */



/* Classes and Structs
   ------------------- */

// An entry in the database.
// '<' overloaded w.r.t. relative_time
struct DataEntry
{
	uint16_t	node_id;
	uint32_t	relative_time;
	float		latitude;
	float		longitude;
	uint16_t	speed;
	uint16_t	heading;
	// uint16_t	rsu_id;
	uint32_t	contact_time;

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
