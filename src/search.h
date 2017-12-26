#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>
#include <stack>
#include <sstream>
#include <string>
#include <vector>
#include "evaluation.h"
#include "move_generator.h"
#include "state.h"
#include "types.h"
#include "transpositiontable.h"
#include "timer.h"
#include "line_manager.h"
#include "move.h"
#include "history.h"

static const int LmrCount = 3;
static const int LmrDepth = 2;
static const int NullMoveCount = 3;
static const int NullMoveDepth = 3;

enum SearchType
{
    qSearch,
    scoutSearch
};

struct SearchInfo
{
    SearchInfo() : moveTime(0), nodes(0), moves_to_go(0), quit(false), infinite(false)
    {
        time[white] = 0;
        time[black] = 0;
    }
    int time[Player_size], inc[Player_size];
    int moves_to_go, depth, max_nodes, nodes, mate;
    int64_t moveTime;
    Clock clock;
    bool infinite, ponder, quit;
    std::vector<Move> sm;
};

extern History history;
extern LineManager lineManager;
void setup_search(State& s, SearchInfo& si);
void iterative_deepening(State& s, SearchInfo& si);
int scout_search(State& s, SearchInfo& si, int depth, int ply, int alpha, int beta, bool isPv, bool isNull);

#endif  