/*
 * longestpath.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: Tomas Balyo
 */

extern "C" {
    #include "ipasir.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <sys/time.h>
#include <stdarg.h>
#include <signal.h>


using namespace std;

struct Graph {
	int verticesCount;
	int edgesCount;
	vector<vector<int> > neighbours;
};

struct SimpleFormula {
	int vars;
	vector<vector<int> > clauses;
};


double getAbsoluteTimeLP() {
	timeval time;
	gettimeofday(&time, NULL);
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

static double start = getAbsoluteTimeLP();

void log(const char* fmt ...) {
	va_list args;
	va_start(args, fmt);
	printf("[%.3f] ", getAbsoluteTimeLP() - start);
	vprintf(fmt, args);
	va_end(args);
}

int best;
int pathLength;

void signalHandler(int signal) {
	log("Search terminated, longest found path is %d, there is no path of lenght %d-%d\n", best, best+1, pathLength);
	abort();
}

Graph loadGraph(const char* filename, bool backEdges = false) {
	FILE* f = fopen(filename, "r");
	if (f == NULL) {
		log("File '%s' cannot be opened\n", filename);
		exit(1);
	}
	Graph g;
	int c = 0;
	bool edge = true;

	while (c != EOF) {
		c = fgetc(f);

		// problem definition line
		if (c == 'p') {
			char pline[512];
			int i = 0;
			while (c != '\n') {
				pline[i++] = c;
				c = fgetc(f);
			}
			pline[i] = 0;
			if (2 != sscanf(pline, "p edge %d %d", &(g.verticesCount), &(g.edgesCount))) {
				edge = false;
				if (2 != sscanf(pline, "p sp %d %d", &(g.verticesCount), &(g.edgesCount))) {
					log("Failed to parse the problem definition line (%s)\n", pline);
				}
			}
			continue;
		}
		// comment
		if (c == 'c') {
			// skip this line
			while(c != '\n') {
				c = fgetc(f);
			}
			continue;
		}
		// edge
		if (c == 'e' || c == 'a') {
			char pline[512];
			int i = 0;
			while (c != '\n') {
				pline[i++] = c;
				c = fgetc(f);
			}
			pline[i] = 0;
			size_t start, end, weight;
			if (edge && 2 != sscanf(pline, "e %lu %lu", &start , &end)) {
				log("Invalid edge line (%s)\n", pline);
				exit(1);
			}
			if (!edge && 3 != sscanf(pline, "a %lu %lu %lu", &start , &end, &weight)) {
				log("Invalid edge line (%s)\n", pline);
				exit(1);
			}
			while (g.neighbours.size() <= start) {
				g.neighbours.push_back(vector<int>());
			}
			g.neighbours[start].push_back(end);

			if (edge && backEdges) {
				while (g.neighbours.size() <= end) {
					g.neighbours.push_back(vector<int>());
				}
				g.neighbours[end].push_back(start);
			}
			continue;
		}
	}
	fclose(f);
	return g;
}

void addFormula(void* solver, const SimpleFormula& formula) {
	for (size_t i = 0; i < formula.clauses.size(); i++) {
		for (size_t j = 0; j < formula.clauses[i].size(); j++) {
			ipasir_add(solver, formula.clauses[i][j]);
		}
		ipasir_add(solver, 0);
	}
}

void printGraph(const Graph& graph) {
	printf("p edge %d %d\n", graph.verticesCount, graph.edgesCount);
	for (size_t i = 0; i < graph.neighbours.size(); i++) {
		for (size_t j = 0; j < graph.neighbours[i].size(); j++) {
			printf("e %lu %d\n", i, graph.neighbours[i][j]);
		}
	}
}

void printFormula(const SimpleFormula& formula) {
	printf("p cnf %d %lu\n", formula.vars, formula.clauses.size());
	for (size_t i = 0; i < formula.clauses.size(); i++) {
		for (size_t j = 0; j < formula.clauses[i].size(); j++) {
			printf("%d ", formula.clauses[i][j]);
		}
		printf("0\n");
	}
}

map<int,int> visited;

bool isVisited(int vert) {
	return visited.find(vert) != visited.end();
}

int recDFS(const Graph&g, int vert, int goal) {
	//log("called dfs with %d\n", vert);
	if (vert == goal) {
		return 1;
	}
	visited[vert]=vert;
	int best = 0;
	for (size_t i = 0; i < g.neighbours[vert].size(); i++) {
		int nvert = g.neighbours[vert][i];
		if (!isVisited(nvert)) {
			int res = recDFS(g, nvert, goal);
			if (res > best) {
				best = res;
			}
		}
	}
	return best > 0 ? best+1 : 0;
}

// get a lower bound for the longest path by finding path by DFS
int getLowerUpperBound(const Graph& g, int start, int goal, int& lowBound, int& upBound) {
	visited.clear();
	lowBound = recDFS(g,start,goal);
	visited[goal] = goal; // because goal is not marked visited by DFS
	upBound = visited.size();
	return lowBound;
}

inline int mapVar(int i, int j, int verts) {
	return j*verts + i;
}

void addInitalCondition(void* solver, const Graph&g, int start) {
	ipasir_add(solver, mapVar(start,0,g.verticesCount));
	ipasir_add(solver, 0);
}

void assumeGoalCondition(void* solver, const Graph&g, int goal, int step) {
	ipasir_assume(solver, mapVar(goal,step,g.verticesCount));
}

void addUniversalConditions(void* solver, const Graph&g, int step) {
	// at most one vertex has a particular position
	for (int v1 = 1; v1 <= g.verticesCount; v1++) {
		if (!isVisited(v1)) continue;
		for (int v2 = 1; v2 < v1; v2++) {
			if (!isVisited(v2)) continue;
			ipasir_add(solver, -mapVar(v1,step,g.verticesCount));
			ipasir_add(solver, -mapVar(v2,step,g.verticesCount));
			ipasir_add(solver, 0);
		}
	}
}

// Between step-1 and step
void addTransitionalConditions(void* solver, const Graph&g, int step) {
	int verts = g.verticesCount;
	for (int v1 = 1; v1 <= verts; v1++) {
		if (!isVisited(v1)) continue;
		// clauses expressing that successor of the node in a path is its neighbour
		ipasir_add(solver, -mapVar(v1,step-1,verts));
		for (size_t i = 0; i < g.neighbours[v1].size(); i++) {
			int v2 = g.neighbours[v1][i];
			ipasir_add(solver, mapVar(v2, step,verts));
		}
		ipasir_add(solver, 0);
		// the selected vertex is not used in any of the previous steps
		for (int prevStep=0; prevStep < step; prevStep++) {
			ipasir_add(solver, -mapVar(v1,step,verts));
			ipasir_add(solver, -mapVar(v1,prevStep,verts));
			ipasir_add(solver, 0);
		}
	}
}

int main(int argc, char **argv) {
	signal(SIGTERM, signalHandler);
	puts("Usage: ./lsp <graph-file> [<start>] [<goal>]");
	puts("       default start=1, goal=#vertices");
	printf("Using the SAT solver %s\n", ipasir_signature());
	Graph g = loadGraph(argv[1], true);
	int start = 1;
	int goal = g.verticesCount;
	if (argc == 4) {
		start = atoi(argv[2]);
		goal = atoi(argv[3]);
	}
	log("INFO: searching for a longest path from %d to %d\n", start, goal);

	int lb, ub;
	getLowerUpperBound(g, start, goal, lb, ub);
	if (lb > 0) {
		log("INFO: DFS found path of length %d, upper bound is %d\n", lb, ub);
	} else {
		log("INFO: no path between start and goal, %d reachable vertices from start\n", ub);
		return 0;
	}

	void* solver = ipasir_init();
	addInitalCondition(solver, g, start);
	best = lb;
	pathLength = lb;
	for (int i = 0; i < pathLength; i++) {
		addUniversalConditions(solver, g, i);
		if (i > 0) {
			addTransitionalConditions(solver, g, i);
		}
	}

	while (pathLength < ub) {
		addUniversalConditions(solver, g, pathLength);
		addTransitionalConditions(solver, g, pathLength);
		assumeGoalCondition(solver, g, goal, pathLength);
		log("INFO: trying to find a path of length %d by SAT\n", pathLength+1);
		int res = ipasir_solve(solver);
		if (res == 10) {
			log(" :-) path found!\n");
			best = pathLength+1;
		} else {
			log(" :-( path does not exist\n");
		}
		pathLength++;
	}
	log("RES: Longest path has %d vertices.\n", best);
	return 0;
}
