TODO
====

v boost::program_options
v random number generators
v PrintToGV ignore disabled connections
x Implement no recurrency at first
- If a prediction is zero, don't include it in the fitness?
  This could bias evolution towards complex structures though
- Sort the DBs by NodeID-then-time. Wipe the genome's memory 

Neural
------
v Implement BIAS node type. Bias node is a disconnected input, and NEAT should add connections as needed.
   Not all nodes need a Bias. 
v Run a DB through a genome and fill its predicted time-to-contact field
   This might be better optimized by evaluating the goodness-of-fit as the predictions are computed,
   instead of storing everything in the database and then evaluating.
v Apply a single data entry through a Genome and get the output (Activation)

v Single node evolution
v Weight mutations
v Structural mutations - AddNode
v Structural mutations - AddConnection
v Innovation numbers
v Crossovers
- Fixed-size species evolution
- Difference between two genomes (for speciation)

- Keep track of the best genome across each species every generation
- Print the best genome across all species each generation, identify its species (to see jumps to a different species)

- Go through a DB and evaluate the goodness-of-fit from real contact to estimated contact time. (?)


Cell maps (throw out)
---------
- Routine to build cell map at any instant
- Routine to fill all DB entries with corresponding cell maps

Tentative
---------
- A routine to save and load Genomes to files
- Implement no recurrency (need special addconnection() and activate()) and test pure regression