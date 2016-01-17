TODO
====

v boost::program_options
v random number generators
v PrintToGV ignore disabled connections
x Implement no recurrency at first
x If a prediction is zero, don't include it in the fitness?
  This could bias evolution towards complex structures though
  But without it you'll get massive fitness penalties on the first few entries of a new node.
  -> No, direct links to the output will give a rough initial estimate.
v Sort the DBs by NodeID-then-time
- Wipe the genome's memory on new nodeID incoming

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
v Species reprodution
v Update fitness statistics on genomes/species/population

v Difference between two genomes (for speciation)
- Fitness sharing
- If the maximumFitness of a species does not improve for 15 generations, stop reprodution.

v Keep track of the best genome across each species every generation
- Print the best genome across all species each generation, identify its species (to see jumps to a different species)


Tentative
---------
- A routine to save and load Genomes to files
- Go through a DB and evaluate the goodness-of-fit from real contact to estimated contact time
- Implement no recurrency (need special addconnection() and activate()) and test pure regression
