#include "evaluation.h"

int eval(const State & s, const Color c)
{
    const Square * p;
    const int dir = c == white ? 8 : -8;
    int score = Draw;

    bool eg = false; // determine middle or endgame.

    // Pawn evaluation.
    bool isolated, passed, doubled, connected;
    for (p = s.piece_list[c][pawn]; *p != no_sq; ++p)
    {
    	score += Pst_pawn[c][*p];
    	isolated = adj_files[*p] & s.piece_bb(c, pawn);
        passed   = adj_files[*p] & in_front[c][*p] & s.piece_bb(!c, pawn);
        doubled  = pop_count(file_bb[*p] & s.piece_bb(c, pawn)) > 1;
        connected = file_bb[*p - dir] & s.piece_bb(c, pawn);
    	if (isolated)  score += Isolated;
        if (passed)    score += Passed;
        if (doubled)   score += Doubled;
        if (connected) score += Connected;
    }
    score += s.piece_count[c][pawn] * Pawn_wt;

    // Knight evaluation.
    for (p = s.piece_list[c][knight]; *p != no_sq; ++p)
    {
    	score += Pst_knight[c][*p];
    }
    score += s.piece_count[c][knight] * Knight_wt;


    // Bishop evaluation.
    for (p = s.piece_list[c][bishop]; *p != no_sq; ++p)
    {
    	score += Pst_bishop[c][*p];
    }
    score += s.piece_count[c][bishop] * Bishop_wt;

    // Rook evaluation.
    for (p = s.piece_list[c][rook]; *p != no_sq; ++p)
    {
    	score += Pst_rook[c][*p];
    }
    score += s.piece_count[c][rook] * Rook_wt;

    // Queen evaluation.
    for (p = s.piece_list[c][queen]; *p != no_sq; ++p)
    {
    	score += Pst_queen[c][*p];
    }
    score += s.piece_count[c][queen] * Queen_wt;

    // King evaluation.
    p = s.piece_list[c][king];
    score += Pst_king[eg][c][*p];

    return score;
}

int evaluate(const State & s)
{
	return eval(s, s.us) - eval(s, s.them);
}