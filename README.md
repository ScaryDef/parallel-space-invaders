# parallel-space-invaders
An attempt to parallelize the popular 80's game space invader. Part of a project for a Computer Organization class.

The base part of the game is based on a normal (non-parallelized) version of space invaders, made by ___, my team and I 
added the parallelism that allows each alien to run on a different core using OpenMPI.

The project had several difficulties, mostly the fact that printing to cli from each core was too difficult to coordinate
originally, and we were forced to pass every single alien's movements to the master core, who was then responsible for 
printing the new position for each alien on the cli.

As for what needs to be improved, I see one major issue that needs work: proper printing to cli.

Right now most bombs dropped by the aliens are not moved correctly, and end up duplicating and sometimes creating a massive
amount of bombs to show up on a single column, making the game harder than it really has to be. 
