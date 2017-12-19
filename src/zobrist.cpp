#include "zobrist.h"

namespace Zobrist
{
	U64 piece_rand[Player_size][Types_size][Board_size];
	U64 ep_file_rand[8];
	U64 castle_rand[16];
	U64 side_to_move_rand;

	U64 rand_64()
	{
		return U64(rand()) << 32 | U64(rand());
	}

	void init()
	{
		srand(6736199);
		for (PieceType p = pawn; p < none; ++p)
		{
			for (Square s = first_sq; s <= last_sq; ++s)
			{
				piece_rand[white][p][s] = rand_64();
				piece_rand[black][p][s] = rand_64();
			}
		}
		std::cout << piece_rand[black][pawn][52] << '\n';
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
};