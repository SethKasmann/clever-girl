#include "test.h"

const int ccrTotalTests = 25;

const std::string ccrFen[ccrTotalTests] =
{
	"rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/2N1P3/PP3PPP/R1BQKBNR w KQkq",
	"rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/1QN1P3/PP3PPP/R1B1KBNR b KQkq",
	"r1bqk2r/ppp2ppp/2n5/4P3/2Bp2n1/5N1P/PP1N1PP1/R2Q1RK1 b kq",
	"r1bqrnk1/pp2bp1p/2p2np1/3p2B1/3P4/2NBPN2/PPQ2PPP/1R3RK1 w",
	"rnbqkb1r/ppp1pppp/5n2/8/3PP3/2N5/PP3PPP/R1BQKBNR b KQkq",
	"rnbq1rk1/pppp1ppp/4pn2/8/1bPP4/P1N5/1PQ1PPPP/R1B1KBNR b KQ",
	"r4rk1/3nppbp/bq1p1np1/2pP4/8/2N2NPP/PP2PPB1/R1BQR1K1 b",
	"rn1qkb1r/pb1p1ppp/1p2pn2/2p5/2PP4/5NP1/PP2PPBP/RNBQK2R w KQkq c6",
	"r1bq1rk1/1pp2pbp/p1np1np1/3Pp3/2P1P3/2N1BP2/PP4PP/R1NQKB1R b KQ",
	"rnbqr1k1/1p3pbp/p2p1np1/2pP4/4P3/2N5/PP1NBPPP/R1BQ1RK1 w",
	"rnbqkb1r/pppp1ppp/5n2/4p3/4PP2/2N5/PPPP2PP/R1BQKBNR b KQkq f3",
	"r1bqk1nr/pppnbppp/3p4/8/2BNP3/8/PPP2PPP/RNBQK2R w KQkq",
	"rnbq1b1r/ppp2kpp/3p1n2/8/3PP3/8/PPP2PPP/RNBQKB1R b KQ d3",
	"rnbqkb1r/pppp1ppp/3n4/8/2BQ4/5N2/PPP2PPP/RNB2RK1 b kq",
	"r2q1rk1/2p1bppp/p2p1n2/1p2P3/4P1b1/1nP1BN2/PP3PPP/RN1QR1K1 w",
	"r1bqkb1r/2pp1ppp/p1n5/1p2p3/3Pn3/1B3N2/PPP2PPP/RNBQ1RK1 b kq",
	"r2qkbnr/2p2pp1/p1pp4/4p2p/4P1b1/5N1P/PPPP1PP1/RNBQ1RK1 w kq",
	"r1bqkb1r/pp3ppp/2np1n2/4p1B1/3NP3/2N5/PPP2PPP/R2QKB1R w KQkq e6",
	"rn1qk2r/1b2bppp/p2ppn2/1p6/3NP3/1BN5/PPP2PPP/R1BQR1K1 w kq",
	"r1b1kb1r/1pqpnppp/p1n1p3/8/3NP3/2N1B3/PPP1BPPP/R2QK2R w KQkq",
	"r1bqnr2/pp1ppkbp/4N1p1/n3P3/8/2N1B3/PPP2PPP/R2QK2R b KQ",
	"r3kb1r/pp1n1ppp/1q2p3/n2p4/3P1Bb1/2PB1N2/PPQ2PPP/RN2K2R w KQkq",
	"r1bq1rk1/pppnnppp/4p3/3pP3/1b1P4/2NB3N/PPP2PPP/R1BQK2R w KQ",
	"r2qkbnr/ppp1pp1p/3p2p1/3Pn3/4P1b1/2N2N2/PPP2PPP/R1BQKB1R w KQkq",
	"rn2kb1r/pp2pppp/1qP2n2/8/6b1/1Q6/PP1PPPBP/RNB1K1NR b KQkq"
};

const std::string ccrResults[ccrTotalTests]
{
	"bm Qb3",
	"bm Bc8",
	"bm Nh6 am Ne5",
	"bm b4",
	"bm e5",
	"bm Bcx3+",
	"bm Rfb8",
	"bm d5",
	"bm Nd4",
	"bm a4",
	"bm d5",
	"bm Bxf7+",
	"am Ne4",
	"am Nxc4",
	"bm exf6",
	"bm d5",
	"am hxg4",
	"bm Bxf6+",
	"am Bxe6",
	"am Ndb5",
	"am Kxe6",
	"bm a4",
	"bm Bxh7+",
	"bm Nxe5",
	"am Qxb3",
};

void ccrTest()
{
	for (int i = 0; i < ccrTotalTests; ++i)
	{
		State s(ccrFen[i]);
		history.clear();
		history.push(std::make_pair(nullMove, s.getKey()));
		SearchInfo si;
		si.move_time = allocate_time(100000, 0, 40);
		si.start_time = system_time();
		// Clear the game list.
		ttable.clear();
		lineManager.clear_pv();
		search_init();

		int score;
		for (int d = 1; d < 6/*!si.quit*/; ++d)
		{
			score = scout_search(s, si, d, 0, Neg_inf, Pos_inf);
			if (si.quit)
				break;
		}

		Move_t pv = lineManager.get_pv_move();

		std::cout << s;
		std::cout << "bestmove " << to_string(s.onSquare(get_src(pv))) << toString(pv) << std::endl;
		std::cout << ccrResults[i] << '\n';
	}
}