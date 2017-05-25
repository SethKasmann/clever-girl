#include "move_generator.h"

// ----------------------------------------------------------------------------
// Initialize magics.
// ----------------------------------------------------------------------------

void mg_init()
{
    initmagicmoves();
}

// ----------------------------------------------------------------------------
// Push all legal pawn moves and pawn attacks, including promotions,
// double pawn pushes, and en-passant.
// ----------------------------------------------------------------------------

void push_pawn_moves(State & s, MoveList * mlist, U64 checker=FULL, U64 ray=FULL)
{
    const Color C = s.us == WHITE ? WHITE  : BLACK;
    const Dir   P     = C == WHITE ? N      : S;
    const Dir   L     = C == WHITE ? NW     : SW;
    const Dir   R     = C == WHITE ? NE     : SE;
    const U64   DBL   = C == WHITE ? Rank_2 : Rank_7;
    const U64   PROMO = C == WHITE ? Rank_8 : Rank_1;

    U64 attack, push, dbl, promo, no_promo, ep;
    Square dst;

    const U64 pawns = s.p_pawn();

    // Pawn attacks and promotion attacks to the east.
    attack   = shift_e(pawns, R) & s.e_occ() & checker;
    promo    = attack & PROMO;
    no_promo = attack ^ promo;

    while (no_promo)
    {
        dst = pop_lsb(no_promo);
        mlist->push(dst - R, dst, ATTACK, SCORE[PAWN][s.on_square(dst, !C)]);
    }

    while (promo)
    {
        dst = pop_lsb(promo);
        mlist->push(dst - R, dst, QUEEN_PROMO,  QP);
        mlist->push(dst - R, dst, KNIGHT_PROMO, NP);
        mlist->push(dst - R, dst, ROOK_PROMO,   RP);
        mlist->push(dst - R, dst, BISHOP_PROMO, BP);
    }

    // Pawn attacks and promotion attacks to the west.
    attack   = shift_w(pawns, L) & s.e_occ() & checker;
    promo    = attack & PROMO;
    no_promo = attack ^ promo;

    while (no_promo)
    {
        dst = pop_lsb(no_promo);
        mlist->push(dst - L, dst, ATTACK, SCORE[PAWN][s.on_square(dst, !C)]);
    }

    while (promo)
    {
        dst = pop_lsb(promo);
        mlist->push(dst - L, dst, QUEEN_PROMO,  QP);
        mlist->push(dst - L, dst, KNIGHT_PROMO, NP);
        mlist->push(dst - L, dst, ROOK_PROMO,   RP);
        mlist->push(dst - L, dst, BISHOP_PROMO, BP);
    }

    // Pawn pushes and push promotions.
    push     = shift(pawns, P) & s.empty() & ray;
    promo    = push & PROMO;
    no_promo = push ^ promo;

    while (no_promo)
    {
        dst = pop_lsb(no_promo);
        mlist->push(dst - P, dst, QUIET, Q);
    }

    while (promo)
    {
        dst = pop_lsb(promo);
        mlist->push(dst - P, dst, QUEEN_PROMO,  QP);
        mlist->push(dst - P, dst, KNIGHT_PROMO, NP);
        mlist->push(dst - P, dst, ROOK_PROMO,   RP);
        mlist->push(dst - P, dst, BISHOP_PROMO, BP);
    }

    dbl = shift(shift(pawns & DBL, P) & s.empty(), P) & s.empty() & ray;

    while(dbl)
    {
        dst = pop_lsb(dbl);
        mlist->push(dst - P - P, dst, DBL_PUSH, Q);
    }

    // En passant.
    ep = shift(s.en_passant & checker, P) & s.empty();
    if (ep)
    {
        dst = get_lsb(ep);
        attack = pawn_attacks[!C][dst] & s.p_pawn();
        while (attack)
        {
            if (s.is_attacked_by_slider(s.en_passant | get_lsb_bb(attack)))
            {
                return;
            }
            mlist->push(pop_lsb(attack), dst, EN_PASSANT, EP);
        }
    }
}

// ----------------------------------------------------------------------------
// Push all pseudo-legal moves and attacks for Knights, Bishops, Rooks, and 
// Queens.
// ----------------------------------------------------------------------------

