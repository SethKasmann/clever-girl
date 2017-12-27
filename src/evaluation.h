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
static const int GuaranteedWin = Checkmate - Max_ply;
static const int Stalemate = 0;
static const int Draw = 0;

static const int Passed         = 20;
static const int Candidate      = 10;
static const int Connected      = 10;
static const int Isolated       = -10;
static const int Doubled        = -5;
static const int Full_backwards = -15;
static const int Backwards      = -5;
static const int BadBishop      = -2;
static const int TrappedRook    = -25;
static const int StrongPawnAttack = -80;
static const int WeakPawnAttack = -40;
static const int Hanging        = -25;

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
    Evaluate(const State& pState);
    // Returns the score of a bishop or rook on an outpost square.
    template<PieceType PT>
    int outpost(Square p, Color c);
    void evalPawns(const Color c);
    void evalPieces(const Color c);
    void evalAttacks(Color c);
    int getScore() const;
    friend std::ostream& operator<<(std::ostream& o, const Evaluate& e);
private:
    float mGamePhase;
    const State& mState;
    int mScore;
    std::array<int, Player_size> mMobility;
    std::array<int, Player_size> mKingSafety;
    std::array<int, Player_size> mPawnStructure;
    std::array<int, Player_size> mMaterial;
    std::array<int, Player_size> mAttacks;
    std::array<std::array<U64, Types_size>, Player_size> mPieceAttacksBB;
    std::array<U64, Player_size> mAllAttacksBB;
};

#endif