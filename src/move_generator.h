#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
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
        assert(src + dst != 0);
        *(e++) = src | dst << 6 | p << 12 | s << 16;
    }
    bool contains(Move move)
    {
        return std::find(_m, e, move) != e;
    }
    void extract(Move move)
    {
        if (move == No_move)
            return;
        Move* pv = std::find(_m, c, move);
        //assert(std::find(_m, c, move) != c);
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
    void set()
    {
        c = e;
    }
    void clear()
    {
        c = _m;
        e = _m;
    }
    void order_killer(Move* killers)
    {
        Move* k;
        int offset, i;

        offset = 1;
        for (i = 0; i < Killer_size; ++i)
        {
            k = std::find(_m, c, *(killers + i));
            if (k != c)
            {
                std::swap(*k, *(std::find_if_not(k, c, is_quiet)-offset));
                offset++;
            }
        }
    }

    Move _m[Max_size];
    Move * c;
    Move * e;

    friend std::ostream & operator << (std::ostream & o, const MoveList & mlist);
};

void mg_init();
void push_moves(State &, MoveList *);

#endif