template <PieceType P>
void push_moves(State & s, MoveList * mlist, U64 checker=FULL, U64 ray=FULL)
{
    U64 m, a;
    Square dst;
    int i, score;
    for (i = 0; i < s.piece_count[s.us][P]; ++i)
    {
        m   = P == KNIGHT ? knight_moves[s.piece_list[s.us][KNIGHT][i]] & (ray | checker)
            : P == BISHOP ? Bmagic(s.piece_list[s.us][P][i], s.occ())  & (ray | checker)
            : P == ROOK   ? Rmagic(s.piece_list[s.us][P][i], s.occ())  & (ray | checker)
                          : Qmagic(s.piece_list[s.us][P][i], s.occ())  & (ray | checker);

        a   = m & s.e_occ();
        m  &= s.empty();
        while (a)
        {
            dst   = pop_lsb(a);
            score = SCORE[P][s.on_square(dst, s.them)];
            mlist -> push(s.piece_list[s.us][P][i], dst, ATTACK, score);
        }
        while (m)
        {
            mlist -> push(s.piece_list[s.us][P][i], pop_lsb(m), QUIET, Q);
        }
    }
}

// ----------------------------------------------------------------------------
// Push all legal King moves and attacks. This also handles castling moves.
// ----------------------------------------------------------------------------

void push_king_moves(State & s, MoveList * mlist, int checks=0)
{
    U64 m, a;
    int score;
    Square k, dst;

    k = s.p_king_sq();

    m = s.valid_king_moves();
    a = m & s.e_occ();
    m ^= a;

    while (m) 
        mlist -> push(k, pop_lsb(m), QUIET, Q);

    if (checks == 2) return;

    while (a)
    {
        score = SCORE[KING][s.on_square(dst, s.them)];
        mlist -> push(k, pop_lsb(a), ATTACK, score);
    }

    if (checks) return;

    if (s.k_castle() 
        && !(between_hor[k][k-3] & s.occ())
        && !s.is_attacked(k-1) 
        && !s.is_attacked(k-2))
        mlist->push(k, k-2, KING_CAST, C);

    if (s.q_castle()
        && !(between_hor[k][k+4] & s.occ())
        && !s.is_attacked(k+1)
        && !s.is_attacked(k+2))
        mlist->push(k, k+2, QUEEN_CAST, C);
}

// ----------------------------------------------------------------------------
// Check if moves are legal by removing any moves from the moves list
// where a pinned piece moves off it's pin ray.
// ----------------------------------------------------------------------------

void check_legal(State & s, MoveList * mlist)
{
    U64 pin;

    pin = s.get_pins(); // Bitboard of pinned pieces.

    if (!pin) return;
    
    // Run through the move list. Remove any moves where the source location
    // is a pinned piece and the king square is not on the line formed by the
    // source and destination locations.
    for (Move m = *mlist->c; mlist->c < mlist->e; m = *mlist->c)
    {
        if (get_src(m) & pin 
         && !(coplanar[get_src(m)][get_dst(m)] & s.p_king()))
            *mlist->c = *(--mlist->e);
        else
            mlist->c++;
    }
}

// ----------------------------------------------------------------------------
// Push legal moves onto the moves list. The generation technique is to first 
// find the number of checks on our king.
// If checks == 0, add all pseudo-legal moves.
// If checks == 1, add pseudo-legal moves that block or capture the checking
//     piece.
// If checks == 0, add only legal king moves.
// Lastly, verify all moves made by pinned pieces stay on their pin ray.
// ----------------------------------------------------------------------------

void push_moves(State & s, MoveList * mlist)
{
    U64 ray, checker;
    int checks;

    checker = s.checkers();       // Bitboard of check locations.
    checks  = pop_count(checker); // Number of checkers.

    // No checks, push all moves.
    if (checks == 0)
    {
        push_pawn_moves(s, mlist);
        push_moves<KNIGHT>(s, mlist);
        push_moves<BISHOP>(s, mlist);
        push_moves<ROOK  >(s, mlist);
        push_moves<QUEEN >(s, mlist);
        push_king_moves(s, mlist);
    }
    // Single check, generate attack ray from checker location and push
    // evasions.
    else if (checks == 1)
    {
        ray = between_dia[s.p_king_sq()][get_lsb(checker)] 
            | between_hor[s.p_king_sq()][get_lsb(checker)];
        push_pawn_moves(s, mlist, checker, ray);
        push_moves<KNIGHT>(s, mlist, checker, ray);
        push_moves<BISHOP>(s, mlist, checker, ray);
        push_moves<ROOK  >(s, mlist, checker, ray);
        push_moves<QUEEN >(s, mlist, checker, ray);
        push_king_moves(s, mlist, checks);
    }
    // Double check, only king moves are legal.
    else
    {
        push_king_moves(s, mlist, checks);
        return;
    }

    check_legal(s, mlist);
}