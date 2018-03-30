# jitter-measurement-application

This repository is to demonstrate partial code of jitter measurement application since this is a collaborated research project.

Paper reference:
- http://ieeexplore.ieee.org/document/7151516/#full-text-section
- http://ieeexplore.ieee.org/document/7520532/#full-text-section



### LIDE-C

This directory is to show the C implementation of DVL and TRT actor.

Both actors are designed and implemented as abstract data type (ADT). 

lide_c_DVL.c and lide_c_DVL.h are source files for DVL actor ADT.

Simiarly, lide_c_TRT.c and lide_c_TRT.h are source files for TRT actor ADT.

dlcconfig file is the configuration file for automatically genearation compiling command using DICE which is software tool developed in DSPCAD research group.

makeme file is the script to call utility in DICE to genearte compiling command and execute that command.

ADT here is very similar to class in C++. To use ADT, call the new function in it to instantiate; enable function is to check whether the actor is enabled; invoke function is to execute computational task in this actor; terminate function is to free memory allocated.

Some other header files mentioned in the source code is from LIDE-C package which is software tools for dataflow implementation. It contains pre-designed library for dataflow elements implementation.

### LIDE-OCL

This directory is to show the C++/OpenCL implementation of DVL and TRT actor.

Both actors are designed and implemented as abstract data type (ADT). 
