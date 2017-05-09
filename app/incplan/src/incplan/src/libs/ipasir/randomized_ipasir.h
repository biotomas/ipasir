#include "ipasir_cpp.h"
#include <vector>

#include <algorithm>
#include <random>

namespace ipasir {
class RandomizedSolver : public Ipasir {
public:
	RandomizedSolver(unsigned seed, std::unique_ptr<Ipasir> _solver = std::make_unique<ipasir::Solver>()):
			solver(std::move(_solver)) {
		std::cout << "c [randomizedIpasir] seed: " << seed << std::endl;
		g = std::mt19937(seed);
		init();
	}

	RandomizedSolver(std::unique_ptr<Ipasir> _solver = std::make_unique<ipasir::Solver>()):
		RandomizedSolver(std::random_device()(), std::move(_solver)){
	}

	virtual std::string signature(){
		return solver->signature();
	};

	virtual void add(int lit_or_zero) {
		if (lit_or_zero == 0) {
			clauses.push_back(std::vector<int>());
		} else {
			clauses.back().push_back(lit_or_zero);
		}
	}

	virtual void assume(int lit) {
		assumptions.push_back(lit);
	}

	virtual int val(int lit) {
		return unmap(solver->val(map(lit)));
	};

	virtual int failed (int lit) {
		return solver->failed(map(lit));
	};

	virtual void set_learn(int max_length, std::function<void(int*)> callback) {
		this->learnedClauseCallback = callback;
		solver->set_learn(
			max_length,
			std::bind(
				&RandomizedSolver::mappingCallback,
				this,
				std::placeholders::_1));
	}

	virtual void mappingCallback(int* clause){
		std::vector<int> mappedClause;
		for (;*clause != 0; clause++) {
			mappedClause.push_back(unmap(*clause));
		}
		mappedClause.push_back(0);

		learnedClauseCallback(mappedClause.data());
	}

	virtual SolveResult solve() {
		scrumbleVariables();
		scrumbleClauses();

		for (std::vector<int>& clause: clauses) {
			for (int lit: clause) {
				solver->add(map(lit));
			}
			solver->add(0);
		}
		clauses.clear();
		clauses.push_back(std::vector<int>());

		for (int literal:assumptions) {
			solver->assume(map(literal));
		}
		assumptions.clear();

		return solver->solve();
	}

	virtual void set_terminate (std::function<int(void)> callback) {
		solver->set_terminate(callback);
	};

	virtual void reset() {
		solver->reset();
		clauses.clear();
		assumptions.clear();
		toIpasir.clear();
		fromIpasir.clear();
		knownVariables.clear();
		init();
	};

private:
	std::unique_ptr<Ipasir> solver;

	std::vector<std::vector<int>> clauses;
	std::vector<int> assumptions;

	std::vector<unsigned> toIpasir;
	std::vector<unsigned> fromIpasir;
	std::set<int> knownVariables;
	std::mt19937 g;

	std::function<void(int*)> learnedClauseCallback;

	void init() {
		clauses.push_back(std::vector<int>());
		toIpasir.push_back(0);
		fromIpasir.push_back(0);
	}

	void addLiteral(std::vector<unsigned>& newVariables, unsigned& maxVariable, int lit) {
		unsigned var = litToVar(lit);
		if (var > maxVariable) {
			maxVariable = var;
		}

		auto insertResult = knownVariables.insert(var);
		bool inserted = insertResult.second;
		if (inserted) {
			newVariables.push_back(var);
		}
	}

	void scrumbleVariables() {
		std::vector<unsigned> newVariables;
		unsigned maxVariable = toIpasir.size();

		for (std::vector<int>& clause: clauses) {
			for (int lit: clause) {
				addLiteral(newVariables, maxVariable, lit);
			}
		}

		for (int lit:assumptions) {
			addLiteral(newVariables, maxVariable, lit);
		}

		std::shuffle(newVariables.begin(), newVariables.end(), g);

		size_t newStart = fromIpasir.size();
		fromIpasir.insert(fromIpasir.end(), newVariables.begin(), newVariables.end());
		toIpasir.resize(maxVariable + 1, 0);

		for (size_t i = newStart; i < fromIpasir.size(); i++) {
			toIpasir[fromIpasir[i]] = i;
		}
	}

	void scrumbleClauses() {
		assert(clauses.back().size() == 0);
		clauses.pop_back();
		std::shuffle(clauses.begin(), clauses.end(), g);
		for (std::vector<int>& clause: clauses) {
			std::shuffle(clause.begin(), clause.end(), g);
		}
		std::shuffle(assumptions.begin(), assumptions.end(), g);
	}

	inline unsigned litToVar(int lit) {
		return std::abs(lit);
	}

	int unmap(int literal) {
		return map(literal, true);
	}

	int map(int literal, bool unMap = false) {
		if (literal == 0) {
			return 0;
		}

		unsigned variable = std::abs(literal);
		int sign = (literal < 0)? -1 : 1;

		if (!unMap) {
			// Note that toIpasir is a permutation from [0,size())
			variable = toIpasir[variable];
		} else {
			variable = fromIpasir[variable];
		}
		assert(variable != 0);

		return sign * variable;
	}
};
}