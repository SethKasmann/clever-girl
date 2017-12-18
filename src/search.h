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
#include "misc.h"
#include "line_manager.h"
#include "move.h"
#include "history.h"

enum SearchType
{
    qSearch,
    scoutSearch
};

struct OldMove
{
    Move move;
    U64 key;
};

class GameList
{
public:
    GameList() : e(history), r(history)
    {}
    void push_root(Move m, U64 k)
    {
        e->move = m;
        e->key  = k;
        ++e;
        ++r;
    }
    void push(Move m, U64 k)
    {
        e->move = m;
        e->key  = k;
        ++e;
    }
    void operator--()
    {
        --e;
    }
    Move operator*() const
    {
        return e->move;
    }
    size_t size() const
    {
        return e - history;
    }
    int ply() const
    {
        return e - r - 1;
    }
    bool repeat()
    {
        return size() >= 5 ? ((e-1)->key == (e-5)->key) 
                           : false;
    }
    void clear()
    {
        e = history;
        r = history;
    }
private:
    OldMove history[1024];
    OldMove* e;
    OldMove* r;
};

struct SearchInfo
{
    SearchInfo() : moveTime(0), nodes(0), moves_to_go(0), quit(false), infinite(false)
    {
        time[white] = 0;
        time[black] = 0;
    }
    int time[Player_size], inc[Player_size];
    int moves_to_go, depth, max_nodes, nodes, mate, moveTime;
    int64_t start_time;
    bool infinite, ponder, quit;
    std::vector<Move> sm;
};

extern GameList glist;
extern History history;
extern LineManager lineManager;
void setup_search(State& s, SearchInfo& si);
void iterative_deepening(State& s, SearchInfo& si);
int scout_search(State& s, SearchInfo& si, int depth, int ply, int alpha, int beta);

#endif  