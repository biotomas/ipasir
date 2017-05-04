/* Author: Tomas Balyo, KIT, Karlsruhe */

#include <stdio.h>
#include <stdlib.h>
#include "Threading.h"
#include <algorithm>
#include <vector>
#include <ctype.h>

// The linked SAT solver might be written in C
// while this application is written in C++
extern "C" {
#include "ipasir.h"
}

using namespace std;

// Parse a dimacs cnf formula from a given file and
// save its clauses into a given vector.
bool loadFormula(vector<vector<int> >& clauses, const char* filename) {
	FILE* f = fopen(filename, "r");
	if (f == NULL) {
		return false;
	}
	int c = 0;
	bool neg = false;
	vector<int> cls;
	while (c != EOF) {
		c = fgetc(f);
		// comment or problem definition line
		if (c == 'c' || c == 'p') {
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
			if (num != 0) {
				cls.push_back(num);
			} else {
				clauses.push_back(vector<int>(cls));
				cls.clear();
			}
		}
	}
	fclose(f);
	return true;
}

// This global variable is used to store the result of the solving
// and also to signal that all the SAT solving threads can stop.
int result = 0;

// This function is called by the SAT solvers from different threads
// to determine if they should abort solving of the formula.
// A non-zero value indicates that the solver should abort (in accordance with
// the ipasir definition).
int terminator(void* state) {
	return result;
}

// Each thread is running this function which runs ipasir_solve
void* solverThread(void* solver) {
	int res = ipasir_solve(solver);
	printf("c [genipafolio] solver stopped, res = %d\n", res);
	if (res != 0) {
		result = res;
	}
	return NULL;
}

// Returns a random number between 0 and Max
int randGen(int Max) {
	return rand() % Max;
}

int main(int argc, char** argv) {

	puts("c [genipafolio] USAGE: ./pfolio dimacs.cnf [#threads=4]");

	srand(2015);
	char* filename = argv[1];
	int cores = 4;
	if (argc > 2) {
		cores = atoi(argv[2]);
	}

	void** solvers = (void**)malloc(cores*sizeof(void*));
	Thread** threads = (Thread**)malloc(cores*sizeof(Thread*));

	printf("c [genipafolio] Solving %s with %d cores using %s and shuffling.\n", filename, cores, ipasir_signature());

	vector<vector<int> > fla;
	loadFormula(fla, filename);

	for (int i = 0; i < cores; i++) {
		// initialize the solver
		solvers[i] = ipasir_init();
		// set temination callback
		ipasir_set_terminate(solvers[i], NULL, terminator);
		// add the clauses (might be shuffled)
		for (size_t j = 0; j < fla.size(); j++) {
			vector<int>& cls = fla[j];
			for (size_t k = 0; k < cls.size(); k++) {
				ipasir_add(solvers[i], cls[k]);
			}
			ipasir_add(solvers[i], 0);
		}
		// start the solver
		threads[i] = new Thread(solverThread, solvers[i]);
		// shuffle the clauses for the next solver
		random_shuffle(fla.begin(), fla.end(), randGen);
	}
	// wait for each solver to stop and release them
	for (int i = 0; i < cores; i++) {
		threads[i]->join();
		ipasir_release(solvers[i]);
	}
	free(solvers);
	free(threads);
	printf("c [genipafolio] All done, result = %d\n", result);
	return result;
}
