/*
 * Lingeling.cpp
 *
 *  Created on: Dec 4, 2014
 *      Author: balyo
 */


#include "ipasir.h"

extern "C" {
	#include "lingeling-bcj/lglib.h"
}

const char* ipasir_signature() {
	return lglversion();
}

void* ipasir_init() {
	return lglinit();
}

void ipasir_release(void* solver) {
	lglrelease((LGL*)solver);
}

void ipasir_add(void* solver, int32_t lit) {
	if (lit != 0) {
		lglfreeze((LGL*)solver, lit);
	}
	lgladd((LGL*)solver, lit);
}

void ipasir_assume(void* solver, int32_t lit) {
	lglfreeze((LGL*)solver, lit);
	lglassume((LGL*)solver, lit);
}

int ipasir_solve(void* solver) {
	return lglsat((LGL*)solver);
}

int ipasir_val(void * solver, int32_t var) {
	return var*lglderef((LGL*)solver, var);
}

int ipasir_failed(void * solver, int32_t lit) {
	return lglfailed((LGL*)solver, lit);
}

void ipasir_set_terminate(void * solver,  void * state, int (*terminate)(void * state)) {
	lglseterm((LGL*)solver, terminate, state);
}

void ipasir_set_learn (void * solver, void * state, int max_length, void (*learn)(void * state, int32_t * clause)) {
	//not implemented
}
