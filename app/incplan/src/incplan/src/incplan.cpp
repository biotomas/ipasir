#include "ipasir/ipasir_cpp.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <limits>
#include <cassert>
#include <memory>
#include <cmath>
#include <set>
#include <map>
#include <random>
#include <stack>

#include "tclap/CmdLine.h"

#include "TimeSlotMapping.h"
#include "TimePointBasedSolver.h"
#include "carj/carj.h"
#include "carj/logging.h"
#include "carj/ScopedTimer.h"


struct Options {
	bool error;
	std::string inputFile;
	bool unitInGoal2Assume;
	bool solveBeforeGoalClauses;
	bool nonIncrementalSolving;
	bool normalOutput;
	bool singleEnded;
	bool cleanLitearl;
	bool icaps2017Version;
	double ratio;
	std::function<int(int)> stepToMakespan;
};

Options options;

class Problem {
public:
	Problem(std::istream& in){
		this->numberLiteralsPerTime = 0;
		parse(in);
		//inferAdditionalInformation();
	}

	std::vector<int> initial, invariant, goal , transfer;
	unsigned numberLiteralsPerTime;
	std::vector<int> actionVariables;
	std::vector<int> stateVariables;
	std::map<int, std::vector<int>> support;

private:
	void inferAdditionalInformation() {
		VLOG(2) << "Number Of Literals per Time: " << this->numberLiteralsPerTime;

		// state variables should all be set in the initial state
		std::set<int> stateVariables;
		for (int lit: this->initial) {
			stateVariables.insert(std::abs(lit));
		}
		stateVariables.erase(0);

		// {
		// 	std::stringstream ss;
		// 	ss << "State Variables: ";
		// 	for (int var: stateVariables) {
		// 		ss << var << ", ";
		// 	}
		// 	VLOG(2) << ss.str();
		// }


		// guess action variables from clauses containing states
		std::set<int> actionVariablesHelper;
		size_t clauseStart = 0;
		bool clauseHasStateVar = false;
		for (size_t i = 0; i < this->transfer.size(); i++) {
			unsigned var = std::abs(this->transfer[i]);
			var = var > this->numberLiteralsPerTime ? var - this->numberLiteralsPerTime: var;
			if (this->transfer[i] != 0) {
				if (stateVariables.find(var) != stateVariables.end()) {
					clauseHasStateVar = true;
				}
			} else {
				if (clauseHasStateVar) {
					for (size_t j = clauseStart; j < i; j++) {
						unsigned var = std::abs(this->transfer[j]);
						var = var > this->numberLiteralsPerTime ? var - this->numberLiteralsPerTime: var;
						actionVariablesHelper.insert(var);
					}
				}

				clauseStart = i + 1;
				clauseHasStateVar = false;
			}
		}

		std::set<int> actionVariables;
		std::set_difference(actionVariablesHelper.begin(), actionVariablesHelper.end(),
							stateVariables.begin(), stateVariables.end(),
							std::inserter(actionVariables, actionVariables.end()));

		std::vector<int> currentClauseActions;
		std::vector<int> currentClauseFutureState;
		for (size_t i = 0; i < this->transfer.size(); i++) {
			unsigned var = std::abs(this->transfer[i]);
			if (var == 0) {
				for (int stateVar: currentClauseFutureState) {
					auto res = support.insert(std::make_pair(stateVar, std::vector<int>()));
					std::copy(currentClauseActions.begin(), currentClauseActions.end(), std::back_inserter(res.first->second));
				}

			} else {
				bool isNextTime = false;
				if (var > this->numberLiteralsPerTime) {
					isNextTime = true;
					var -= this->numberLiteralsPerTime;
				}

				if (stateVariables.find(var) != stateVariables.end() && isNextTime) {
					currentClauseFutureState.push_back(var);
				}

				if (actionVariables.find(var) != actionVariables.end()) {
					assert(!isNextTime);
					currentClauseActions.push_back(var);
				}
			}
		}

		std::copy(actionVariables.begin(), actionVariables.end(), std::back_inserter(this->actionVariables));
		std::copy(stateVariables.begin(), stateVariables.end(), std::back_inserter(this->stateVariables));

		// {
		// 	std::stringstream ss;
		// 	ss << "State Variables: ";
		// 	for (int var: actionVariables) {
		// 		ss << var << ", ";
		// 	}
		// 	VLOG(2) << ss.str();
		// }

	}

