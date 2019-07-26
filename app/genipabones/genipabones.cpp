/*
 * essentials.cpp
 *
 *  Created on: Feb 3, 2015
 *      Author: Tomas Balyo, KIT
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <signal.h>
#include <unistd.h>

using namespace std;

extern "C" {
	#include "ipasir.h"
}
/**
 * Reads a formula from a given file and transforms it using the dual rail encoding,
 * i.e., replaces each x by px and each \overline{x} by nx. Also adds clauses
 * of the form (\overline{px} \vee \overline{nx})
 */
bool loadFormula(void* solver, const char* filename, vector<int>& boundary, int& maxVar) {
	FILE* f = fopen(filename, "r");
	if (f == NULL) {
		return false;
	}
	maxVar = 0;
	int c = 0;
	bool neg = false;
	while (c != EOF) {
		c = fgetc(f);

		// comment or problem definition line
		if (c == 'c' || c == 'p') {
			c = fgetc(f);
			if (c == 'v' && fgetc(f) == 'i' && fgetc(f) == 'p' && fgetc(f) == ' ') {
				// boundary variables
				printf("c back-bone candidates specified in input file.\n");
				int num = 0;
				while(c != '\n') {
					c = fgetc(f);
					while (isdigit(c)) {
						num = num*10 + (c-'0');
						c = fgetc(f);
					}
					boundary.push_back(num);
					num = 0;
				}
			}
			// skip this line
			while(c != '\n') {
				c = fgetc(f);
			}
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
			int num = c - '0';
			c = fgetc(f);
			while (isdigit(c)) {
				num = num*10 + (c-'0');
				c = fgetc(f);
			}
			if (neg) {
				num *= -1;
			}
			neg = false;

			if (abs(num) > maxVar) {
				maxVar = abs(num);
			}
			// add to the solver
			ipasir_add(solver, num);
		}
	}
	fclose(f);
	return true;
}

int terminateFlag = 0;

int terminateCallback(void * state) {
	return terminateFlag;
}

void watchdog(int sig) {
	terminateFlag = 1;
}

int main(int argc, char **argv) {
	printf("c This program finds back-bones of a CNF SAT formula. A back-bone is assignment fixed in all solutions.\n");
	printf("c You can specify backbone candidates by adding a line starting with 'cvip' followed by variables names separated by space to the cnf file.\n");
	printf("c You can specify a time limit (in seconds) for checking one backbone using the -t parameter.\n");
	printf("c Using the incremental SAT solver %s.\n", ipasir_signature());


	if (argc < 2) {
		puts("USAGE: ./bbones <dimacs.cnf> [-t=<time-limit-per-check>]");
		return 0;
	}

	void *solver = ipasir_init();
	vector<int> boundary;
	int maxVar;
	bool loaded = loadFormula(solver, argv[1], boundary, maxVar);

	if (!loaded) {
		printf("The input formula \"%s\" could not be loaded.\n", argv[1]);
		return 0;
	}

	if (boundary.size() == 0) {
		boundary.clear();
		for (int var = 1; var <= maxVar; var++) {
			boundary.push_back(var);
		}
		printf("c Will check all the variables (%d) for backbones.\n", maxVar);
	} else {
		printf("c Will check %lu candidates for backbones.\n", boundary.size());
	}

	int timelimit = 0;
	if ((argc > 2 && argv[2][1] == 't')) {
		timelimit = atoi(argv[2]+3);
		printf("c Time limit per check is %d seconds\n", timelimit);
		ipasir_set_terminate(solver, NULL, terminateCallback);
		signal(SIGALRM, watchdog);
	}

	int bbonesFound = 0;

	for (size_t i = 0; i < boundary.size(); i++) {
		int candidate = boundary[i];
		printf("c Checking canidate %d, (nr. %lu of %lu)\n", candidate, i+1, boundary.size());
		// check positive case
		ipasir_assume(solver, candidate);
		terminateFlag = 0;
		alarm(timelimit);
		int res = ipasir_solve(solver);
		if (res == 20) {
			bbonesFound++;
			printf("%d 0\n", -candidate);
			continue;
		}
		alarm(0);

		// check negative case
		ipasir_assume(solver, -candidate);
		terminateFlag = 0;
		alarm(timelimit);
		res = ipasir_solve(solver);
		if (res == 20) {
			bbonesFound++;
			printf("%d 0\n", candidate);
		}
		alarm(0);
	}

	printf("c Found %d backbones\n", bbonesFound);
}
