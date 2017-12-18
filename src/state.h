//=============================================================================
// A struct to keep track of the game state
//=============================================================================

#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <array>
#include "pst.h"
#include "bitboard.h"
#include "types.h"
#include "move.h"
#include "zobrist.h"

class State
{
public:
    State() {};
    State(const std::string &);
    State(const State & s);
    void operator=(const State &);
    void init();

    Color getOurColor() const;
    Color getTheirColor() const;
    int getFiftyMoveRule() const;
    U64 getKey() const;
    U64 getPawnKey() const;
    U64 getEnPassantBB() const;
    int getCastleRights() const;
    U64 getCheckersBB() const;
    PieceType onSquare(const Square s) const;
    Square getKingSquare(Color c) const;
    int getPstScore(GameStage g) const;

    // Access piece bitboards.
    template<PieceType P> const std::array<Square, Piece_max>& getPieceList(Color pColor) const;
    template<PieceType P> U64 getPieceBB(Color c) const;
    template<PieceType P> U64 getPieceBB() const;
    template<PieceType P> int getPieceCount(Color c) const;
    template<PieceType P> int getPieceCount() const;

    // Board occupancy
    U64 getOccupancyBB() const;
    U64 getOccupancyBB(Color c) const;
    U64 getEmptyBB() const;

    // Castle rights.
    bool canCastleKingside() const;
    bool canCastleQueenside() const;
    bool isQuiet(Move pMove) const;
    bool isCapture(Move pMove) const;
    bool isValid(Move pMove, U64 pValid) const;

    // Functions involved in making a move.
    void make_t(Move m);
    void addPiece(Color pColor, PieceType pPiece, Square pSquare);
    void movePiece(Color pColor, PieceType pPiece, Square pSrc, Square pDst);
    void removePiece(Color pColor, PieceType pPiece, Square pSquare);
    void swapTurn();

    // Valid king moves and pins.
    U64 getValidKingMoves() const;
    void setCheckers();
    void setPins(Color c);
    U64 getPinsBB(Color c) const;
    U64 getDiscoveredChecks(Color c) const;

    // Check and attack information.
    bool isLegal(Move pMove) const;
    bool isAttacked(Square pSquare, Color pColor, U64 pChange) const;
    bool attacked(Square s) const;
    bool defended(Square s, Color c) const;
    bool check() const;
    bool check(U64 change) const;
    bool check(U64 change, Color c) const;
    bool inCheck() const;
    bool inDoubleCheck() const;
    U64 getAttackersBB(Square s, Color c) const;
    U64 allAttackers(Square s) const;
    template<PieceType> U64 getAttackBB(Square s, Color c=white) const;
    int see(Move m) const;
    U64 getXRayAttacks(Square square) const;

    // Print
    friend std::ostream & operator << (std::ostream & o, const State & state);

private:
    Color mUs;
    Color mThem;
    int mFiftyMoveRule;
    int mCastleRights;
    U64 mKey;
    U64 mPawnKey;
    U64 mCheckers;
    U64 mEnPassant;
    std::array<U64, Types_size> mCheckSquares;
    std::array<U64, Player_size> mPinned;
    std::array<U64, Player_size> mOccupancy;
    std::array<int, Board_size> mPieceIndex;
    std::array<PieceType, Board_size> mBoard;
    std::array<std::array<U64, Types_size>, Player_size> mPieces;
    std::array<std::array<int, Types_size>, Player_size> mPieceCount;
    std::array<std::array<int, gameStageSize>, Player_size> mPstScore;
    std::array<std::array<std::array<Square, Piece_max>, Types_size>, Player_size> mPieceList;
};

inline Color State::getOurColor() const
{
    return mUs;
}

inline Color State::getTheirColor() const
{
    return mThem;
}

inline int State::getFiftyMoveRule() const
{
    return mFiftyMoveRule;
}

inline U64 State::getKey() const
{
    return mKey;
}

inline U64 State::getPawnKey() const
{
    return mPawnKey;
}

inline U64 State::getEnPassantBB() const
{
    return mEnPassant;
}

inline int State::getCastleRights() const
{
    return mCastleRights;
}

inline U64 State::getPinsBB(Color c) const
{
    return mPinned[c];
}

inline bool State::isCapture(Move pMove) const
{
    return square_bb[getDst(pMove)] & (getOccupancyBB(mThem) | mEnPassant);
}

inline bool State::isQuiet(Move m) const
{
    return !isCapture(m);
}

template<PieceType P>
inline const std::array<Square, Piece_max>& State::getPieceList(Color pColor) const
{
    return mPieceList[pColor][P];
}

