#ifndef EVALUATION_H
#define EVALUATION_H

#include <iostream>
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
static const int Candidate      = 15;
static const int Connected      = 15;
static const int Isolated       = -15;
static const int Doubled        = -10;
static const int Fork           = 30;
static const int Full_backwards = -30;
static const int Backwards      = -10;

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
	-75, -50, -5, 0, 5, 10, 20, 35, 50
};

static const int bishopMobility[] =
{
	-75, -50, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

static const int rookMobility[] =
{
	-75, -50, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50
};

static const int queenMobility[] =
{
	-75, -50, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};

struct PawnEntry
{
	PawnEntry()
	: mKey(0), mScore(0), mColor(white)
	{}
	PawnEntry(U64 pKey, int pScore, Color pColor)
	: mKey(pKey), mScore(pScore), mColor(pColor)
	{}
	U64 mKey;
	int mScore;
	Color mColor;
};

extern std::array<PawnEntry, hash_size> pawnHash;

inline PawnEntry* probe(U64 pKey)
{
	return &pawnHash[pKey % pawnHash.size()];
}

inline void store(U64 pKey, int pScore, Color pColor)
{
	pawnHash[pKey % pawnHash.size()] = PawnEntry(pKey, pScore, pColor);
}

void init_eval();
int evaluate(const State & s);

#endif