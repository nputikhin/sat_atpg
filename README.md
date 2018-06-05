# SAT-based ATPG using TG-Pro model

This is an implementation of SAT-based Automatic Test Pattern Generator for single stuck-at faults that uses TG-Pro model with several modifications:
* XOR gate is not expanded and uses XOR as sensitization constraint
* Partial circuit CNF is used for good clause set if only some of primary outputs are needed for fault propagation (controlled by threshold)
* [CaDiCaL](https://github.com/arminbiere/cadical) is used for SAT solving


Fault detection will run on fault list with equivalent faults collapsed.

We do not use TG-Pro-ALL optimizations and there is no fault simulation and structural ATPG engine like in TG-System.

[TG-Pro](http://core.di.fc.ul.pt/wiki/doku.php?id=tg-pro) is described in this article:  
Chen, Huan, and Joao Marques-Silva. "A two-variable model for SAT-based ATPG." IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems 32, no. 12 (2013): 1943-1956.

## Building
Linux only, will probably compile on Windows if you set USE_CADICAL to 0, but you'll need to provide alternative solver or modify the code and tests to work without the solver.

Build with [cmake](https://cmake.org/):

    mkdir _build && cd _build
    cmake --build .
    
## Running
Only bench format is accepted:

    _build/bin/atpgSat *.bench

You can find the instances here:
* [ISCAS'85](http://www.pld.ttu.ee/~maksim/benchmarks/iscas85/bench/)
* [ISCAS'89](http://www.pld.ttu.ee/~maksim/benchmarks/iscas89/bench/)
* [ITC'99](http://www.pld.ttu.ee/~maksim/benchmarks/iscas99/bench/)
