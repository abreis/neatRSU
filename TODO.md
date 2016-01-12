TODO
====

v boost::program_options
v random number generators
v PrintToGV ignore disabled connections
x Implement no recurrency at first
- If a prediction is zero, don't include it in the fitness?
  This could bias evolution towards complex structures though

Neural
------
v Implement BIAS node type. Bias node is a disconnected input, and NEAT should add connections as needed.
   Not all nodes need a Bias. 
v Run a DB through a genome and fill its predicted time-to-contact field
   This might be better optimized by evaluating the goodness-of-fit as the predictions are computed,
   instead of storing everything in the database and then evaluating.
- Apply a single data entry through a Genome and get the output (Activation)

1- Single node evolution (weight mutations)
2- Fixed-size species evolution
3- Structural mutations
4- Crossovers
5- Structural differences for speciation


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