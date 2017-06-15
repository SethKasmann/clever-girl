#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <cmath>
#include <algorithm>
#include "state.h"
#include "bitboard.h"
#include "MagicMoves.hpp"
#include "types.h"

const U64 Full = 0xFFFFFFFFFFFFFFFF;
const int Max_size = 256;

struct Check
{
    Check(State & s) : checker(s.checkers()), checks(pop_count(checker))
    {
        checks == 1 ? ray = between_dia[s.king_sq(s.us)][get_lsb(checker)] 
                          | between_hor[s.king_sq(s.us)][get_lsb(checker)]
                    : ray = checker = Full;
    }
    U64 checker;
    U64 ray;
    int checks;
};

class MoveList
{
public:
    MoveList() : e(_m), c(_m) {}
    ~MoveList() {}
    int size() const
    { 
        return e - _m; 
    }
    void push(int src, int dst, int p, int s)
    {
        *(e++) = src | dst << 6 | p << 12 | s << 16;
    }
    void extract(Move move)
    {
        c = e;
        if (move == No_move)
            return;
        Move* pv = std::find(_m, c, move);
        std::swap(*pv, *(--c));
    }
    Move pop() 
    { 
        return *(--e);
    }
    void sort()
    {
        std::stable_sort(_m, c);
    }

    Move _m[Max_size];
    Move * c;
    Move * e;

    friend std::ostream & operator << (std::ostream & o, const MoveList & mlist);
};

void mg_init();
void push_moves(State &, MoveList *);

inline std::string to_string(Move m)
{
    return std::string { to_char(file(get_src(m))), 
                         to_char(rank(get_src(m))), 
                         ',',
                         to_char(file(get_dst(m))), 
                         to_char(rank(get_dst(m))) };
}   

#endif