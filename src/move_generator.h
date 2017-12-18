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
const int maxSize = 256;

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
    MoveList(const State& pState, Move pBest, History* pHistory, int pPly, bool pQSearch=false);
    MoveList(const State& pState);
    std::size_t size() const;
    void push(Move m);
    bool contains(Move move) const;
    Move getBestMove();
    Move pop();
    void checkLegal();
    void pushPawnCaptures();
    void generateAttacks();
    void generateQuiets();
    void generateQuietChecks();
    void generateAllMoves();
    template<PieceType P> void pushQuietMoves();
    template<PieceType P> void pushAttackMoves();
    void pushQuietChecks();
    friend std::ostream & operator << (std::ostream & o, const MoveList & mlist);
private:
    bool mQSearch;
    U64 mValid;
    const State& mState;
    const History* mHistory;
    int mPly;
    int mStage;
    std::array<MoveEntry, maxSize> mList;
    std::vector<MoveEntry> badCaptures;
    std::size_t mSize;
    Move mBest;
    Move mKiller1;
    Move mKiller2;
};

void mg_init();

#endif