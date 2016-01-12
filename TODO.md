TODO
====

v boost::program_options
v random number generators
v PrintToGV ignore disabled connections
- Implement no recurrency at first

Neural
------
- Implement BIAS node type. Bias node is a disconnected input, and NEAT should add connections as needed.
-- Not all nodes need a Bias. 
- Apply a single data entry through a Genome and get the output
- Run a DB through a genome and fill its predicted time-to-contact field
-- This might be better optimized by evaluating the goodness-of-fit as the predictions are computed,
   instead of storing everything in the database and then evaluating.
- Go through a DB and evaluate the goodness-of-fit from real contact to estimated contact time. (?)

Genetic
-------
- Difference between two genomes (for speciation)

Cell maps
---------
- Routine to build cell map at any instant
- Routine to fill all DB entries with corresponding cell maps

Tentative
---------
- A routine to save and load Genomes to files