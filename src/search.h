#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>
#include <stack>
#include <sstream>
#include "evaluation.h"
#include "move_generator.h"
#include "state.h"
#include "types.h"
#include "transpositiontable.h"
#include "timer.h"

extern int search_nodes;
extern int table_hits;

struct History
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
    History history[1024];
    History* e;
    History* r;
};

struct RootMove
{
    RootMove() : move(No_move), score(0), prev_score(0)
    {}
    RootMove(Move m, int s) : move(m), score(s), prev_score(s)
    {}
    bool operator<(const RootMove& c) const
    {
        return score < c.score;
    }
    bool operator>(const RootMove& c) const
    {
        return score > c.score;
    }
    Move move;
    int score;
    int prev_score;
};

struct PV
{
    PV() {};
    PV(Move m, U64 k) : move(m), key(k) {}
    Move move;
    U64 key;
};

struct SearchInfo
{
    SearchInfo() : move_time(0), nodes(0), moves_to_go(0), quit(false)
    {}
    int time[Player_size], inc[Player_size];
    int moves_to_go, depth, max_nodes, nodes, mate, move_time;
    int64_t start_time;
    bool infinite, ponder, quit;
    std::vector<Move> sm;
};

extern GameList glist;
void setup_search(State& s, SearchInfo& si);
Move search(State& s, SearchInfo& si, std::vector<RootMove>& rmoves);
void search_init();

#endif  