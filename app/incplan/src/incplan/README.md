# incplan
Tool to do SAT Based Planning using an incremental SAT solver.

## Preparation
Get a sat solver implementing the ipasir api.
I.e. from http://baldur.iti.kit.edu/sat-race-2015/downloads/ipasir.zip
Drop the sat solver library implementing the ipasir api in lib/ipasir

## Building
```
cd build
cmake ../src
make
```

The binaries will now be in bin/

## Running
To show the help you can run without parameters:
```
incplan-[solvername]
```
To run with default settings:
```
incplan-[solvername] [problemFile]
```

The problem file should be encoded in dimspec. It contains four sections:
```
i cnf [numberVariables] [numberclauses]
[initial clauses in dimacs cnf]
u cnf [numberVariables] [numberclauses]
[universal/invariant clauses in dimacs cnf]
g cnf [numberVariables] [numberclauses]
[goal clauses in dimacs cnf]
t cnf [numberVariables] [numberclauses]
[transition clauses in dimacs cnf]
```
The order of iugt is relevant. The number of variables/ clauses is only for the
section.

## Experiments

For information about the experiments of our icaps17 paper submission see
experiment/icaps17/README
