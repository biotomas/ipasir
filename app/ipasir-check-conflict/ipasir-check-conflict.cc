// Norbert Manthey, 2017

// make sure we use IPASIR
#define HAVE_IPASIR

// declare IPASIR functions here, which we might need
#include "SATSolver.h"

#include <iostream>
#include <vector>

using namespace std;

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>

inline double cpuTime(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000;
}

void learnhook(void * state, int * clause) {
  if(!clause) return;
  int len = 0;
  while( clause[len] != 0 ) ++len;
  std::cout << "c learned clause of size " << len << std::endl;
}


int main(int argc, char* argv[])
{
  std::cout << "Norbert Manthey -- IPASIR conflict checker" << std::endl;
  std::cout << "  reserve max len for conflict of size X and try force a conflict clause of size Y" << std::endl
            << "  and a full stack of assumption literals" << std::endl;
  if(argc < 3) {
    std::cout << "usage: " << argv[0] << " X Y ... with 0 < X < Y < 2^30" << std::endl;
    return 1;
  }
  int X = atoi(argv[1]);
  int Y = atoi(argv[2]);
  if(X<1) {
    std::cout << "error: given first number " << X << " is not in limits" << std::endl;
    return 1;
  }
  if(Y<1) {
    std::cout << "error: given second number " << Y << " is not in limits" << std::endl;
    return 1;
  }
  
  vector<int> assumptions;
  vector<int> positive, negative;
  
  std::vector<int> model;
  std::vector<int> conflict;
  
  SATSolver solver;
  solver.addLearnStatHook(X, learnhook);
  
  double runtime = cpuTime();
  for(int i = 1; i < Y; ++ i)
  {
    assumptions.push_back(-i);
    positive.push_back(i);
    negative.push_back(i);
  }
  positive.push_back(Y);
  negative.push_back(-Y);
  
  solver.addClause(positive);
  std::cout << "add clause " << positive << std::endl;
  solver.addClause(negative);
  std::cout << "add clause " << negative << std::endl;
  
  std::cout << "solve with assumptions " << assumptions << std::endl;
  int ret = solver.solve(assumptions, model, conflict);
  if(ret!=20) {
    std::cout << "error: solver fails learning a huge clause with status " << ret << std::endl;
    return 1;
  }
  
  return 0;
}
  
