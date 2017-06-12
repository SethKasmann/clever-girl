#include "perft.h"

const int Total_tests = 19;
int nodes = 0;

// Fen positions for perft testing
std::string fen[Total_tests] =
{
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq",
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq",
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq",
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq",
	"1k6/1b6/8/8/7R/8/8/4K2R b K",
	"3k4/3p4/8/K1P4r/8/8/8/8 b",
	"8/8/4k3/8/2p5/8/B2P2K1/8 w",
	"8/8/1k6/2b5/2pP4/8/5K2/8 b - d3",
	"5k2/8/8/8/8/8/8/4K2R w K",
	"3k4/8/8/8/8/8/8/R3K3 w Q",
	"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq",
	"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq",
	"2K2r2/4P3/8/8/8/8/8/3k4 w",
	"8/8/1P2K3/8/2n5/1q6/8/5k2 b",
	"4k3/1P6/8/8/8/8/K7/8 w",
	"8/P1k5/K7/8/8/8/8/8 w",
	"K1k5/8/P7/8/8/8/8/8 w",
	"8/k1P5/8/1K6/8/8/8/8 w",
	"8/8/2k5/5q2/5n2/8/5K2/8 b"
};

// Depth for each fen position for perft testing
const int depth[Total_tests] = 
{ 
	3, 4, 5, 6, 
	5, 6, 6, 6, 
	6, 6, 4, 4, 
	6, 5, 6, 6, 
	6, 7, 4 
};

// Nodes generated for each fen position for perft testing
const int results[Total_tests] = 
{
	8902, 		197281, 	4865609, 	119060324, 
	1063513, 	1134888, 	1015133, 	1440467, 
	661072, 	803711, 	1274206, 	1720476, 
	3821001, 	1004658,	217342, 	92683, 
	2217, 		567584, 	23527
};

void perft_test()
{
	std::cout << "Perft Results:\n"
			  << "----------------------------------------------\n";
	for (int i = 0; i < Total_tests; ++i)
	{
		nodes = 0;
		State s(fen[i]);
		perft_tree(s, depth[i]);
		std::cout << fen[i] << '\n'
				  << (nodes == results[i] ? "Success\n" : "Fail\n")
				  << nodes << " == " << results[i] << '\n'
				  << "----------------------------------------------\n";
	}
}

void perft(std::string & fen, int depth)
{
	nodes = 0;
	State s(fen);
	std::cout << s;
	Timer::start_clock("perft");
	perft_tree(s, depth);
	const float T = Timer::clock_time("perft")/1000.0;
	std::cout << "Time:" << T << '\n'
			  << "NPS :" << nodes/T << '\n';
}

void perft_tree(State & s, int depth)
{
	if (s.fmr == 100)
		return;

	MoveList mlist;
	push_moves(s, &mlist);

	if (mlist.size() == 0)
		return;

	if (depth == 1)
	{
		nodes += mlist.size();
		return;
	}

	State c;
	std::cout << mlist;
	mlist.sort();
	std::cout << mlist;
	int z;
	std::cin >> z;
	while (mlist.size() > 0)
	{
		std::memmove(&c, &s, sizeof s);
		c.make(mlist.pop());
		perft_tree(c, depth-1);
	}
	return;
}