	void skipComments(std::istream& in){
		char nextChar;
		in >> nextChar;
		while ( nextChar == 'c' ) {
			in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			in >> nextChar;
		}
		if (in.eof()) {
			std::cerr << "Input Error: Expected [iugt] cnf [0-9*] [0-9*] but got nothing. (File empty or only comments) " << std::endl;
			std::exit(1);
		}
		// Next character wasn't c, so revert the last get:
		in.unget();
		assert(in.good() && "Internal Error: Failed to put back character to stream. Change previous code.");
	}

	int parseCnfHeader(char expectedType, std::istream& in) {
		char type;
		unsigned literals;
		int numberClauses;
		std::string cnfString;
		in >> type >> cnfString >> literals >> numberClauses;
		if (!in.good() || cnfString != "cnf") {
			std::string line;
			in.clear();
			in >> line;
			LOG(FATAL) << "Input Error: Expected [iugt] cnf [0-9*] [0-9*] but got: " << line;
		}

		if (type != expectedType) {
			LOG(FATAL) << "Input Error: Expected type " << expectedType << " but got: " << type;
		}

		switch(type) {
			case 'i':
			case 'u':
			case 'g':
			case 't':
			break;
			default:
			LOG(FATAL) << "Input Error: Expected type i,u,g or t but got " << type;
		}

		if (numberLiteralsPerTime == 0) {
			numberLiteralsPerTime = literals;
		} else if (literals != numberLiteralsPerTime && (type != 't' || literals != 2 * numberLiteralsPerTime)) {
			LOG(FATAL) << "Input Error: Wrong Number of Literals!" << literals << ":" << numberLiteralsPerTime << type;
		}

		return numberClauses;
	}

	void parseCnf(std::vector<int>& cnf, std::istream& in) {
		int literal;
		while (in >> literal) {
			cnf.push_back(literal);
		}
		// The above will abort as soon as no further number is found.
		// Therefore, we need to reset the error state of the input stream:
		in.clear();
	}

	void parse(std::istream& in) {
		if (in.eof()) {
			LOG(FATAL) << "Input Error: Got empty File.";
		}
		skipComments(in);
		parseCnfHeader('i', in);
		parseCnf(initial, in);
		parseCnfHeader('u', in);
		parseCnf(invariant, in);
		parseCnfHeader('g', in);
		parseCnf(goal, in);
		parseCnfHeader('t', in);
		parseCnf(transfer, in);
	}
};

struct AddInfo {
	unsigned transitionSource;
	unsigned transitionGoal;
	unsigned addedSlot;
};

enum class HelperVariables { ZeroVariableIsNotAllowed_DoNotRemoveThis,
	ActivationLiteral };

class Solver : public TimePointBasedSolver {
	public:
		Solver(const Problem* problem):
			TimePointBasedSolver(
				problem->numberLiteralsPerTime,
				1,
				std::make_unique<ipasir::Solver>(),
				//std::make_unique<ipasir::RandomizedSolver>(std::make_unique<ipasir::Solver>()),
				options.icaps2017Version?
					TimePointBasedSolver::HelperVariablePosition::AllBefore:
					TimePointBasedSolver::HelperVariablePosition::SingleAfter){
			this->problem = problem;
		}

		/**
		 * Solve the problem. Return true if a solution was found.
		 */
		bool solve(){
			bool result = slv();
			LOG(INFO) << "Final Makespan: " << this->makeSpan;
			this->finalMakeSpan = this->makeSpan;
			return result;
		}

		void printSolution() {
			if (this->solveResult == ipasir::SolveResult::SAT) {
				std::cout << "solution " << this->problem->numberLiteralsPerTime << " " << this->finalMakeSpan + 1 << std::endl;
				TimePoint t = timePointManager->getFirst();
				int time = 0;

				do {
					for (unsigned j = 1; j <= this->problem->numberLiteralsPerTime; j++) {
						int val = valueProblemLiteral(j,t);
						if (options.normalOutput) {
							int offset = time * problem->numberLiteralsPerTime;
							if (val < 0) {
								offset = -offset;
							}
							val += offset;
						}
						std::cout << val << " ";
					}
					if (!options.normalOutput) {
						std::cout << std::endl;
					}
					if (t == timePointManager->getLast()) {
						break;
					}

					t = timePointManager->getSuccessor(t);
					time++;
				} while (true);

				if (options.normalOutput) {
					std::cout << std::endl;
				}
			} else {
				std::cout << "no solution" << std::endl;
			}
		}

