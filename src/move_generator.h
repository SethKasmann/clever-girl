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
#include "move.h"

const U64 Full = 0xFFFFFFFFFFFFFFFF;
const int Max_size = 256;

struct Check
{
    Check(State & s) : checker(s.getCheckers()), checks(pop_count(checker))
    {
        checks == 1 ? ray = between_dia[s.getKingSquare(s.getOurColor())][get_lsb(checker)] 
                          | between_hor[s.getKingSquare(s.getOurColor())][get_lsb(checker)]
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
    void push(Move_t m)
    {
        *(e++) = m;
    }
    bool contains(Move_t move)
    {
        return std::find(_m, e, move) != e;
    }
    void extract(Move_t move)
    {
        if (move == nullMove)
            return;
        Move_t* pv = std::find(_m, c, move);
        //assert(std::find(_m, c, move) != c);
        std::swap(*pv, *(--c));
    }
    Move_t pop() 
    { 
        return *(--e);
    }
    void sort(const State& s)
    {
        /*
        for (Move_t* move = _m; move < c; ++move)
        {
            if (!is_quiet(*move))
            {
                if (s.see(*move) < 0){}
                    //set_score(move, BC);
            }
        }
        std::stable_sort(_m, c);
        */
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
    void order_killer(Move_t* killers)
    {
        /*
        Move_t* k;
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
        */
    }

    Move_t _m[Max_size];
    Move_t * c;
    Move_t * e;

    friend std::ostream & operator << (std::ostream & o, const MoveList & mlist);
};

void mg_init();
void push_moves(State &, MoveList *);

#endif