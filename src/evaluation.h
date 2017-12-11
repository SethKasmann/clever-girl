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
	-20, -10, -5, 0, 5, 10, 20, 35, 50
};

static const int bishopMobility[] =
{
	-20, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

static const int rookMobility[] =
{
	-20, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50
};

static const int queenMobility[] =
{
	-20, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 
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
		/*
		PawnEntry* pawnEntry;
		if (s.getPieceCount<pawn>(s.getOurColor()) + s.getPieceCount<pawn>(s.getTheirColor()))
		{
			pawnEntry = probe(mState.getPawnKey());
			if (pawnEntry && pawnEntry->mKey == mState.getPawnKey())

		}
		pawnStructure(mState.getOurColor());
		pawnStructure(mState.getTheirColor());
		material(mState.getOurColor());
		material(mState.getTheirColor());

		int finalScore;
    int pawnScore = 0;
    PawnEntry* pawnEntry;

    scaledPstScore(s);

    // Pawn Evaluation
    if (s.getPieceCount<pawn>(s.getOurColor()) + s.getPieceCount<pawn>(s.getTheirColor()))
    {
        //Probe the pawn hash.
        pawnEntry = probe(s.getPawnKey());
        if (pawnEntry && pawnEntry->mKey == s.getPawnKey())
        {
            pawnScore = (pawnEntry->mColor == s.getOurColor()) ?  pawnEntry->mScore 
                                                    : -pawnEntry->mScore;
        }
        else
        {
            pawnScore = eval_pawns(s, s.getOurColor()) - eval_pawns(s, s.getTheirColor());
            store(s.getPawnKey(), pawnScore, s.getOurColor());
        }
    }
    finalScore += pawnScore;

    finalScore += eval(s, s.getOurColor()) - eval(s, s.getTheirColor()) + tempo;

	return finalScore;*/
	}
	float getGamePhase()
	{
	    float phase;

	    phase = totalPhase 
	          - mState.getPieceCount<pawn>()   * pawnPhase
	          - mState.getPieceCount<knight>() * knightPhase
	          - mState.getPieceCount<bishop>() * bishopPhase
	          - mState.getPieceCount<rook>()   * rookPhase
	          - mState.getPieceCount<queen>()  * queenPhase;

	    return (phase * 256 + (totalPhase / 2)) / totalPhase;
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
	void pawnStructure(const Color c)
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
	void material(const Color c)
	{
	    U64 moves, pins;
	    U64 mobilityNet;
	    int king_threats = 0;
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
	        if (mState.getAttackBB<bishop>(p) & king_net_bb[!c][mState.getKingSquare(!c)])
	            king_threats += Bishop_th;
	        mMaterial[c] += outpost<bishop>(p, c);

	        // Calculate bishop mobility
	        moves = mState.getAttackBB<bishop>(p) & mobilityNet;
	        if (square_bb[p] & pins)
	            moves &= between_dia[p][kingSq];
	        mMobility[c] += bishopMobility[pop_count(moves)];
	    }

	    // Rook evaluation.
	    for (Square p : mState.getPieceList<rook>(c))
	    {
	        if (p == no_sq)
	            break;
	        mMaterial[c] += Rook_wt;
	        if (mState.getAttackBB<rook>(p) & king_net_bb[!c][mState.getKingSquare(!c)])
	            king_threats += Rook_th;

	        // Calculate rook mobility.
	        moves = mState.getAttackBB<rook>(p) & mobilityNet;
	        if (square_bb[p] & pins)
	            moves &= between_hor[p][kingSq];
	        mMobility[c] += rookMobility[pop_count(moves)];
	    }

	    // Queen evaluation.
	    for (Square p : mState.getPieceList<queen>(c))
	    {
	        if (p == no_sq)
	            break;
	        mMaterial[c] += Queen_wt;
	        if (mState.getAttackBB<queen>(p) & king_net_bb[!c][mState.getKingSquare(!c)])
	            king_threats += Queen_th;

	        // Calculate queen mobility.
	        moves = mState.getAttackBB<queen>(p) & mobilityNet;
	        if (square_bb[p] & pins)
	        {
	            moves &= between_hor[p][kingSq] ? between_hor[p][kingSq]
	                                            : between_dia[p][kingSq];
	        }
	        mMobility[c] += queenMobility[pop_count(moves)];
	    }

	    // King evaluation.
	    mKingSafety[!c] -= Safety_table[king_threats];
	}
private:
	const State& mState;
	std::array<int, Player_size> mMaterial;
	std::array<int, Player_size> mMobility;
	std::array<int, Player_size> mKingSafety;
	std::array<int, Player_size> mPawnStructure;
};

#endif