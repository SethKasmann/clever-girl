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

class MoveList
{
public:
    MoveList(const State& pState, bool pQSearch=false)
    : mState(pState), mValid(Full), mQSearch(pQSearch)
    {
        mSize = 0;
        mStage = 0;
        if (pop_count(mState.getCheckersBB()) == 1)
        {
            Square checker = get_lsb(mState.getCheckersBB());
            Square king = mState.getKingSquare(mState.getOurColor());
            mValid = between_dia[king][checker] | between_hor[king][checker]
                   | mState.getCheckersBB();
        }
        mValidKingMoves = mState.valid_king_moves();
        if (pop_count(mState.getCheckersBB()) == 2)
            mStage = mQSearch ? 11 : 13;
        else
            mStage = mQSearch ? 7 : 1;
    }
    ~MoveList() {}
    std::size_t size() const
    { 
        return mSize;
    }
    void push(Move_t m)
    {
        mList[mSize++].move = m;
    }
    bool contains(Move_t move)
    {
        return std::find(mList.begin(), mList.begin() + mSize, move)
            != mList.begin() + mSize;
    }
    Move_t getBestMove();
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
        return mList[--mSize].move;
    }
    void checkLegal();
    void generateAttacks();
    void generateQuiets();
    void generateQuietChecks();
    template<PieceType P> void pushQuietMoves();
    template<PieceType P> void pushAttackMoves();
    void push_moves();
    void pushQuietChecks();
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
    bool mQSearch;
    U64 mValid, mValidKingMoves;
    const State& mState;
    int mStage;
    std::array<MoveEntry, Max_size> mList;
    std::size_t mSize;
    Move_t mBest;
    Move_t mKiller1;
    Move_t mKiller2;

    friend std::ostream & operator << (std::ostream & o, const MoveList & mlist);
};

void mg_init();

#endif