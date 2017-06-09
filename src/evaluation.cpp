#include "evaluation.h"

int eval(const State & s, const Color c)
{
    const Square * p;
    const int dir = c == white ? 8 : -8;
    int score = Draw;
    int king_threats = 0;

    bool gs = false; // determing game stage, middle or late

    // Pawn evaluation.
    bool isolated, passed, doubled, connected;
    for (p = s.piece_list[c][pawn]; *p != no_sq; ++p)
    {
    	score += Pst_pawn[gs][c][*p];
    	isolated = adj_files[*p] & s.piece_bb<pawn>(c);
        passed   = adj_files[*p] & in_front[c][*p] & s.piece_bb<pawn>(!c);
        doubled  = pop_count(file_bb[*p] & s.piece_bb<pawn>(c)) > 1;
        connected = rank_bb[*p - dir] & s.piece_bb<pawn>(c) & adj_files[*p];
    	if (isolated)  
            score += Isolated;
        if (doubled)   
            score += Doubled;
        if (connected) 
            score += Connected;
        if (passed)    
            score += Passed;
    }
    score += s.piece_count[c][pawn] * Pawn_wt;

    // Knight evaluation.
    for (p = s.piece<knight>(c); *p != no_sq; ++p)
    {
    	score += Pst_knight[gs][c][*p];
        if (s.attack_bb<knight>(*p) & king_net_bb[!c][s.king_sq(!c)])
            king_threats += Knight_th;
    }
    score += s.piece_count[c][knight] * Knight_wt;

    // Bishop evaluation.
    for (p = s.piece<bishop>(c); *p != no_sq; ++p)
    {
    	score += Pst_bishop[gs][c][*p];
        if (s.attack_bb<bishop>(*p) & king_net_bb[!c][s.king_sq(!c)])
            king_threats += Bishop_th;
    }
    score += s.piece_count[c][bishop] * Bishop_wt;

    // Rook evaluation.
    for (p = s.piece<rook>(c); *p != no_sq; ++p)
    {
    	score += Pst_rook[gs][c][*p];
        if (s.attack_bb<rook>(*p) & king_net_bb[!c][s.king_sq(!c)])
            king_threats += Rook_th;
    }
    score += s.piece_count[c][rook] * Rook_wt;

    // Queen evaluation.
    for (p = s.piece<queen>(c); *p != no_sq; ++p)
    {
    	score += Pst_queen[gs][c][*p];
        if (s.attack_bb<queen>(*p) & king_net_bb[!c][s.king_sq(!c)])
            king_threats += Queen_th;
    }
    score += s.piece_count[c][queen] * Queen_wt;

    // King evaluation.
    score += Pst_king[gs][c][s.king_sq(c)];
    score += Safety_table[king_threats];

    return score;
}

int evaluate(const State & s)
{
	return eval(s, s.us) - eval(s, s.them);
}