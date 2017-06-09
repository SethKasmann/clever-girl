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

template<Color C>
void push_pawn_moves(State & s, MoveList * mlist, Check & ch)
{
    const U64 Dbl   = C == white ? Rank_3 : Rank_6;
    const U64 Promo = C == white ? Rank_8 : Rank_1;
    const int dir   = C == white ? 8      : -8;

    const Square* sq;
    Square src, dst;

    for (sq = s.piece<pawn>(C); *sq != no_sq; ++sq)
    {
        src = *sq;
        dst = src + dir;
        if (dst & Promo)
        {
            if (dst & Not_a_file && s.board[s.them][dst+1] != none)
            {
                mlist->push(src, dst+1, queen_promo,  QP);
                mlist->push(src, dst+1, knight_promo, NP);
                mlist->push(src, dst+1, rook_promo,   RP);
                mlist->push(src, dst+1, bishop_promo, BP);
            }
            if (dst & Not_h_file && s.board[s.them][dst-1] != none)
            {
                mlist->push(src, dst-1, queen_promo,  QP);
                mlist->push(src, dst-1, knight_promo, NP);
                mlist->push(src, dst-1, rook_promo,   RP);
                mlist->push(src, dst-1, bishop_promo, BP);
            }
            if (dst & s.empty())
            {
                mlist->push(src, dst, queen_promo,  QP);
                mlist->push(src, dst, knight_promo, NP);
                mlist->push(src, dst, rook_promo,   RP);
                mlist->push(src, dst, bishop_promo, BP);
            }
        }
        else
        {
            if (s.board[s.them][dst+1] != none && dst & Not_a_file)
            {
                mlist->push(src, dst+1, attack, Score[pawn][s.board[s.them][dst+1]]);
            }
            if (s.board[s.them][dst-1] != none && dst & Not_h_file)
            {
                mlist->push(src, dst-1, attack, Score[pawn][s.board[s.them][dst-1]]);
            }
            if (dst & s.empty())
            {
                mlist->push(src, dst, quiet, Q);
                if (pawn_dbl_push[C][src] && (dst+dir) & s.empty())
                {
                    mlist->push(src, dst+dir, dbl_push, Q);
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

    U64 a, push, dbl, promo, en_pass;
    // En passant.
    if (s.ep)
    {
        en_pass = pawn_push[C][get_lsb(s.ep & ch.checker)] & s.empty();
        if (en_pass)
        {
            dst = get_lsb(en_pass);
            a = pawn_attacks[!C][dst] & s.piece_bb<pawn>(C);
            while (a)
            {
                if (s.check(s.ep | get_lsb_bb(a)))
                {
                    return;
                }
                mlist->push(pop_lsb(a), dst, en_passant, EP);
            }
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
    const Square* src;
    int score;
    for (src = s.piece<P>(s.us); *src != no_sq; ++src)
    {
        m  = s.attack_bb<P>(*src) & (ch.ray | ch.checker);
        a  = m & s.occ(s.them);
        m &= s.empty();
        while (a)
        {
            score = Score[P][s.on_square(get_lsb(a), s.them)];
            mlist -> push(*src, pop_lsb(a), attack, score);
        }
        while (m)
            mlist -> push(*src, pop_lsb(m), quiet, Q);
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

    k = s.king_sq(s.us);

    m = s.valid_king_moves();
    a = m & s.occ(s.them);
    m ^= a;

    while (m) 
        mlist -> push(k, pop_lsb(m), quiet, Q);

    if (ch.checks == 2) return;

    while (a)
    {
        score = Score[king][s.on_square(dst, s.them)];
        mlist -> push(k, pop_lsb(a), attack, score);
    }

    if (ch.checks) return;

    if (s.k_castle() 
        && !(between_hor[k][k-3] & s.occ())
        && !s.attacked(k-1) 
        && !s.attacked(k-2))
        mlist->push(k, k-2, king_cast, C);

    if (s.q_castle()
        && !(between_hor[k][k+4] & s.occ())
        && !s.attacked(k+1)
        && !s.attacked(k+2))
        mlist->push(k, k+2, queen_cast, C);
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
        if (get_src(m) & pin ? 
            !(coplanar[get_src(m)][get_dst(m)] & s.piece_bb<king>(s.us)) : false)
            *mlist->c = *(--mlist->e);
        else
            mlist->c++;
    }
}

template<Color C>
void push(State & s, MoveList * mlist, Check & ch)
{
    push_pawn_moves<C>(s, mlist, ch);
    push_moves<knight>(s, mlist, ch);
    push_moves<bishop>(s, mlist, ch);
    push_moves<rook  >(s, mlist, ch);
    push_moves<queen >(s, mlist, ch);
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
        s.us == white ? push<white>(s, mlist, ch)
                      : push<black>(s, mlist, ch);
    }

    check_legal(s, mlist);
}