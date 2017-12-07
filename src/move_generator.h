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
    Check(State & s) : checker(s.getCheckersBB()), checks(pop_count(checker))
    {
        checks == 1 ? ray = between_dia[s.getKingSquare(s.getOurColor())][get_lsb(checker)] 
                          | between_hor[s.getKingSquare(s.getOurColor())][get_lsb(checker)]
                    : ray = checker = Full;
        validKingMoves = s.valid_king_moves();
    }
    U64 checker;
    U64 ray;
    U64 validKingMoves;
    int checks;
};

class MoveList
{
public:
    MoveList() 
    {
        mSize = 0;
        mStage = 0;
    }
    MoveList(State& s, Move_t& pBest, Move_t& pKiller1, Move_t& pKiller2)
    : mBest(pBest)
    , mKiller1(pKiller1)
    , mKiller2(pKiller2)
    {
        push_moves(s);
    }
    ~MoveList() {}
    std::size_t size() const
    { 
        return mSize;
    }
    void push(Move_t m)
    {
        mList[mSize++] = m;
    }
    bool contains(Move_t move)
    {
        return std::find(mList.begin(), mList.begin() + mSize, move)
            != mList.begin() + mSize;
    }
    Move_t getBestMove()
    {

    }
    void extract(Move_t move)
    {
        //if (move == nullMove)
            //return;
        //Move_t* pv = std::find(_m, c, move);
        //assert(std::find(_m, c, move) != c);
        //std::swap(*pv, *(--c));
    }
    Move_t pop() 
    { 
        return mList[--mSize];
    }
    template<Color C>
    void pushPawnMoves(State& s, Check& ch);
    template<Color C>
    void pushPawnAttacks(State& s, Check& ch);
    template<Color C>
    void push_pawn_moves(State& s, Check& ch);
    template<PieceType P>
    void push_moves(State& s, Check& ch);
    void push_king_moves(State& s, Check& ch);
    void check_legal(State& s);
    template<Color C>
    void push_all(State& s, Check& ch);
    template<PieceType P>
    void pushMoves(State& s, Check& ch);
    template<PieceType P>
    void pushAttackMoves(State& s, Check& ch);
    void pushKingAttacks(State& s, Check& ch);
    void pushKingMoves(State& s, Check& ch);
    void push_moves(State& s);
    void pushQuietChecks(State& s, Check ch);
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
    void clear()
    {
        mSize = 0;
        //c = _m;
        //e = _m;
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
    int mStage;
    std::array<Move_t, Max_size> mList;
    std::size_t mSize;
    Move_t mBest;
    Move_t mKiller1;
    Move_t mKiller2;

    friend std::ostream & operator << (std::ostream & o, const MoveList & mlist);
};

void mg_init();
void push_moves(State &, MoveList *);

#endif