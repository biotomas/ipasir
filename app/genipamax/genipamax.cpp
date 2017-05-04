/* author: Tomas Balyo, KIT, Karlsruhe */
#include <stdio.h>
#include "pblib/pb2cnf.h"
#include "pblib/clausedatabase.h"
#include <vector>

using namespace std;
using namespace PBLib;

// The linked SAT solver might be written in C
extern "C" {
#include "ipasir.h"
}

/**
 * Read a partial maxsat problem from the specified file and add it to the solver
 * with activation literals for the soft clauses. The names of the activation
 * literals are *startVar, ..., *endVar. Returns false if the reading was not
 * successful.
 */
bool readMaxSatProblem(const char* filename, void* solver, int* startActVar, int* endActVar) {
	FILE* f = fopen(filename, "r");
	if (f == NULL) {
		return false;
	}
	int vars, cls, top;
	int c = 0;
	bool neg = false;
	bool first = true;

	while (c != EOF) {
		c = fgetc(f);

		// problem definition line
		if (c == 'p') {
			char pline[512];
			int i = 0;
			while (c != '\n') {
				pline[i++] = c;
				c = fgetc(f);
			}
			pline[i] = 0;
			if (3 != sscanf(pline, "p wcnf %d %d %d", &vars, &cls, &top)) {
				printf("Failed to parse the problem definition line (%s)\n", pline);
				return false;
			}
			vars++;
			*startActVar = vars;
			continue;
		}
		// comment
		if (c == 'c') {
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
			// the first number is the weight
			if (first) {
				first = false;
				// this is a soft clause
				if (num == 1) {
					// will add the next activation literal instead of num
					ipasir_add(solver, vars++);
				}
				continue;
			}
			if (num == 0) {
				first = true;
			}
			// add to the solver
			ipasir_add(solver, num);
		}
	}
	fclose(f);
	*endActVar = vars;
	return true;
}

/**
 * This class is used by the PBLib library to add clauses 
 * of the cardinality constraint to the solver. See the PBLib
 * documentation for more details.
 */
class IpasirClauseDatabase : public ClauseDatabase {
public:
	IpasirClauseDatabase(PBConfig config, void* solver):ClauseDatabase(config),solver(solver) {
	}
protected:
	void* solver;
	void addClauseIntern(const vector<int>& clause) {
		for (size_t i = 0; i < clause.size(); i++) {
			ipasir_add(solver, clause[i]);
		}
		ipasir_add(solver, 0);
	}
};


int main(int argc, char **argv) {
	void * solver = ipasir_init();
	int startActVar, endActVar;

	if (!readMaxSatProblem(argv[1], solver, &startActVar, &endActVar)) {
		puts("Input could not be parsed");
		return 0;
	}

	printf("c The input problem has %d soft clauses.\n", endActVar - startActVar);

	// find an initial solution
	int res = ipasir_solve(solver);
	if (res == 20) {
		puts("Hard clauses are UNSAT");
		return 20;
	}

	// count the number of unsat soft clauses in the initial solution
	// and start constructing a cardinality constraint on the activation
	// literals of all soft clauses.
	int unsatisfiedSoft = 0;
	vector<WeightedLit> lits;
	for (int i = startActVar; i < endActVar; i++) {
		lits.push_back(WeightedLit(i,1));
		if (ipasir_val(solver, i) > 0) {
			unsatisfiedSoft++;
		}
	}
	printf("c initial bound\no %d\n", unsatisfiedSoft);
	int bestResult = unsatisfiedSoft;

	// PBLib initialization code
	AuxVarManager avm(endActVar);
	PBConfig config = make_shared<PBConfigClass>();
	PB2CNF convertor(config);
	IpasirClauseDatabase icd(config, solver);

	// create the first cardinality constraint stating
	// that the number of unsat soft clauses must be smaller than
	// in the initial solution.
	unsatisfiedSoft--;
	IncPBConstraint pbc(lits, LEQ, unsatisfiedSoft);
	convertor.encodeIncInital(pbc, icd, avm);

	// keep strengthening the bound and solving
	// until we reach an unsat formula.
	res = ipasir_solve(solver);
	while (res != 20) {
		// count the unsat soft clauses in the last solution
		unsatisfiedSoft = 0;
		for (int i = startActVar; i < endActVar; i++) {
			if (ipasir_val(solver, i) > 0) {
				unsatisfiedSoft++;
			}
		}
		printf("o %d\n", unsatisfiedSoft);
		bestResult = unsatisfiedSoft;
		// strenghten the cardinality constraint
		unsatisfiedSoft--;
		pbc.encodeNewLeq(unsatisfiedSoft, icd, avm);
		res = ipasir_solve(solver);
	}
	puts("s OPTIMUM FOUND");
	printf("c final-result %d\n", bestResult);
}
