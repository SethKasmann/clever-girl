#include "evaluation.h"

int eval(const State & s, const Color c)
{
    const Square * p;
    int score = Draw;

    bool eg = false; // determine middle or endgame.

    // Pawn evaluation.
    bool isolated;
    for (p = s.piece_list[c][PAWN]; *p != NO_SQ; ++p)
    {
    	score += PstPawn[c][*p];
    	isolated = adj_files[*p] & s.piece_bb(c, PAWN);
    	if (isolated) score += Isolated;
    }
    score += s.piece_count[c][PAWN] * PawnWt;

    // Knight evaluation.
    for (p = s.piece_list[c][KNIGHT]; *p != NO_SQ; ++p)
    {
    	score += PstKnight[c][*p];
    }
    score += s.piece_count[c][KNIGHT] * KnightWt;


    // Bishop evaluation.
    for (p = s.piece_list[c][BISHOP]; *p != NO_SQ; ++p)
    {
    	score += PstBishop[c][*p];
    }
    score += s.piece_count[c][BISHOP] * BishopWt;

    // Rook evaluation.
    for (p = s.piece_list[c][ROOK]; *p != NO_SQ; ++p)
    {
    	score += PstRook[c][*p];
    }
    score += s.piece_count[c][ROOK] * RookWt;

    // Queen evaluation.
    for (p = s.piece_list[c][QUEEN]; *p != NO_SQ; ++p)
    {
    	score += PstQueen[c][*p];
    }
    score += s.piece_count[c][QUEEN] * QueenWt;

    // King evaluation.
    p = s.piece_list[c][KING];
    score += PstKing[eg][c][*p];

    return score;
}

int evaluate(const State & s)
{
	return eval(s, s.us) - eval(s, s.them);
}