#include "ipasir.h"
#include "picosat.h"

static const char * sig = "picosat" VERSION;

const char * ipasir_signature () { return sig; }

void * ipasir_init () { 
  char prefix[80];
  void * res = picosat_init ();
  sprintf (prefix, "c [%s] ", sig);
  picosat_set_prefix (res, prefix);
  picosat_set_verbosity (res, 1);
  picosat_set_output (res, stdout);
  return res;
}

void ipasir_release (void * solver) {
  picosat_stats (solver);
  picosat_reset (solver);
}

void ipasir_add (void * solver, int32_t lit) { picosat_add (solver, lit); }

void ipasir_assume (void * solver, int32_t lit) { picosat_assume (solver, lit); }

int ipasir_solve (void * solver) { return picosat_sat (solver, -1); }

int ipasir_failed (void * solver, int32_t lit) {
  return picosat_failed_assumption (solver, lit);
}

int ipasir_val (void * solver, int32_t var) {
  int val = picosat_deref (solver, var);
  if (!val) return 0;
  return val < 0 ? -var : var;
}

void
ipasir_set_terminate (
  void * solver,
  void * state, int (*terminate)(void * state)) {
  picosat_set_interrupt (solver, state, terminate);
}

/* Picosat does not implement clause sharing functionality */
void ipasir_set_learn (void * solver, void * state, int max_length, void (*learn)(void * state, int32_t * clause)) {}

