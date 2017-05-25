#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <cmath>
#include "state.h"
#include "bitboard.h"
#include "MagicMoves.hpp"
#include "move.h"
#include "types.h"

const U64 FULL = 0xFFFFFFFFFFFFFFFF;

void mg_init();
void push_moves(State &, MoveList *);
void push_pawn_moves(State & s, MoveList *, U64 checker=FULL, U64 ray=FULL);
void push_king_moves(State &, MoveList *, int checks=0);
void check_legal(State & s, MoveList * mlist);
template <PieceType P>
void push_moves(State &, MoveList *, U64 checker=FULL, U64 ray=FULL);

#endif