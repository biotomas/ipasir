#include "ipasir_cpp.h"

namespace ipasir {
	int ipasir_terminate_callback(void* state) {
		return static_cast<Solver*>(state)->terminateCallback();
	}

	int ipasir_select_literal_callback(void* state) {
		return static_cast<Solver*>(state)->selectLiteralCallback();
	}

	void ipasir_learn_callback(void* state, int* clause) {
		static_cast<Solver*>(state)->learnedClauseCallback(clause);
	}

	Solver::Solver():
		solver(nullptr),
		terminateCallback([]{return 0;}),
		selectLiteralCallback([]{return 0;}),
		learnedClauseCallback([](int*){return;}) {

		reset();
	}

	Solver::~Solver(){
		ipasir_release(solver);
	}

	std::string Solver::signature() {
		return ipasir_signature();
	}

	void Solver::add(int lit_or_zero) {
		ipasir_add(solver, lit_or_zero);
	}

	void Ipasir::addClause(std::vector<int> clause) {
		for (int literal: clause) {
			this->add(literal);
		}
		this->add(0);
	}

	void Solver::assume(int lit) {
		ipasir_assume(solver, lit);
	}

	SolveResult Solver::solve() {
		return static_cast<SolveResult>(ipasir_solve(solver));
	}

	int Solver::val(int lit) {
		return ipasir_val (solver, lit);
	}

	int Solver::failed (int lit) {
		return ipasir_failed(solver, lit);
	}

	void Solver::set_terminate (std::function<int(void)> callback) {
		terminateCallback = callback;
		ipasir_set_terminate(this->solver, this, &ipasir_terminate_callback);
	}

	void Solver::set_learn (int max_length, std::function<void(int*)> callback) {
		learnedClauseCallback = callback;
		ipasir_set_learn(this->solver, this, max_length, &ipasir_learn_callback);
	}

	void Solver::reset() {
		if (solver != nullptr) {
			ipasir_release(solver);
		}
		solver = ipasir_init();

		set_terminate([]{return 0;});
		set_learn(0,[](int*){return;});
		#ifdef USE_EXTENDED_IPASIR
		eipasir_set_select_literal_callback(this->solver, this, &ipasir_select_literal_callback);
		#endif
	}
}
