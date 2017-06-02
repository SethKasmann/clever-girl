#include "evaluation.h"

int eval(const State & s, const Color c)
{
    const Square * p;
    int score = DRAW;

    bool eg = false; // determine middle or endgame.

    // Pawn evaluation.
    bool isolated;
    for (p = s.piece_list[c][PAWN]; *p != NO_SQ; ++p)
    {
    	score += pst_pawn[c][*p];
    	//isolated = adj_files[*p]
    }
    score += s.piece_count[c][PAWN] * PAWN_WT;

    // Knight evaluation.
    for (p = s.piece_list[c][KNIGHT]; *p != NO_SQ; ++p)
    {
    	score += pst_knight[c][*p];
    }
    score += s.piece_count[c][KNIGHT] * KNIGHT_WT;


    // Bishop evaluation.
    for (p = s.piece_list[c][BISHOP]; *p != NO_SQ; ++p)
    {
    	score += pst_bishop[c][*p];
    }
    score += s.piece_count[c][BISHOP] * BISHOP_WT;

    // Rook evaluation.
    for (p = s.piece_list[c][ROOK]; *p != NO_SQ; ++p)
    {
    	score += pst_pawn[c][*p];
    }
    score += s.piece_count[c][ROOK] * ROOK_WT;

    // Queen evaluation.
    for (p = s.piece_list[c][QUEEN]; *p != NO_SQ; ++p)
    {
    	score += pst_pawn[c][*p];
    }
    score += s.piece_count[c][QUEEN] * QUEEN_WT;

    // King evaluation.
    p = s.piece_list[c][KING];
    score += pst_king[eg][c][*p];

    return score;
}

int evaluate(const State & s)
{
	return eval(s, s.us) - eval(s, s.them);
}