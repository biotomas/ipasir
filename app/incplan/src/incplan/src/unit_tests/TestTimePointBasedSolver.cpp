#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ipasir/test/mock_ipasir_cpp.h"
#include "TimePointBasedSolver.h"
#include "TimeSlotMapping.h"

using ::testing::_;
using ::testing::InSequence;

class TimePointBasedSolver_mini : public ::testing::Test {
public:
TimePointBasedSolver_mini() {
		SingleEndedTimePointManager tpm;
		t0 = tpm.aquireNext();
		t1 = tpm.aquireNext();

		std::unique_ptr<ipasir::MockIpasir> ipasirPtr =
			std::make_unique<ipasir::MockIpasir>();
		ipasir = ipasirPtr.get();

		int varsPerTime = 2;
		int helperPerTime = 1;
		solver = std::make_unique<TimePointBasedSolver>(varsPerTime,
			helperPerTime, std::move(ipasirPtr));
	}

	TimePoint t0;
	TimePoint t1;

	std::unique_ptr<TimePointBasedSolver> solver;
	ipasir::MockIpasir* ipasir;
};

TEST_F( TimePointBasedSolver_mini, addPositive) {
	InSequence dummy;

	EXPECT_CALL(*ipasir, add(1));
	EXPECT_CALL(*ipasir, add(4));

	solver->addProblemLiteral(1, t0);
	solver->addProblemLiteral(1, t1);
}

TEST_F( TimePointBasedSolver_mini, addNegative) {
	InSequence dummy;

	EXPECT_CALL(*ipasir, add(-1));
	EXPECT_CALL(*ipasir, add(-4));

	solver->addProblemLiteral(-1, t0);
	solver->addProblemLiteral(-1, t1);
}

TEST_F( TimePointBasedSolver_mini, addPositiveHelper) {
	InSequence dummy;

	EXPECT_CALL(*ipasir, add(3));
	EXPECT_CALL(*ipasir, add(6));

	solver->addHelperLiteral(1, t0);
	solver->addHelperLiteral(1, t1);
}

TEST_F( TimePointBasedSolver_mini, addNegativeHelper) {
	InSequence dummy;

	EXPECT_CALL(*ipasir, add(-3));
	EXPECT_CALL(*ipasir, add(-6));

	solver->addHelperLiteral(-1, t0);
	solver->addHelperLiteral(-1, t1);
}

TEST_F( TimePointBasedSolver_mini, inteferences) {
	InSequence dummy;
	EXPECT_CALL(*ipasir, add(1))
		.Times(2);
	EXPECT_CALL(*ipasir, add(2))
		.Times(1);
	EXPECT_CALL(*ipasir, add(3))
		.Times(1);

	EXPECT_CALL(*ipasir, add(4))
		.Times(2);
	EXPECT_CALL(*ipasir, add(-5))
		.Times(1);
	EXPECT_CALL(*ipasir, add(-6))
		.Times(1);

	EXPECT_CALL(*ipasir, add(1))
		.Times(1);

	solver->addProblemLiteral(1, t0);
	solver->addProblemLiteral(1, t0);
	solver->addProblemLiteral(2, t0);
	solver->addHelperLiteral(1, t0);

	solver->addProblemLiteral(1, t1);
	solver->addProblemLiteral(1, t1);
	solver->addProblemLiteral(-2, t1);
	solver->addHelperLiteral(-1, t1);

	solver->addProblemLiteral(1, t0);
}