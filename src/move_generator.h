//=============================================================================
// Generation of all legal moves for search
//=============================================================================

#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <cmath>
#include "state.h"
#include "bitboard.h"
#include "MagicMoves.hpp"
#include "move.h"
#include "types.h"

const U64 DEFAULT = 0xFFFFFFFFFFFFFFFF;

namespace MoveGenerator
{
	void init();
	void push_moves(State *, MoveList *);
	void push_evasions(State *, MoveList *, U64);
	void push_pawn_moves(State * s, MoveList *, U64 checker=DEFAULT, U64 ray=DEFAULT);
	void push_knight_moves(State * s, MoveList *, U64 ray=DEFAULT, U64 checker=DEFAULT);
	void push_bishop_moves(State * s, MoveList *, U64 ray=DEFAULT, U64 checker=DEFAULT);
	void push_rook_moves(State * s, MoveList *, U64 ray=DEFAULT, U64 checker=DEFAULT);
	void push_queen_moves(State * s, MoveList *, U64 ray=DEFAULT, U64 checker=DEFAULT);
	void push_king_moves(State *, MoveList *, int);
	U64 valid_king_moves(State *);
	void check_pins(State * s, MoveList * mlist);
}

#endif