#include "zobrist.h"
#include "state.h"

namespace Zobrist
{
	U64 rand_64()
	{
		return U64(rand()) << 32 | U64(rand());
	}

	void init()
	{
		srand(6736199);
		for (PieceType p = pawn; p <= none; ++p)
		{
			for (Square s = first_sq; s <= last_sq; ++s)
			{
				piece_rand[white][p][s] = rand_64();
				piece_rand[black][p][s] = rand_64();
			}
		}
		for (File f = a_file; f <= h_file; ++f)
		{
			ep_file_rand[f] = rand_64();
		}
		for (int i = 0; i < 16; ++i)
		{
			castle_rand[i] = rand_64();
		}
		side_to_move_rand = rand_64();
	}

	void init_pieces(State * s)
	{
		for (Square sq = first_sq; sq <= last_sq; ++sq)
		{
			if (s->on_square(sq, s->us) != none)
			{
				s->key ^= piece_rand[s->us][s->on_square(sq, s->us)][sq];
			} 
			else if (s->on_square(sq, s->them) != none)
			{
				s->key ^= piece_rand[s->them][s->on_square(sq, s->them)][sq];
			}
			else
			{
				s->key ^= piece_rand[s->us][none][sq];
			}
		}

		if (s->us == white) s->key ^= side_to_move_rand;

		if (s->ep) 
		{
			s->key ^= ep_file_rand[get_file(s->ep)]; 
		}

		s->key ^= castle_rand[s->castle];
	}

	void move(State * s, int src, int dst)
	{
		s->key ^= piece_rand[s->us  ][s->board[  s->us][src]][src];
		s->key ^= piece_rand[s->us  ][s->board[  s->us][src]][dst];
		s->key ^= piece_rand[s->them][s->board[s->them][dst]][dst];
	}

	void promo(State * s, int src, int dst, int piece)
	{
		s->key ^= piece_rand[s->us  ][pawn ][src];
		s->key ^= piece_rand[s->us  ][piece][dst];
		s->key ^= piece_rand[s->them][s->board[s->them][dst]][dst];
	}

	void en_passant(State * s, int src, int dst, int ep)
	{
		s->key ^= piece_rand[s->us  ][pawn ][src];
		s->key ^= piece_rand[s->us  ][pawn ][dst];
		s->key ^= piece_rand[s->them][none ][dst];
		s->key ^= piece_rand[s->them][pawn ][ep];
	}

	void ep(U64 & key, U64 ep)
	{
		key ^= ep_file_rand[get_file(ep)];
	}

	void castle(U64 & key, int castle)
	{
		key ^= castle_rand[castle];
	}

	void turn(U64 & key)
	{
		key ^= side_to_move_rand;
	}

};