		~Solver(){
		}

	private:
		const Problem* problem;

		int finalMakeSpan;
		int makeSpan;
		ipasir::SolveResult solveResult;
		std::unique_ptr<TimePointManager> timePointManager;

		TimePoint initialize() {
			this->reset();

			if (options.singleEnded) {
				timePointManager = std::make_unique<SingleEndedTimePointManager>();
			} else {
				timePointManager = std::make_unique<DoubleEndedTimePointManager>(
					options.ratio,
					DoubleEndedTimePointManager::TopElementOption::Dublicated);
			}

			makeSpan = 0;
			TimePoint t0 = timePointManager->aquireNext();
			addInitialClauses(t0);
			addInvariantClauses(t0);

			if (!options.singleEnded) {
				TimePoint tN = timePointManager->aquireNext();
				addGoalClauses(tN);
				addInvariantClauses(tN);

				return tN;
			}

			return t0;
		}

		void finalize(const TimePoint elementInsertedLast) {
			if (options.singleEnded) {
				addGoalClauses(elementInsertedLast, true);
			} else {
				TimePoint linkSource, linkDestination;
				if (elementInsertedLast == timePointManager->getLast()) {
					linkSource = timePointManager->getFirst();
					linkDestination = timePointManager->getLast();
				} else if (timePointManager->isOnForwardStack(elementInsertedLast)){
					linkSource = elementInsertedLast;
					linkDestination = timePointManager->getSuccessor(
						timePointManager->getPredecessor(elementInsertedLast));
				} else {
					linkSource = timePointManager->getPredecessor(
						timePointManager->getSuccessor(elementInsertedLast));
					linkDestination = elementInsertedLast;
				}

				addLink(linkSource, linkDestination, elementInsertedLast);
			}
			int activationLiteral = static_cast<int>(HelperVariables::ActivationLiteral);
			assumeHelperLiteral(activationLiteral, elementInsertedLast);

			// if (elementInsertedLast != timePointManager->getFirst()) {
			// 	TimePoint next = timePointManager->getPredecessor(elementInsertedLast);
			// 	while (next != timePointManager->getFirst()) {
			// 		VLOG(2) << next.second;
			// 		assumeHelperLiteral(-activationLiteral, next);
			// 		next = timePointManager->getPredecessor(next);
			// 	}
			// 	assumeHelperLiteral(-activationLiteral, next);
			// }
		}

		bool slv(){
			LOG(INFO) << "Start solving";
			nlohmann::json solves;
			int step = 0;
			TimePoint elementInsertedLast = initialize();

			ipasir::SolveResult result = ipasir::SolveResult::UNSAT;
			for (;result != ipasir::SolveResult::SAT;step++) {
				if(options.nonIncrementalSolving) {
					elementInsertedLast = initialize();
				}

				int targetMakeSpan = options.stepToMakespan(step);
				for (; makeSpan < targetMakeSpan; makeSpan++) {
					TimePoint tNew = timePointManager->aquireNext();
					addInvariantClauses(tNew);

					if (timePointManager->isOnForwardStack(tNew)) {
						TimePoint pred = timePointManager->getPredecessor(tNew);
						addTransferClauses(pred, tNew);
					} else {
						TimePoint succ = timePointManager->getSuccessor(tNew);
						addTransferClauses(tNew, succ);
					}

					elementInsertedLast = tNew;
				}

				if (options.solveBeforeGoalClauses) {
					TIMED_SCOPE(blkScope, "intermediateSolve");
					solveSAT();
				}

				finalize(elementInsertedLast);

				VLOG(1) << "Solving makespan " << makeSpan;
				solves.push_back({
					{"makespan", makeSpan},
					{"time", -1}
				});
				{
					carj::ScopedTimer timer((*solves.rbegin())["time"]);
					TIMED_SCOPE(blkScope, "solve");
					result = solveSAT();
				}

				if (options.cleanLitearl) {
					VLOG(1) << "Cleaning helper Literal.";
					int activationLiteral = static_cast<int>(HelperVariables::ActivationLiteral);
					addHelperLiteral(-activationLiteral, elementInsertedLast);
					finalizeClause();
				}
			}

			carj::getCarj().data["/incplan/result/solves"_json_pointer] =
				solves;
			this->solveResult = result;
			return result == ipasir::SolveResult::SAT;
		}

