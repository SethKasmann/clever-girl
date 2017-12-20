#ifndef EVALUATION_H
#define EVALUATION_H

#include <iostream>
#include <iomanip>
#include <array>
#include <algorithm>
#include "state.h"
#include "pst.h"

enum Phase
{
    pawnPhase   = 0,
    knightPhase = 1,
    bishopPhase = 1,
    rookPhase   = 2,
    queenPhase  = 4,
    totalPhase  = 24
};

static const int tempo = 15;

static const int Knight_th = 2;
static const int Bishop_th = 2;
static const int Rook_th   = 3;
static const int Queen_th  = 5;

static const int Checkmate = 32767;
static const int Stalemate = 0;
static const int Draw = 0;

static const int Passed         = 20;
static const int Candidate      = 10;
static const int Connected      = 10;
static const int Isolated       = -10;
static const int Doubled        = -5;
static const int Full_backwards = -15;
static const int Backwards      = -5;
static const int BadBishop = -2;
static const int TrappedRook = -25;

static const int Midgame_limit  = 4500;
static const int Lategame_limit = 2500;

static const size_t hash_size = 8192;

static const int Safety_table[100] = 
{
      0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
     18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
     68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
    140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
    260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
    377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
    494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
    500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

static const int knightMobility[] =
{
    -30, -15, -5, 0, 5, 10, 15, 20, 25
};

static const int bishopMobility[] =
{
    -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

static const int rookMobility[] =
{
    -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50
};

static const int queenMobility[] =
{
    -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};

struct PawnEntry
{
    PawnEntry()
    : mKey(0), mStructure{}
    {}
    U64 getKey() const
    {
        return mKey;
    }
    const std::array<int, Player_size>& getStructure() const
    {
        return mStructure;
    }
    const std::array<int, Player_size>& getMaterial() const
    {
        return mMaterial;
    }
    U64 mKey;
    std::array<int, Player_size> mStructure;
    std::array<int, Player_size> mMaterial;
};

static std::array<PawnEntry, hash_size> pawnHash;

inline void init_eval()
{
    std::fill(pawnHash.begin(), pawnHash.end(), PawnEntry());
}


inline PawnEntry* probe(U64 pKey)
{
    return &pawnHash[pKey % pawnHash.size()];
}

inline void store(U64 pKey, 
                  const std::array<int, Player_size>& pStructure,
                  const std::array<int, Player_size>& pMaterial)
{
    pawnHash[pKey % pawnHash.size()].mKey = pKey;
    pawnHash[pKey % pawnHash.size()].mStructure = pStructure;
    pawnHash[pKey % pawnHash.size()].mMaterial = pMaterial;
}

//int evaluate(const State & s);

class Evaluate
{
public:
    Evaluate(const State& pState)
    : mState(pState)
    , mMaterial{}
    , mPawnStructure{}
    , mMobility{}
    , mKingSafety{}
    {
        if (mState.getPieceCount<pawn>())
        {
            const PawnEntry* pawnEntry = probe(mState.getPawnKey());
            if (pawnEntry && pawnEntry->getKey() == mState.getPawnKey())
            {
                mPawnStructure = pawnEntry->getStructure();
                mMaterial = pawnEntry->getMaterial();
            }
            else
            {
                evalPawns(white);
                evalPawns(black);
                store(mState.getPawnKey(), mPawnStructure, mMaterial);
            }
        }

        float phase = totalPhase
                    - mState.getPieceCount<pawn>()   * pawnPhase
                    - mState.getPieceCount<knight>() * knightPhase
                    - mState.getPieceCount<bishop>() * bishopPhase
                    - mState.getPieceCount<rook>()   * rookPhase
                    - mState.getPieceCount<queen>()  * queenPhase;

        mGamePhase = (phase * 256 + (totalPhase / 2)) / totalPhase;

        evalPieces(white);
        evalPieces(black);

        Color c = mState.getOurColor();
        mScore = mMobility[c]      - mMobility[!c]
              + mKingSafety[c]    - mKingSafety[!c]
              + mPawnStructure[c] - mPawnStructure[!c]
              + mMaterial[c]      - mMaterial[!c];

        mScore += ((mState.getPstScore(middle) * (256 - mGamePhase))
               + mState.getPstScore(late) * mGamePhase) / 256;

        mScore += tempo;
    }
    // Returns the score of a bishop or rook on an outpost square.
    template<PieceType PT>
    int outpost(Square p, Color c)
    {
        int score;
        // To be an outpost, the piece must be supported by a friendly pawn
        // and unable to be attacked by an opponents pawn.
        if (   !(p & outpost_area[c])
            || !(pawn_attacks[!c][p] & mState.getPieceBB<pawn>(c))
            || in_front[c][p] & adj_files[p] & mState.getPieceBB<pawn>(!c))
            return 0;

        score = PieceSquareTable::outpost[PT == bishop][c][p];

        // Extra bonus if the outpost cannot be captured by a minor piece.
        if (   !mState.getPieceBB<knight>(!c)
            && !(mState.getPieceBB<bishop>(!c) & squares_of_color(p)))
            score *= 2;

        return score;
    }
    void evalPawns(const Color c)
    {
        const int dir = c == white ? 8 : -8;

        // Pawn evaluation.
        for (Square p : mState.getPieceList<pawn>(c))
        {
            if (p == no_sq)
                break;
            mMaterial[c] += Pawn_wt;
            // Check if the pawn is a passed pawn.
            if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & mState.getPieceBB<pawn>(!c)))
                mPawnStructure[c] += Passed;
            // Look for candidate and backwards pawns since the logic is related.
            // First check for a half open file.
            else if (!(file_bb[p] & in_front[c][p] & mState.getPieceBB<pawn>(!c)))
            {
                int sentries, helpers;
                assert(p < 56 && p > 7);
                // Check, if this pawn were to push, if there are any enemy pawns
                // defending that square.
                sentries = pop_count(pawn_attacks[c][p + dir] & mState.getPieceBB<pawn>(!c));
                if (sentries > 0)
                {
                    // If the enemy has defenders, check if we helper pawns defending
                    // that square.
                    helpers = pop_count(pawn_attacks[!c][p + dir] & mState.getPieceBB<pawn>(c));
                    // If there are more helpers then sentries, we have a candidate
                    // passer.
                    if (helpers >= sentries)
                        mPawnStructure[c] += Candidate;
                    // If there are less helpers then sentries, check for a backwards
                    // pawn. First, see if there are no pawns eligible to defend it.
                    else if (!(~in_front[c][p] & adj_files[p] & mState.getPieceBB<pawn>(c)))
                    {
                        // Make sure the backwards pawn is defending at least 1 pawn.
                        if (pawn_attacks[c][p] & mState.getPieceBB<pawn>(c))
                        {
                            // If there are two sentries, this backwards pawn is
                            // nearly imposible to push.
                            if (sentries == 2)
                                mPawnStructure[c] += Full_backwards;
                            else
                                mPawnStructure[c] += Backwards;
                        }
                    }
                }
            }

            // Check if the pawn is isolated
            if (!(adj_files[p] & mState.getPieceBB<pawn>(c)))
                mPawnStructure[c] += Isolated;
            // Check if the pawn is connected
            else if (rank_bb[p - dir] & mState.getPieceBB<pawn>(c) & adj_files[p])
                mPawnStructure[c] += Connected;

            // Check if the pawn is doubled.
            if (pop_count(file_bb[p] & mState.getPieceBB<pawn>(c)) > 1)
                mPawnStructure[c] += Doubled;
        }
    }
    void evalPieces(const Color c)
    {
        U64 moves, pins;
        U64 mobilityNet;
        int king_threats = 0;
        U64 bottomRank = c == white ? Rank_1 : Rank_8;
        Square kingSq = mState.getKingSquare(c);

        // The mobilityNet is all empty squares not attacked by enemy pawns.
        mobilityNet = mState.getEmptyBB();
        if (c == white)
            mobilityNet &= ~((mState.getPieceBB<pawn>(black) & Not_a_file) >> 7
                           | (mState.getPieceBB<pawn>(black) & Not_h_file) >> 9);
        else
            mobilityNet &= ~((mState.getPieceBB<pawn>(white) & Not_a_file) << 9
                           | (mState.getPieceBB<pawn>(white) & Not_h_file) << 7);

        // Get the pinned pieces for the current player.
        pins = mState.getPinsBB(c);

        // Knight evaluation.
        for (Square p : mState.getPieceList<knight>(c))
        {
            if (p == no_sq)
                break;
            mMaterial[c] += Knight_wt;
            if (mState.getAttackBB<knight>(p) & king_net_bb[!c][mState.getKingSquare(!c)])
                king_threats += Knight_th;
            mMaterial[c] += outpost<knight>(p, c);

            // Calculate knight mobility.
            if (square_bb[p] & pins)
                mMobility[c] += knightMobility[0];
            else
            {
                moves = mState.getAttackBB<knight>(p) & mobilityNet;
                mMobility[c] += knightMobility[pop_count(moves)];
            }
        }

        // Bishop evaluation.
        for (Square p : mState.getPieceList<bishop>(c))
        {
            if (p == no_sq)
                break;
            mMaterial[c] += Bishop_wt;
            mMaterial[c] += outpost<bishop>(p, c);

            moves = mState.getAttackBB<bishop>(p) & mobilityNet;
            if (square_bb[p] & pins)
                moves &= between_dia[p][mState.getKingSquare(!c)];

            if (moves & king_net_bb[!c][mState.getKingSquare(!c)])
                king_threats += Bishop_th;

            mMobility[c] += bishopMobility[pop_count(moves)];

            // Check for a bad bishop (penalty for pawns on the same square);
            mMobility[c] += BadBishop * 
                pop_count(squares_of_color(p) & mState.getPieceBB<pawn>(c));
        }

        // Rook evaluation.
        for (Square p : mState.getPieceList<rook>(c))
        {
            if (p == no_sq)
                break;
            mMaterial[c] += Rook_wt;
            moves = mState.getAttackBB<rook>(p) & mobilityNet;
            if (square_bb[p] & pins)
                moves &= between_hor[p][kingSq];

            if (moves & king_net_bb[!c][mState.getKingSquare(!c)])
                king_threats += Rook_th;

            mMobility[c] += rookMobility[pop_count(moves)];

            // Check if the rook is trapped.
            // TODO: more testing to confirm this is working propertly.
            if (mState.getPieceBB<king>(c) & bottomRank 
                && square_bb[p] & bottomRank)
            {
                if ((kingSq > p && !mState.canCastleKingside(c) &&
                    square_bb[kingSq] & Rightside) ||
                    (kingSq < p && !mState.canCastleQueenside(c) &&
                    square_bb[kingSq] & Leftside)) 
                {
                    if (pop_count(moves & ~(bottomRank)) <= 3)
                        mMaterial[c] += TrappedRook;
                }
            }
        }

        // Queen evaluation.
        for (Square p : mState.getPieceList<queen>(c))
        {
            if (p == no_sq)
                break;
            mMaterial[c] += Queen_wt;
            moves = mState.getAttackBB<queen>(p) & mobilityNet;
            if (square_bb[p] & pins)
                moves &= between[p][kingSq];

            if (moves & king_net_bb[!c][mState.getKingSquare(!c)])
                king_threats += Queen_th;

            // Calculate queen mobility.
            mMobility[c] += queenMobility[pop_count(moves)];
        }

        // King evaluation.
        mKingSafety[!c] -= Safety_table[king_threats];
    }
    int getScore() const
    {
        return mScore;
    }
    friend std::ostream& operator<<(std::ostream& o, const Evaluate& e)
    {
        Color c = e.mState.getOurColor();
        std::string us = c == white ? "White" : "Black";
        std::string them = c == white ? "Black" : "White";
        int pstMid = e.mState.getPstScore(middle) * (256 - e.mGamePhase) / 256;
        int pstLate = e.mState.getPstScore(late) * e.mGamePhase / 256;

        o << e.mState
          << "-------------------------------------------------------------\n"
          << "| Evaluation Type |    " << us << "    |    " << them 
          << "    | Total       |\n"
          << "-------------------------------------------------------------\n"
          << "| Material        |"
          << std::setw(13) << e.mMaterial[ c] << "|"
          << std::setw(13) << e.mMaterial[!c] << "|"
          << std::setw(13) << e.mMaterial[ c] - e.mMaterial[!c] << "|\n"
          << "-------------------------------------------------------------\n"
          << "| Mobility        |" 
          << std::setw(13) << e.mMobility[ c] << "|" 
          << std::setw(13) << e.mMobility[!c] << "|"
          << std::setw(13) << e.mMobility[ c] - e.mMobility[!c] << "|\n"
          << "-------------------------------------------------------------\n"
          << "| Pawn Structure  |"
          << std::setw(13) << e.mPawnStructure[ c] << "|" 
          << std::setw(13) << e.mPawnStructure[!c] << "|"
          << std::setw(13) << e.mPawnStructure[ c] - e.mPawnStructure[!c] 
          << "|\n"
          << "-------------------------------------------------------------\n"
          << "| King Safety     |"
          << std::setw(13) << e.mKingSafety[ c] << "|" 
          << std::setw(13) << e.mKingSafety[!c] << "|"
          << std::setw(13) << e.mKingSafety[ c] - e.mKingSafety[!c] << "|\n"
          << "-------------------------------------------------------------\n"
          << "| PST (mid/late)  |"
          << std::setw(13) << pstMid << "|" 
          << std::setw(13) << pstLate << "|"
          << std::setw(13) << pstMid + pstLate << "|\n"
          << "-------------------------------------------------------------\n"
          << "| Total           |             |             |"
          << std::setw(13) << e.mScore << "|\n"
          << "-------------------------------------------------------------\n";


          return o;
    }
private:
    float mGamePhase;
    const State& mState;
    int mScore;
    std::array<int, Player_size> mMobility;
    std::array<int, Player_size> mKingSafety;
    std::array<int, Player_size> mPawnStructure;
    std::array<int, Player_size> mMaterial;
};

#endif