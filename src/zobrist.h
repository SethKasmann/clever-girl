#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <cstdlib>
#include <iostream>
#include "bitboard.h"
#include "types.h"

class State;

namespace Zobrist
{
	static U64 piece_rand[PLAYER_SIZE][TYPES_SIZE+1][BOARD_SIZE];
	static U64 ep_file_rand[8];
	static U64 castle_rand[16];
	static U64 side_to_move_rand;
	void init();
	void init_pieces(State *);
	void move(State *, int, int);
	void promo(State *, int, int, int);
	void en_passant(State *, int, int, int);
	void ep(U64 &, U64);
	void castle(U64 &, int);
	void turn(U64 &);
};

#endif