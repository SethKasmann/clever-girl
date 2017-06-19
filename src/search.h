#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>
#include <stack>
#include "evaluation.h"
#include "move_generator.h"
#include "state.h"
#include "types.h"
#include "transpositiontable.h"

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
    GameList() : e(history)
    {}
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
    bool repeat()
    {
        return size() >= 5 ? ((e-1)->key == (e-5)->key) 
                           : false;
    }
    void clear()
    {
        e = history;
    }
    // Possible issue with this. It doesn't cover EP attacks.
    bool last_cap() const
    {
        return get_prop((e-1)->move) == attack;
    }
private:
    History history[1024];
    History* e;
};

struct Candidate
{
    Candidate(Move m, int s) : move(m), score(s)
    {}
    bool operator<(const Candidate& c) const
    {
        return score < c.score;
    }
    Move move;
    int score;
};

struct PV
{
    PV() {};
    PV(Move m, U64 k) : move(m), key(k) {}
    Move move;
    U64 key;
};

extern GameList glist;
Move search(State & s);

#endif  