inline void State::addPiece(Color pColor, PieceType pPiece, Square pSquare)
{
    mPieces[pColor][pPiece] |= square_bb[pSquare];
    mOccupancy[pColor] |= square_bb[pSquare];
    mBoard[pSquare] = pPiece;
    mPieceIndex[pSquare] = mPieceCount[pColor][pPiece]++;
    mPieceList[pColor][pPiece][mPieceIndex[pSquare]] = pSquare;

    mPstScore[pColor][middle] += PieceSquareTable::pst[pPiece][middle][pColor][pSquare];
    mPstScore[pColor][late] += PieceSquareTable::pst[pPiece][late][pColor][pSquare];

    mKey ^= Zobrist::key(pColor, pPiece, pSquare);
}

inline void State::movePiece(Color pColor, PieceType pPiece, Square pSrc, Square pDst)
{
    mPieces[pColor][pPiece] ^= square_bb[pSrc] | square_bb[pDst];
    mOccupancy[pColor] ^= square_bb[pSrc] | square_bb[pDst];
    mBoard[pDst] = pPiece;
    mBoard[pSrc] = none;
    mPieceIndex[pDst] = mPieceIndex[pSrc];
    mPieceList[pColor][pPiece][mPieceIndex[pDst]] = pDst;

    mPstScore[pColor][middle] -= PieceSquareTable::pst[pPiece][middle][pColor][pSrc];
    mPstScore[pColor][late] -= PieceSquareTable::pst[pPiece][late][pColor][pSrc];
    mPstScore[pColor][middle] += PieceSquareTable::pst[pPiece][middle][pColor][pDst];
    mPstScore[pColor][late] += PieceSquareTable::pst[pPiece][late][pColor][pDst];

    mKey ^= Zobrist::key(pColor, pPiece, pSrc, pDst);
}

inline void State::removePiece(Color pColor, PieceType pPiece, Square pSquare)
{
    Square swap;
    int pieceCount;

    mPieces[pColor][pPiece] &= ~(square_bb[pSquare]);
    mOccupancy[pColor] &= ~(square_bb[pSquare]);
    mBoard[pSquare] = none;

    pieceCount = --mPieceCount[pColor][pPiece];

    swap = mPieceList[pColor][pPiece][pieceCount];
    mPieceIndex[swap] = mPieceIndex[pSquare];
    mPieceList[pColor][pPiece][mPieceIndex[swap]] = swap;
    mPieceList[pColor][pPiece][pieceCount] = no_sq;

    mPstScore[pColor][middle] -= PieceSquareTable::pst[pPiece][middle][pColor][pSquare];
    mPstScore[pColor][late] -= PieceSquareTable::pst[pPiece][late][pColor][pSquare];

    mKey ^= Zobrist::key(pColor, pPiece, pSquare);
}

inline
void State::swapTurn()
{
    mThem =  mUs;
    mUs   = !mUs;
}

template<PieceType P>
inline U64 State::getPieceBB(Color c) const
{
    return mPieces[c][P];
}

template<PieceType P>
inline U64 State::getPieceBB() const
{
    return mPieces[white][P] | mPieces[black][P];
}

template<PieceType P>
inline int State::getPieceCount() const
{
    return mPieceCount[white][P] + mPieceCount[black][P];
}

template<PieceType P>
inline int State::getPieceCount(Color c) const
{
    return mPieceCount[c][P];
}

inline
Square State::getKingSquare(Color c) const
{
    return mPieceList[c][king][0];
}

inline
int State::getPstScore(GameStage g) const
{
    return mPstScore[mUs][g] - mPstScore[mThem][g];
}

inline
PieceType State::onSquare(const Square s) const
{
    return mBoard[s];
}

inline
U64 State::getOccupancyBB() const
{
    return mOccupancy[white] | mOccupancy[black];
}

inline
U64 State::getOccupancyBB(Color c) const
{
    return mOccupancy[c];
}

inline
U64 State::getEmptyBB() const
{
    return ~(mOccupancy[white] | mOccupancy[black]);
}

inline
U64 State::getCheckersBB() const
{
    return mCheckers;
}

inline 
bool State::canCastleKingside() const
{
    return mUs ? mCastleRights & b_king_castle : mCastleRights & w_king_castle;
}

inline
bool State::canCastleQueenside() const
{
    return mUs ? mCastleRights & b_queen_castle : mCastleRights & w_queen_castle;
}

template<PieceType Pawn>
inline U64 State::getAttackBB(Square s, Color c) const
{
    return pawn_attacks[c][s];
}

template<>
inline U64 State::getAttackBB<knight>(Square s, Color c) const
{
    return Knight_moves[s];
}

template<>
inline U64 State::getAttackBB<bishop>(Square s, Color c) const
{
    return Bmagic(s, getOccupancyBB());
}

template<>
inline U64 State::getAttackBB<rook>(Square s, Color c) const
{
    return Rmagic(s, getOccupancyBB());
}

