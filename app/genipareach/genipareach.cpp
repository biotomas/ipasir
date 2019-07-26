/*
 * Simple reachability solver for DIMSPEC files
 *
 *  Created on: May 3, 2016
 *      Author: Balyo
 */

extern "C" {
#include "ipasir.h"
}
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <vector>

using namespace std;

struct Formula {
	int variables;
	int clausesCount;
	vector<vector<int> > clauses;
	vector<int> tmpClause;

	Formula():variables(0),clausesCount(0) {
	};

	void addLit(int lit) {
		if (lit == 0) {
			clausesCount++;
			clauses.push_back(tmpClause);
			tmpClause.clear();
		} else {
			tmpClause.push_back(lit);
		}
	}
};

int readNextNumber(FILE* f, int c) {
	while (!isdigit(c)) {
		c = fgetc(f);
	}
	int num = 0;
	while (isdigit(c)) {
		num = num*10 + (c-'0');
		c = fgetc(f);
	}
	return num;
}

void readLine(FILE* f) {
	int c = fgetc(f);
	while(c != '\n') {
		c = fgetc(f);
	}
}

bool loadReachabilityProblem(const char* filename, Formula* initial, Formula* universal, Formula* goal, Formula* transition) {
	FILE* f = fopen(filename, "r");
	if (f == NULL) {
		return false;
	}
	int c = 0;
	bool neg = false;
	Formula* current = goal;
	while (c != EOF) {
		c = fgetc(f);

		// comment line
		if (c == 'c') {
			readLine(f);
			continue;
		}
		// problem lines
		if (c == 'i' || c == 'u' || c == 'g' || c == 't') {
			switch (c) {
			case 'i':
				current = initial;
				break;
			case 'u':
				current = universal;
				break;
			case 'g':
				current = goal;
				break;
			case 't':
				current = transition;
				break;
			default:
				printf("Invalid character: \"%c\"\n", c);
				return false;
			}
			current->variables = readNextNumber(f, 0);
			readLine(f);
			continue;
		}
		// whitespace
		if (isspace(c)) {
			continue;
		}
		// negative
		if (c == '-') {
			neg = true;
			continue;
		}

		// number
		if (isdigit(c)) {
			int num = readNextNumber(f, c);
			if (neg) {
				num *= -1;
			}
			neg = false;
			current->addLit(num);
		}
	}
	fclose(f);

	return true;
}

void addClauses(void* solver, const vector<vector<int> >& clauses, int variableOffset) {
	for (size_t cid = 0; cid < clauses.size(); cid++) {
		if (clauses[cid].size() > 0) {
			for (size_t lid = 0; lid < clauses[cid].size(); lid++) {
				int lit = clauses[cid][lid];
				if (lit > 0) {
					ipasir_add(solver, lit+variableOffset);
					//printf("added %d\n", lit+variableOffset);
				} else {
					ipasir_add(solver, lit-variableOffset);
					//printf("added %d\n", lit-variableOffset);
				}
			}
			ipasir_add(solver, 0);
			//printf("added %d\n", 0);
		}
	}
}

void assumeLits(void* solver, const vector<int>& lits, int variableOffset) {
	for (size_t lid = 0; lid < lits.size(); lid++) {
		int lit = lits[lid];
		if (lit > 0) {
			ipasir_assume(solver, lit+variableOffset);
		} else {
			ipasir_assume(solver, lit-variableOffset);
		}
	}
}

int main(int argc, char **argv) {
	puts("c this is a trivial DIMSPEC formula solver.");

	if (argc != 2) {
		puts("c USAGE: ./reachability <dimspec.srt>");
		return 0;
	}

	Formula init,univ,trans, goal;
	loadReachabilityProblem(argv[1], &init, &univ, &goal, &trans);
	printf("c variables in initial %d universal %d goal %d transition %d\n", init.variables, univ.variables, goal.variables, trans.variables);
	printf("c clauses in initial %lu universal %lu goal %lu transition %lu\n", init.clauses.size(), univ.clauses.size(), goal.clauses.size(), trans.clauses.size());
	printf("c Using the incremental SAT solver %s.\n", ipasir_signature());

	int baseVariables = init.variables;
	if (univ.variables != baseVariables || goal.variables != baseVariables || trans.variables != 2*baseVariables) {
		printf("Error: Variable number inconsistency in the input formula.\n");
		return 1;
	}
	vector<int> goalLiterals;
	for (size_t cid = 0; cid < goal.clauses.size(); cid++) {
		if (goal.clauses[cid].size() > 1) {
			printf("Error: only unit goal clauses supported, encode your problem accordingly.\n");
			return 1;
		} else if (goal.clauses[cid].size() > 0) {
			goalLiterals.push_back(goal.clauses[cid][0]);
		}
	}

	void* solver = ipasir_init();
	// Add the initial conditions
	addClauses(solver, init.clauses, 0);

	for (int step = 0;;step++) {
		printf("c running step nr. %d\n", step);
		addClauses(solver, univ.clauses, step*baseVariables);
		if (step > 0) {
			addClauses(solver, trans.clauses, (step-1)*baseVariables);
		}
		assumeLits(solver, goalLiterals, step*baseVariables);
		int res = ipasir_solve(solver);
		if (res == 10) {
			printf("c solved at step nr. %d\n", step);
			break;
		}
	}
	ipasir_release(solver);
	return 0;
}
