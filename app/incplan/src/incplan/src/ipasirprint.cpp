extern "C" {
	#include "ipasir/ipasir.h"
}

#include <iostream>
#include <limits>
#include "misc.h"

using namespace incplan;

extern "C" {
	#define SAT 10
	#define UNSAT 20
	#define TIMEOUT 0

	const char * ipasir_signature (){
		return "printing all clauses";
	}

	void * ipasir_init (){
		return nullptr;
	}

	void ipasir_release (void * solver){
		UNUSED(solver);
	}

	void ipasir_add (void * solver, int lit_or_zero){
		UNUSED(solver);
		std::cout << lit_or_zero << " ";
		if (lit_or_zero == 0) {
			std::cout << std::endl;
		}
	}

	void ipasir_assume (void * solver, int lit){
		UNUSED(solver);
		std::cout << "a" << lit << " ";
	}

	int ipasir_solve (void * solver){
		UNUSED(solver);
		std::cout << " solved? 0/1 [0]: ";
		bool solved = false;
		if (std::cin.peek() == '\n') { //check if next character is newline
			solved = false; //and assign the default
		} else if (!(std::cin >> solved)) { //be sure to handle invalid input
			std::cout << "Invalid input. Using default.\n";
			std::cin.clear();
		//error handling
		}

		std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
		if (solved) {
			return SAT;
		} else {
			return UNSAT;
		}
	}

	int ipasir_val (void * solver, int lit){
		UNUSED(solver);
		UNUSED(lit);
		return 0;
	}


	int ipasir_failed (void * solver, int lit){
		UNUSED(solver);
		UNUSED(lit);
		return 0;
	}

	void ipasir_set_terminate (void * solver, void * state, int (*terminate)(void * state)){
		UNUSED(solver);
		UNUSED(state);
		UNUSED(terminate);
	}

	void ipasir_set_learn (void * solver, void * state, int max_length, void (*learn)(void * state, int * clause)){
		UNUSED(solver);
		UNUSED(state);
		UNUSED(max_length);
		UNUSED(learn);
	}

}