template<>
inline U64 State::getAttackBB<queen>(Square s, Color c) const
{
    return Qmagic(s, getOccupancyBB());
}

template<>
inline U64 State::getAttackBB<king>(Square s, Color c) const
{
    return King_moves[s];
}

inline
void State::setCheckers()
{
    mCheckers = getAttackersBB(getKingSquare(mUs), mThem);
    mCheckSquares[pawn] = getAttackBB<pawn>(getKingSquare(mUs), mUs);
    mCheckSquares[knight] = getAttackBB<knight>(getKingSquare(mUs));
    mCheckSquares[bishop] = getAttackBB<bishop>(getKingSquare(mUs));
    mCheckSquares[rook] = getAttackBB<rook>(getKingSquare(mUs));
    mCheckSquares[queen] = mCheckSquares[bishop] | mCheckSquares[rook];
}

inline
bool State::defended(Square s, Color c) const
{
    return getAttackBB<pawn>(s, !c)  &  getPieceBB< pawn >(c)
        || getAttackBB<knight>(s) &  getPieceBB<knight>(c)
        || getAttackBB<bishop>(s) & (getPieceBB<bishop>(c) | getPieceBB<queen>(c))
        || getAttackBB< rook >(s) & (getPieceBB< rook >(c) | getPieceBB<queen>(c));
}

inline
bool State::isAttacked(Square pSquare, Color pColor, U64 pChange) const
{
    U64 occupancy = getOccupancyBB() ^ pChange;
    return getAttackBB<pawn>(pSquare, pColor) & getPieceBB<pawn>(!pColor)
        || getAttackBB<knight>(pSquare) & getPieceBB<knight>(!pColor)
        || Bmagic(pSquare, occupancy) & (getPieceBB<bishop>(!pColor) | getPieceBB<queen>(!pColor))
        || Rmagic(pSquare, occupancy) & (getPieceBB<rook>(!pColor) | getPieceBB<queen>(!pColor))
        || getAttackBB<king>(pSquare) & getPieceBB<king>(!pColor);
}

inline
bool State::attacked(Square s) const
{
    return getAttackBB< pawn >(s, mUs) &  getPieceBB< pawn >(mThem)
        || getAttackBB<knight>(s) &  getPieceBB<knight>(mThem)
        || getAttackBB<bishop>(s) & (getPieceBB<bishop>(mThem) | getPieceBB<queen>(mThem))
        || getAttackBB< rook >(s) & (getPieceBB< rook >(mThem) | getPieceBB<queen>(mThem));
}

inline
bool State::inCheck() const
{
    return getCheckersBB();
}

inline
bool State::inDoubleCheck() const
{
    return pop_count(getCheckersBB()) == 2;
}

inline
bool State::check() const
{
    return attacked(getKingSquare(mUs));
}

inline
U64 State::getAttackersBB(Square s, Color c) const
{
    return (getAttackBB<pawn>(s, !c) &  getPieceBB< pawn >(c))
         | (getAttackBB<knight>(s)   &  getPieceBB<knight>(c))
         | (getAttackBB<bishop>(s)   & (getPieceBB<bishop>(c) | getPieceBB<queen>(c)))
         | (getAttackBB< rook >(s)   & (getPieceBB< rook >(c) | getPieceBB<queen>(c)));
}

inline
U64 State::allAttackers(Square s) const
{
    return (getAttackBB<pawn>(s, mUs) & getPieceBB<pawn>(mThem))
         | (getAttackBB<pawn>(s, mThem) & getPieceBB<pawn>(mUs))
         | (getAttackBB<knight>(s) & getPieceBB<knight>())
         | (getAttackBB<bishop>(s) & (getPieceBB<bishop>() | getPieceBB<queen>()))
         | (getAttackBB<rook>(s) & (getPieceBB<rook>() | getPieceBB<queen>()))
         | (getAttackBB<king>(s) & getPieceBB<king>());
}

inline
bool State::check(U64 change) const
{
    return (Bmagic(getKingSquare(mUs), getOccupancyBB() ^ change) & (getPieceBB<bishop>(mThem) | getPieceBB<queen>(mThem)))
        || (Rmagic(getKingSquare(mUs), getOccupancyBB() ^ change) & (getPieceBB< rook >(mThem) | getPieceBB<queen>(mThem)));
}

inline
bool State::check(U64 change, Color c) const
{
    return (Bmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB<bishop>(!c) | getPieceBB<queen>(!c)))
        || (Rmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB< rook >(!c) | getPieceBB<queen>(!c)));
}

inline
U64 State::getXRayAttacks(Square square) const
{
    return bishopMoves[square] & (getPieceBB<bishop>() | getPieceBB<queen>())
         | rookMoves[square] & (getPieceBB<rook>() | getPieceBB<queen>());
}

#endif