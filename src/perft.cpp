#include <thread>
#include <vector>
#include <condition_variable>
#include <numeric>
#include "perft.h"

const int totalTests = 19;

// Fen positions for perft testing
const std::string perftFen[totalTests] =
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
const int perftDepth[totalTests] = 
{ 
	3, 4, 5, 6, 
	5, 6, 6, 6, 
	6, 6, 4, 4, 
	6, 5, 6, 6, 
	6, 7, 4 
};

// Nodes generated for each fen position for perft testing
const int perftResults[totalTests] = 
{
	8902, 		197281, 	4865609, 	119060324, 
	1063513, 	1134888, 	1015133, 	1440467, 
	661072, 	803711, 	1274206, 	1720476, 
	3821001, 	1004658,	217342, 	92683, 
	2217, 		567584, 	23527
};

static History history;

int perft(State & s, int depth)
{
	int nodes = 0;

	if (s.getFiftyMoveRule() > 99)
		return nodes;

	MoveList mlist(s);

	if (depth == 1)
		return mlist.size();

	Move m;

	while (m = mlist.getBestMove())
	{
		State c(s);
		c.make_t(m);
		nodes += perft(c, depth-1);
	}

	return nodes;
}

bool ready = false;
std::condition_variable cv;
std::mutex qMutex;
std::mutex lock1;
std::mutex lock2;
std::vector<int> nodeCount;

void test(State s, MoveList* mList, int depth, int id)
{
	{
		std::unique_lock<std::mutex> lk(qMutex);
		cv.wait(lk, [] { return ready; });
	}

	Move m;

	{
		std::lock_guard<std::mutex> lk(qMutex);
		m = mList->getBestMove();
	}

	while (m != nullMove)
	{
		State c(s);
		c.make_t(m);
		int nodes = perft(c, depth - 1);
		
		{
			std::lock_guard<std::mutex> lk(qMutex);
			nodeCount.push_back(nodes);
			m = mList->getBestMove();
		} 
	}
}

int MTperft(State& s, int depth)
{
	unsigned int nThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	nodeCount.clear();
	MoveList mlist(s);

	for (unsigned int i = 0; i < nThreads; ++i)
		threads.push_back(std::thread(test, s, &mlist, depth, i));

	{
		std::lock_guard<std::mutex> lk(qMutex);
		ready = true;
		cv.notify_all();
	}

	for (std::thread& thread : threads)
		thread.join();

	return std::accumulate(nodeCount.begin(), nodeCount.end(), 0);
}

void printPerft(const std::string& fen, int maxDepth)
{
	int nodes;
	double nps, time;
	Clock clock;
	State s(fen);

	std::cout << s;
	std::cout << ' ' << std::setfill('-') << std::setw(53) << std::right << ' '
		      << std::endl;
	std::cout << "| perft     | nodes       | nps                      |"
	          << std::endl;
    std::cout << "|" << std::setfill('-') << std::setw(53) << std::right << "|"
	          << std::endl;
	for (int depth = 2; depth <= maxDepth; ++depth)
	{
		nodes = 0;
		clock.set();
		nodes = MTperft(s, depth);
		time = clock.elapsed<std::chrono::microseconds>() / static_cast<double>(1000000);
		std::cout << "| " << std::setfill(' ') << std::setw(10) << std::left << depth
		          << "| " << std::setw(12) << std::left << nodes
		          << "| ";
		if (time > 0.0)
			std::cout << std::setw(25) << std::left << std::scientific 
		              << std::setprecision(2) 
		              << static_cast<double>(nodes) / time
		              << "|" << std::endl;
		else
			std::cout << std::setw(26) << std::right << "|" << std::endl;
	}

    std::cout << ' ' << std::setfill('-') << std::setw(53) << std::right << ' '
	          << std::endl;
}

void perftTest()
{
	for (int i = 0; i < totalTests; ++i)
	{
		printPerft(perftFen[i], perftDepth[i]);
	}
}

void perftTestDebug()
{
	int nodes;
	for (int i = 0; i < totalTests; ++i)
	{
		history.clear();
		State s(perftFen[i]);
		history.push(std::make_pair(nullMove, s.getKey()));
		nodes = MTperft(s, perftDepth[i]);
		//std::cout << perftFen[i] << std::endl;
		std::cout << (nodes == perftResults[i] ? "Success " : "Fail ")
		          << nodes << " == " << perftResults[i] << std::endl;
	}
}