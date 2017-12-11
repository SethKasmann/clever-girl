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
#include "history.h"

const U64 Full = 0xFFFFFFFFFFFFFFFF;
const int Max_size = 256;

enum MoveStage
{
    nBestMove,
    nAttacksGen,
    nAttacks,
    nKiller1,
    nKiller2,
    nQuietsGen,
    nQuiets,
    qBestMove,
    qAttacksGen,
    qAttacks,
    qQuietChecksGen,
    qQuietChecks,
    qKingEvadeBestMove,
    qKingEvadeAttacksGen,
    qKingEvadeAttacks,
    nKingEvadeBestMove,
    nKingEvadeMovesGen,
    nKingEvadeMoves,
    allLegal
};

class MoveList
{
public:
    MoveList(const State& pState, Move_t pBest, History* pHistory, int pPly, bool pQSearch=false)
    : mState(pState), mValid(Full), mBest(pBest), mQSearch(pQSearch), mKiller1(nullMove), mKiller2(nullMove)
    , mSize(0), mHistory(pHistory), mPly(pPly)
    {
        if (pHistory)
        {
            mKiller1 = mHistory->getKiller(pPly).first;
            mKiller2 = mHistory->getKiller(pPly).second;
        }
        if (pop_count(mState.getCheckersBB()) == 1)
        {
            Square checker = get_lsb(mState.getCheckersBB());
            Square king = mState.getKingSquare(mState.getOurColor());
            mValid = between_dia[king][checker] | between_hor[king][checker]
                   | mState.getCheckersBB();
            mStage = nBestMove;
        }
        else
            mStage = mQSearch ? qBestMove : nBestMove;
        mValidKingMoves = mState.valid_king_moves();
        if (pop_count(mState.getCheckersBB()) == 2)
            mStage = mQSearch ? qKingEvadeBestMove : nKingEvadeBestMove;
    }
    MoveList(const State& pState)
    : mState(pState), mValid(Full), mQSearch(false), mBest(nullMove)
    , mSize(0), mHistory(nullptr), mPly(0)
    {
        if (pop_count(mState.getCheckersBB()) == 1)
        {
            Square checker = get_lsb(mState.getCheckersBB());
            Square king = mState.getKingSquare(mState.getOurColor());
            mValid = between_dia[king][checker] | between_hor[king][checker]
                   | mState.getCheckersBB();
        }
        mValidKingMoves = mState.valid_king_moves();
        pushAllLegal();
        mStage = allLegal;
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
    void pushAllLegal();
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
    const History* mHistory;
    int mPly;
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