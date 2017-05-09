#pragma once
#include "gmock/gmock.h"
#include "ipasir/ipasir_cpp.h"

namespace ipasir {
class MockIpasir: public Ipasir {
public:
	MOCK_METHOD0(signature, std::string ());
	MOCK_METHOD1(add, void (int lit_or_zero));
	MOCK_METHOD1(assume, void (int lit));
	MOCK_METHOD0(solve, ipasir::SolveResult ());
	MOCK_METHOD1(val, int (int lit));
	MOCK_METHOD1(failed, int  (int lit));
	MOCK_METHOD1(set_terminate, void  (std::function<int(void)> callback));
	MOCK_METHOD2(set_learn, void (int max_length, std::function<void(int*)> callback));
	MOCK_METHOD0(reset, void ());
};
}