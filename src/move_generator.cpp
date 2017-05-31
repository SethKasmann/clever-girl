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

const bool promotion[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0
};

template<Color C>
void push_pawn_moves(State & s, MoveList * mlist, Check & ch)
{
    const U64   DBL   = C == WHITE ? Rank_3 : Rank_6;
    const U64   PROMO = C == WHITE ? Rank_8 : Rank_1;
    const int dir = C == WHITE ? 8 : -8;
    const Square * test;
    Square test_dst;
    bool promo_flag;
    for (test = s.piece_list[C][PAWN]; *test != NO_SQ; ++test)
    {
        test_dst = *test + dir;
        U64 test_dst_bit = 1ULL << test_dst;
        if (test_dst & PROMO)
        {
            if (test_dst_bit<<1 & s.e_occ() & PROMO)
            {
                mlist->push(*test, test_dst+1, QUEEN_PROMO,  QP);
                mlist->push(*test, test_dst+1, KNIGHT_PROMO, NP);
                mlist->push(*test, test_dst+1, ROOK_PROMO,   RP);
                mlist->push(*test, test_dst+1, BISHOP_PROMO, BP);
            }
            if (test_dst_bit>>1 & s.e_occ() & PROMO)
            {
                mlist->push(*test, test_dst-1, QUEEN_PROMO,  QP);
                mlist->push(*test, test_dst-1, KNIGHT_PROMO, NP);
                mlist->push(*test, test_dst-1, ROOK_PROMO,   RP);
                mlist->push(*test, test_dst-1, BISHOP_PROMO, BP);
            }
            if (test_dst_bit&s.empty())
            {
                mlist->push(*test, test_dst, QUEEN_PROMO,  QP);
                mlist->push(*test, test_dst, KNIGHT_PROMO, NP);
                mlist->push(*test, test_dst, ROOK_PROMO,   RP);
                mlist->push(*test, test_dst, BISHOP_PROMO, BP);
            }
        }
        else
        {
            if (s.board[s.them][test_dst+1] != NONE && test_dst_bit & NOT_A_FILE)
            {
                mlist->push(*test, test_dst+1, ATTACK, SCORE[PAWN][s.board[s.them][test_dst+1]]);
            }
            if (s.board[s.them][test_dst-1] != NONE && test_dst_bit & NOT_H_FILE)
            {
                mlist->push(*test, test_dst-1, ATTACK, SCORE[PAWN][s.board[s.them][test_dst-1]]);
            }
            if (test_dst_bit&s.empty())
            {
                mlist->push(*test, test_dst, QUIET, Q);
                if (test_dst&DBL && (test_dst+dir)&s.empty())
                {
                    mlist->push(*test, test_dst+dir, DBL_PUSH, Q);
                }
            }
        }
    }

    if (ch.checks)
    {
        for (Move m = *mlist->c; mlist->c < mlist->e; m = *mlist->c)
        {
            if (!(get_dst(m)&ch.ray||get_dst(m)&ch.checker))
                *mlist->c = *(--mlist->e);
            else
                mlist->c++;
        }
        mlist->c = mlist->_m;
    }

    U64 attack, push, dbl, promo, ep;
    Square src, dst;

    // En passant.
    ep = pawn_move_bb<PUSH, C>(s.en_passant & ch.checker) & s.empty(); 
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
void push_moves(State & s, MoveList * mlist, Check & ch)
{
    U64 m, a;
    Square * src;
    int score;
    for (src = s.piece_list[s.us][P]; *src != NO_SQ; ++src)
    {
        m  = s.attack_bb<P>(*src) & (ch.ray | ch.checker);
        a  = m & s.e_occ();
        m &= s.empty();
        while (a)
        {
            score = SCORE[P][s.on_square(get_lsb(a), s.them)];
            mlist -> push(*src, pop_lsb(a), ATTACK, score);
        }
        while (m)
            mlist -> push(*src, pop_lsb(m), QUIET, Q);
    }
}

// ----------------------------------------------------------------------------
// Push all legal King moves and attacks. This also handles castling moves.
// ----------------------------------------------------------------------------

void push_king_moves(State & s, MoveList * mlist, Check & ch)
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

    if (ch.checks == 2) return;

    while (a)
    {
        score = SCORE[KING][s.on_square(dst, s.them)];
        mlist -> push(k, pop_lsb(a), ATTACK, score);
    }

    if (ch.checks) return;

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

template<Color C>
void push(State & s, MoveList * mlist, Check & ch)
{
    push_pawn_moves<C>(s, mlist, ch);
    push_moves<KNIGHT>(s, mlist, ch);
    push_moves<BISHOP>(s, mlist, ch);
    push_moves<ROOK  >(s, mlist, ch);
    push_moves<QUEEN >(s, mlist, ch);
    push_king_moves(s, mlist, ch);
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

    Check ch(s);

    if (ch.checks == 2)
    {
        push_king_moves(s, mlist, ch);
        return;
    }
    else
    {
        s.us == WHITE ? push<WHITE>(s, mlist, ch)
                      : push<BLACK>(s, mlist, ch);
    }

    check_legal(s, mlist);
}