#include "gtest/gtest.h"
#include "TimeSlotMapping.h"

#include <iostream>

TEST( SingleEndedTimePointManager, basic) {
	SingleEndedTimePointManager tpm;

	auto t0 = tpm.aquireNext();
	ASSERT_TRUE( tpm.getFirst() == t0 );
	ASSERT_TRUE( tpm.getLast() == t0 );

	auto t1 = tpm.aquireNext();
	ASSERT_TRUE( tpm.getFirst() == t0 );
	ASSERT_TRUE( tpm.getLast() == t1 );
	ASSERT_TRUE( tpm.getPredecessor(t1) == t0 );
	ASSERT_TRUE( tpm.getSuccessor(t0) == t1 );

	auto t2 = tpm.aquireNext();
	ASSERT_TRUE( tpm.getFirst() == t0 );
	ASSERT_TRUE( tpm.getLast() == t2 );
	ASSERT_TRUE( tpm.getPredecessor(t1) == t0 );
	ASSERT_TRUE( tpm.getSuccessor(t1) == t2 );
}

TEST( SingleEndedTimePointManager, traversal) {
	SingleEndedTimePointManager tpm;

	auto t0 = tpm.aquireNext();
	auto t1 = tpm.aquireNext();
	auto t2 = tpm.aquireNext();

	auto t = tpm.getFirst();
	ASSERT_TRUE( t == t0 );
	t = tpm.getSuccessor(t);
	ASSERT_TRUE( t == t1 );
	t = tpm.getSuccessor(t);
	ASSERT_TRUE( t == t2 );
	ASSERT_TRUE( t == tpm.getLast() );
}

TEST( DoubleEndedTimePointManager, allBegin_unique) {
	DoubleEndedTimePointManager tpm(1.f,
		DoubleEndedTimePointManager::TopElementOption::Unique);

	auto t0 = tpm.aquireNext();
	auto tn = tpm.aquireNext();

	ASSERT_TRUE( tpm.getFirst() == t0 );
	ASSERT_TRUE( tpm.getLast() == tn );

	TimePoint previous = t0;
	for (int i = 0; i < 20; i++) {
		auto t = tpm.aquireNext();
		ASSERT_TRUE( tpm.isOnForwardStack(t) );
		ASSERT_TRUE( tpm.getFirst() == t0 );
		ASSERT_TRUE( tpm.getLast() == tn );
		ASSERT_TRUE( tpm.getPredecessor(t) == previous );
		ASSERT_TRUE( tpm.getSuccessor(t) == tn );
		previous = t;
	}
}

TEST( DoubleEndedTimePointManager, allBegin_dublicate) {
	DoubleEndedTimePointManager tpm(1.f,
		DoubleEndedTimePointManager::TopElementOption::Dublicated);

	auto t0 = tpm.aquireNext();
	auto tn = tpm.aquireNext();

	ASSERT_TRUE( tpm.getFirst() == t0 );
	ASSERT_TRUE( tpm.getLast() == tn );

	TimePoint previous = t0;
	for (int i = 0; i < 20; i++) {
		auto t = tpm.aquireNext();
		ASSERT_TRUE( tpm.isOnForwardStack(t) );
		ASSERT_TRUE( tpm.getFirst() == t0 );
		ASSERT_TRUE( tpm.getLast() == tn );
		ASSERT_TRUE( tpm.getPredecessor(t) == previous );
		ASSERT_TRUE( tpm.getSuccessor(previous) == tn );
		try {
			tpm.getSuccessor(t);
			FAIL() << "";
		} catch (std::out_of_range const & err) {

		} catch (...) {
			FAIL() << "Wrong Exception";
		}
		previous = t;
	}
}

TEST( DoubleEndedTimePointManager, r05_dublicate) {
	DoubleEndedTimePointManager tpm(0.5f,
		DoubleEndedTimePointManager::TopElementOption::Dublicated);

	auto t0 = tpm.aquireNext();
	auto tn = tpm.aquireNext();

	ASSERT_TRUE( tpm.getFirst() == t0 );
	ASSERT_TRUE( tpm.getLast() == tn );

	bool putForward = true;
	for (int i = 0; i < 20; i++) {
		auto t = tpm.aquireNext();
		if (putForward) {
			ASSERT_TRUE( tpm.isOnForwardStack(t) );
		} else {
			ASSERT_FALSE( tpm.isOnForwardStack(t) );
		}
		putForward ^= true;

		TimePoint forward = tpm.getFirst();
		TimePoint backward = tpm.getLast();
		for (int j = 0; j <= i; j ++) {
			forward = tpm.getSuccessor(forward);
			backward = tpm.getPredecessor(backward);
		}
		ASSERT_TRUE(forward == tn);
		ASSERT_TRUE(backward == t0);
	}
}