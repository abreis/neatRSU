TODO
====


## Newest

v Save genome to file
v Load genome from file 
-  evaluate DBs on it
v  use as starting point

- Add a mutation that deletes a hidden node and all links going to it (delete or disable?)

v Fitness sharing
v If the maximumFitness of a species does not improve for 15 generations, mate only the top two genes.
  Would need to disable slowStart

v Clear out empty species
x May need to use species' ages to boost adjFitness to allow young species to take hold. lookfor species::adjust_fitness()
  They seem to be taking hold just fine.
v Self-adjusting m_compat_threshold

v Print the best genome across all species each generation, identify its species (to see jumps to a different species)
- Wipe the genome's memory on new nodeID incoming
- Print the champion on every 'N' generations



## Oldest

v boost::program_options
v random number generators
v PrintToGV ignore disabled connections
x Implement no recurrency at first
x If a prediction is zero, don't include it in the fitness?
  This could bias evolution towards complex structures though
  But without it you'll get massive fitness penalties on the first few entries of a new node.
  -> No, direct links to the output will give a rough initial estimate.
v Sort the DBs by NodeID-then-time

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
v Keep track of the best genome across each species every generation
