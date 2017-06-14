#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>
#include "evaluation.h"
#include "move_generator.h"
#include "state.h"
#include "types.h"
#include "transpositiontable.h"

extern int search_nodes;
extern int table_hits;

struct Candidate
{
	Candidate(Move m, int s) : move(m), score(s)
	{}
	bool operator<(const Candidate& c) const
	{
		return score < c.score;
	}
	Move move;
	int score;
};

Move search(State & s);

#endif	