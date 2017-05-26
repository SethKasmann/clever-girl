#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <cmath>
#include "state.h"
#include "bitboard.h"
#include "MagicMoves.hpp"
#include "types.h"

const U64 FULL = 0xFFFFFFFFFFFFFFFF;
const int MAX_SIZE = 256;

struct Check
{
	Check(State & s) : checker(s.checkers()), checks(pop_count(checker))
	{
        checks == 1 ? ray = between_dia[s.p_king_sq()][get_lsb(checker)] 
            			  | between_hor[s.p_king_sq()][get_lsb(checker)]
        			: ray = checker = 0xFFFFFFFFFFFFFFFF;
	}
	U64 checker;
	U64 ray;
	int checks;
};

class MoveList
{
public:
	MoveList() : e(_m), c(_m) 
	{}
	~MoveList() 
	{}
	int size() { return e - _m; }
	void push(int src, int dst, int p, int s)
	{
		*(e++) = src | dst << 6 | p << 12 | s << 16;
	}
	Move pop() { return *(--e); }
private:
	Move _m[MAX_SIZE];
public:
	Move * c;
	Move * e;
};

void mg_init();
void push_moves(State &, MoveList *);
/*
template<Color C>
void push(State & s, MoveList * mlist);
*/

#endif