		void addInitialClauses(TimePoint t) {
			for (int literal:problem->initial) {
				addProblemLiteral(literal, t);
			}
		}

		void addInvariantClauses(TimePoint t) {
			for (int literal: problem->invariant) {
				addProblemLiteral(literal, t);
			}
		}

		void addTransferClauses(TimePoint source, TimePoint destination) {
			for (int literal: problem->transfer) {
				bool literalIsSourceTime = static_cast<unsigned>(std::abs(literal)) <= problem->numberLiteralsPerTime;
				if (!literalIsSourceTime) {
					if (literal > 0) {
						literal -= problem->numberLiteralsPerTime;
					} else {
						literal += problem->numberLiteralsPerTime;
					}
				}

				if (literalIsSourceTime) {
					addProblemLiteral(literal, source);
				} else {
					addProblemLiteral(literal, destination);
				}
			}
		}

		/*
		 * Returns true if problem->goal[i] is a literal in a unit clause
		 *         false otherwise.
		 */
		bool isUnitGoal(unsigned i) {
			if (!options.unitInGoal2Assume) {
				return false;
			}

			if (i == 0) {
				return problem->goal[i + 1] == 0;
			}

			if (problem->goal[i] == 0) {
				return false;
			}

			assert(i > 0 && i + 1 < problem->goal.size());
			return problem->goal[i + 1] == 0 && problem->goal[i - 1] == 0;
		}

		void addGoalClauses(TimePoint t, bool isGuarded = false) {
			for (unsigned i = 0; i < problem->goal.size(); i++) {
				int literal = problem->goal[i];
				if (!isUnitGoal(i)) {
					if (literal == 0 && isGuarded) {
						int activationLiteral = static_cast<int>(HelperVariables::ActivationLiteral);
						addHelperLiteral(-activationLiteral, t);
					}
					addProblemLiteral(literal, t);
				} else {
					assumeProblemLiteral(literal, t);
					++i; // skip following 0
					assert(problem->goal[i] == 0);
				}
			}
		}

		void addLink(TimePoint A, TimePoint B, TimePoint helperLiteralBinding) {
			for (unsigned i = 1; i <= problem->numberLiteralsPerTime; i++) {
				int activationLiteral = static_cast<int>(HelperVariables::ActivationLiteral);

				addHelperLiteral(-activationLiteral, helperLiteralBinding);
				addProblemLiteral(-i, A);
				addProblemLiteral(i, B);
				finalizeClause();

				addHelperLiteral(-activationLiteral, helperLiteralBinding);
				addProblemLiteral(i, A);
				addProblemLiteral(-i, B);
				finalizeClause();
			}
		}
};


//bool defaultIsTrue = true;
bool defaultIsFalse = false;
bool neccessaryArgument = true;

