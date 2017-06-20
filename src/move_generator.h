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
    /*
    void order_killer(Move move0, Move move1)
    {
        Move* k;
        int offset = 1;
        k = std::find(_m, c, move0);
        if (k != c)
        {
            std::cout << "I'm sorting a killer!\n";
            std::cout << "Killer move:" << to_string(*k) << '\n';
            std::cout << *this;
            std::swap(*k, *(std::find_if_not(k, c, is_quiet)-1));
            std::cout << "It's sorted!\n";
            std::cout << *this;
            int z;
            std::cin >> z;
        }
    }
    */

    Move _m[Max_size];
    Move * c;
    Move * e;

    friend std::ostream & operator << (std::ostream & o, const MoveList & mlist);
};

void mg_init();
void push_moves(State &, MoveList *);

#endif