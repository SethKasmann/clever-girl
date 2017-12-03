#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <cstdlib>
#include <iostream>
#include "bitboard.h"
#include "types.h"

class State;

namespace Zobrist
{
	extern U64 piece_rand[Player_size][Types_size][Board_size];
	extern U64 ep_file_rand[8];
	extern U64 castle_rand[16];
	extern U64 side_to_move_rand;

	void init();
	void init_pieces(State *);
	void init_pawn_key(State * s);

	// Side to move.
	inline U64 key()
	{
		return side_to_move_rand;
	}

	// Castle rights.
	inline U64 key(int castle)
	{
		return castle_rand[castle];
	}

	// En-passant file.
	inline U64 key(File ep)
	{
		return ep_file_rand[ep];
	}

	// Remove/add a piece.
	inline U64 key(Color c, PieceType p, Square src)
	{
		return piece_rand[c][p][src];
	}

	// Moving a piece.
	inline U64 key(Color c, PieceType p, Square src, Square dst)
	{
		return piece_rand[c][p][src] ^ piece_rand[c][p][dst];
	}

};

#endif