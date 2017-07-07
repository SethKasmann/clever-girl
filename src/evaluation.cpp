#include "evaluation.h"

// Returns the score of a bishop or rook on an outpost square.
template<PieceType PT>
int outpost(const State & s, Square p, Color c)
{
    int score;
    // To be an outpost, the piece must be supported by a friendly pawn
    // and unable to be attacked by an opponents pawn.
    if (   !(p & outpost_area[c])
        || !(pawn_attacks[!c][p] & s.piece_bb<pawn>(c))
        || in_front[c][p] & adj_files[p] & s.piece_bb<pawn>(!c))
        return 0;

    score = Pst_outpost[PT == bishop][c][p];

    // Extra bonus if the outpost cannot be captured by a minor piece.
    if (   !s.piece_bb<knight>(!c)
        && !(s.piece_bb<bishop>(!c) & squares_of_color(p)))
        score *= 2;

    return score;
}

// Check to see if a pawn push would fork two of the enemy major or minor
// pieces. Also includes forks involving the enemy's king.
bool pawn_fork(const State & s, Square p, Color c)
{
    assert(p < 56 && p > 7);
    Square push;
    
    push = p + (c == white ? 8 : -8);
    
    // Check to see if the push square is not occupied, if the pawn is not on
    // the edge of the board, and if the push would fork two enemy non-pawn 
    // pieces.
    if (   push & s.occ()
        || !(push & Center_files)
        || pawn_attacks[c][push] & (s.piece_bb<pawn>(!c) | s.empty() | s.occ(c)))
        return false;

    // Check to see if the pawn is pinned.
    if (s.check(square_bb[p], c))
        return false;

    // Return true if the push square is defended.
    return s.defended(push, c);
}

int eval(const State & s, const Color c)
{
    const Square * p;
    const int dir = c == white ? 8 : -8;
    int score = Draw;
    int king_threats = 0;

    bool gs = false; // determing game stage, middle or late

    // Pawn evaluation.
    bool isolated, passed, doubled, connected, fork;
    for (p = s.piece_list[c][pawn]; *p != no_sq; ++p)
    {
    	score += Pst_pawn[gs][c][*p];
    	isolated = adj_files[*p] & s.piece_bb<pawn>(c);
        passed   = adj_files[*p] & in_front[c][*p] & s.piece_bb<pawn>(!c);
        doubled  = pop_count(file_bb[*p] & s.piece_bb<pawn>(c)) > 1;
        connected = rank_bb[*p - dir] & s.piece_bb<pawn>(c) & adj_files[*p];
        //fork = pawn_fork(s, *p, c);
    	if (isolated)  
            score += Isolated;
        if (doubled)   
            score += Doubled;
        if (connected) 
            score += Connected;
        if (passed)    
            score += Passed;
        /*
        if (fork)
            score += Fork;*/
    }
    score += s.piece_count[c][pawn] * Pawn_wt;

    // Knight evaluation.
    for (p = s.piece<knight>(c); *p != no_sq; ++p)
    {
    	score += Pst_knight[gs][c][*p];
        if (s.attack_bb<knight>(*p) & king_net_bb[!c][s.king_sq(!c)])
            king_threats += Knight_th;
        score += outpost<knight>(s, *p, c);
    }
    score += s.piece_count[c][knight] * Knight_wt;

    // Bishop evaluation.
    for (p = s.piece<bishop>(c); *p != no_sq; ++p)
    {
    	score += Pst_bishop[gs][c][*p];
        if (s.attack_bb<bishop>(*p) & king_net_bb[!c][s.king_sq(!c)])
            king_threats += Bishop_th;
        score += outpost<bishop>(s, *p, c);
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