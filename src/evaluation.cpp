#include "evaluation.h"

int eval(const State & s, const Color c)
{
    const Square * p;
    const int dir = c == WHITE ? 8 : -8;
    int score = Draw;

    bool eg = false; // determine middle or endgame.

    // Pawn evaluation.
    bool isolated, passed, doubled, connected;
    for (p = s.piece_list[c][PAWN]; *p != NO_SQ; ++p)
    {
    	score += PstPawn[c][*p];
    	isolated = adj_files[*p] & s.piece_bb(c, PAWN);
        passed   = adj_files[*p] & in_front[c][*p] & s.piece_bb(!c, PAWN);
        doubled  = pop_count(file_bb[*p] & s.piece_bb(c, PAWN)) > 1;
        connected = file_bb[*p - dir] & s.piece_bb(c, PAWN);
    	if (isolated)  score += Isolated;
        if (passed)    score += Passed;
        if (doubled)   score += Doubled;
        if (connected) score += Connected;
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