TCLAP::CmdLine cmd("This tool is does sat planing using an incremental sat solver.", ' ', "0.1");
carj::MCarjArg<TCLAP::MultiArg, std::string> pathSearchPrefix("", "pathSearchPrefix", "If passed files are not found their path will be prefixed with values of this parameter.", !neccessaryArgument, "path", cmd);
void parseOptions(int argc, const char **argv) {
	try {
		carj::TCarjArg<TCLAP::UnlabeledValueArg,std::string>  inputFile("inputFile", "File containing the problem. Omit or use - for stdin.", !neccessaryArgument, "-", "inputFile", cmd);
		carj::TCarjArg<TCLAP::ValueArg, double>  ratio("r", "ratio", "Ratio between states from start to state from end.", !neccessaryArgument, 0.5, "number between 0 and 1", cmd);
		carj::TCarjArg<TCLAP::ValueArg, unsigned>  linearStepSize("l", "linearStepSize", "Linear step size.", !neccessaryArgument, 1, "natural number", cmd);
		carj::TCarjArg<TCLAP::ValueArg, float> exponentialStepBasis("e", "exponentialStepBasis", "Basis of exponential step size. Combinable with options -l and -o (varibale names are equal to parameter): step size = l*n + floor(e ^ (n + o))", !neccessaryArgument, 0, "natural number", cmd);
		carj::TCarjArg<TCLAP::ValueArg, float> exponentialStepOffset("o", "exponentialStepOffset", "Basis of exponential step size.", !neccessaryArgument, 0, "natural number", cmd);
		carj::CarjArg<TCLAP::SwitchArg, bool> unitInGoal2Assume("u", "unitInGoal2Assume", "Add units in goal clauses using assume instead of add. (singleEnded only)", cmd, defaultIsFalse);
		carj::CarjArg<TCLAP::SwitchArg, bool> solveBeforeGoalClauses("i", "intermediateSolveStep", "Add an additional solve step before adding the goal or linking clauses.", cmd, defaultIsFalse);
		carj::CarjArg<TCLAP::SwitchArg, bool> nonIncrementalSolving("n", "nonIncrementalSolving", "Do not use incremental solving.", cmd, defaultIsFalse);
		carj::CarjArg<TCLAP::SwitchArg, bool> cleanLitearl("c", "cleanLitearl", "Add a literal to remove linking or goal clauses.", cmd, defaultIsFalse);

		carj::CarjArg<TCLAP::SwitchArg, bool> singleEnded("s", "singleEnded", "Use naive incremental encoding.", cmd, defaultIsFalse);
		//TCLAP::SwitchArg outputLinePerStep("", "outputLinePerStep", "Output each time point in a new line. Each time point will use the same literals.", defaultIsFalse);
		carj::CarjArg<TCLAP::SwitchArg, bool> outputSolverLike("", "outputSolverLike", "Output result like a normal solver is used. The literals for each time point t are in range t * [literalsPerTime] < lit <= (t + 1) * [literalsPerTime]", cmd, defaultIsFalse);
		carj::CarjArg<TCLAP::SwitchArg, bool> icaps2017Version("", "icaps2017", "Use this option to use encoding as used in the icaps paper.", cmd, defaultIsFalse);

		carj::init(argc, argv, cmd, "/incplan/parameters");

		options.error = false;
		options.inputFile = inputFile.getValue();
		options.unitInGoal2Assume = unitInGoal2Assume.getValue();
		options.solveBeforeGoalClauses = solveBeforeGoalClauses.getValue();
		options.nonIncrementalSolving = nonIncrementalSolving.getValue();
		options.normalOutput = outputSolverLike.getValue();
		options.ratio = ratio.getValue();
		options.singleEnded = singleEnded.getValue();
		options.cleanLitearl = cleanLitearl.getValue();
		options.icaps2017Version = icaps2017Version.getValue();
		{
			int l = linearStepSize.getValue();
			float e = exponentialStepBasis.getValue();
			float o = exponentialStepOffset.getValue();
			if (e != 0) {
				options.stepToMakespan =
					[l,e,o](int n){return l * n + std::floor(pow(e,(n + o)));};
			} else {
				options.stepToMakespan =
					[l,e,o](int n){return l * n;};
			}

		}


		if (options.nonIncrementalSolving) {
			options.singleEnded = true;
		}

	} catch (TCLAP::ArgException &e) {
		options.error = true;
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}
}


int incplan_main(int argc, const char **argv) {
	parseOptions(argc, argv);

	//LOG(INFO) << "Using the incremental SAT solver " << ipasir_signature();

	std::istream* in;
	std::ifstream is;
	if (options.inputFile == "-") {
		in = &std::cin;
		LOG(INFO) << "Using standard input.";
	} else {

		std::vector<std::string> prefixes = pathSearchPrefix.getValue();
		prefixes.insert(prefixes.begin(), "");

		for (std::string prefix: prefixes) {
			is.open(prefix + options.inputFile);
			if (!is.fail()) {
				break;
			}
		}

		if (is.fail()){
			LOG(FATAL) << "Input Error can't open file: " << options.inputFile;
		}
		in = &is;
	}

	bool solved;
	{
		Problem problem(*in);
		Solver solver(&problem);
		solved = solver.solve();
		solver.printSolution();
	}

	if (!solved) {
		LOG(WARNING) << "Did not get a solution within the maximal make span.";
	}

	return 0;
}
