#include "move_generator.h"

namespace MoveGenerator
{

    const U64 KINGSIDE_CASTLE_MASK[2] = {0x6, 0x600000000000000};
    const U64 QUEENSIDE_CASTLE_MASK[2] = {0x70, 0x7000000000000000};
    const int SCORE[6][7]
    {
        { 25, 29, 30, 32, 35, 0,  4 },  
        { 19, 24, 26, 28, 34, 0,  4 },  
        { 18, 20, 23, 27, 33, 0,  4 },  
        { 15, 16, 17, 22, 31, 0,  4 },  
        { 11, 12, 13, 14, 21, 0,  4 },  
        { 5,  6,  7,  8,  9,  0,  4 }, 
    };

    void push_moves(State * s, MoveList * mlist)
    {
        U64 e_diagonal_sliders, e_non_diagonal_sliders, ray,
            attacked, checkers, king_moves;
        int checks;

        const Color us   = s->us;
        const Color them = s->them;
        const Square k = s->p_king_sq();


        checkers = 0;
        checks   = s->check_count(checkers);

        if (checks < 2)
        {
            if (checks)
            {
                push_evasions(s, mlist, checkers);
            }
            else
            {
                push_pawn_moves(s, mlist);
                push_moves<KNIGHT>(s, mlist);
                push_moves<BISHOP>(s, mlist);
                push_moves<ROOK  >(s, mlist);
                push_moves<QUEEN >(s, mlist);
                push_king_moves(s, mlist, checks);
            }
        }
        else
        {
            assert(checks > 0);
            // double check.
            push_king_moves(s, mlist, checks);
            return;
        }
        check_legal(s, mlist);
    }

    void check_legal(State * s, MoveList * mlist)
    {
        U64 pin;
        const Square k = s->p_king_sq();

        pin = s->get_pins();

        if (!pin) return;
        
        for (Move m = *mlist->c; mlist->c < mlist->e; m = *mlist->c)
        {
            if (get_src(m) & pin
             && !(coplanar[get_src(m)][get_dst(m)] & s->p_king())
             || (get_src(m) == k && !s->vkm(get_dst(m))))
            {
                // illegal move
                *mlist->c = *(--mlist->e);
            }
            else
            {
                mlist->c++;
            }
        }
    }

    void push_evasions(State * s, MoveList * mlist, U64 checker)
    {
        U64 ray;
        ray = between_dia[s->p_king_sq()][get_lsb(checker)]
            | between_hor[s->p_king_sq()][get_lsb(checker)];
        push_pawn_moves(s, mlist, checker, ray);
        push_moves<KNIGHT>(s, mlist, checker, ray);
        push_moves<BISHOP>(s, mlist, checker, ray);
        push_moves<ROOK  >(s, mlist, checker, ray);
        push_moves<QUEEN >(s, mlist, checker, ray);
        push_king_moves(s, mlist, 1);
    }

    void push_pawn_moves(State * s, MoveList * mlist, U64 checker, U64 check_ray)
    {
        const Color C = s->us == WHITE ? WHITE  : BLACK;
        const Dir   P     = C == WHITE ? N      : S;
        const Dir   L     = C == WHITE ? NW     : SW;
        const Dir   R     = C == WHITE ? NE     : SE;
        const U64   DBL   = C == WHITE ? Rank_2 : Rank_7;
        const U64   PROMO = C == WHITE ? Rank_8 : Rank_1;

        U64 attack, push, dbl, promo, no_promo, ep;
        Square dst;

        const U64 pawns = s->p_pawn();

        // Pawn attacks and promotion attacks to the east.
        attack   = shift_e(pawns, R) & s->e_occ() & checker;
        promo    = attack & PROMO;
        no_promo = attack ^ promo;

        while (no_promo)
        {
            dst = pop_lsb(no_promo);
            mlist->push(dst - R, dst, ATTACK, SCORE[PAWN][s->on_square(dst, !C)]);
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
        attack   = shift_w(pawns, L) & s->e_occ() & checker;
        promo    = attack & PROMO;
        no_promo = attack ^ promo;

        while (no_promo)
        {
            dst = pop_lsb(no_promo);
            mlist->push(dst - L, dst, ATTACK, SCORE[PAWN][s->on_square(dst, !C)]);
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
        push     = shift(pawns, P) & s->empty() & check_ray;
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

        dbl = shift(shift(pawns & DBL, P) & s->empty(), P) & s->empty() & check_ray;

        while(dbl)
        {
            dst = pop_lsb(dbl);
            mlist->push(dst - P - P, dst, DBL_PUSH, Q);
        }

        // En passant.
        ep = shift(s->en_passant & checker, P) & s->empty();
        if (ep)
        {
            dst = get_lsb(ep);
            attack = pawn_attacks[!C][dst] & s->p_pawn();
            while (attack)
            {
                if (s->is_attacked_by_slider(s->en_passant | get_lsb_bb(attack)))
                {
                    return;
                }
                mlist->push(pop_lsb(attack), dst, EN_PASSANT, EP);
            }
        }
    }

    template <PieceType P>
    void push_moves(State * s, MoveList * mlist, U64 ray, U64 checker)
    {
        U64 m, a;
        Square dst;
        int i, score;
        for (i = 0; i < s->piece_count[s->us][P]; ++i)
        {
            m   = P == KNIGHT ? knight_moves[s->piece_list[s->us][KNIGHT][i]] & (ray | checker)
                : P == BISHOP ? Bmagic(s->piece_list[s->us][P][i], s->occ())  & (ray | checker)
                : P == ROOK   ? Rmagic(s->piece_list[s->us][P][i], s->occ())  & (ray | checker)
                              : Qmagic(s->piece_list[s->us][P][i], s->occ())  & (ray | checker);

            a   = m & s->e_occ();
            m  &= s->empty();
            while (a)
            {
                dst   = pop_lsb(a);
                score = SCORE[P][s->on_square(dst, s->them)];
                mlist -> push(s->piece_list[s->us][P][i], dst, ATTACK, score);
            }
            while (m)
            {
                mlist -> push(s->piece_list[s->us][P][i], pop_lsb(m), QUIET, Q);
            }
        }
    }

    void push_king_moves(State * s, MoveList * mlist, int checks)
    {
        U64 m, a;
        int score;
        Square src, dst;

        src = s->p_king_sq();

        //m = s->valid_king_moves();
        m = king_moves[src] & ~s->p_occ();
        a = m & s->e_occ();
        m ^= a;

        while (m)
        {
            dst = pop_lsb(m);
            mlist -> push(src, dst, QUIET, Q);
        }

        if (checks == 2) return;

        while (a)
        {
            dst = pop_lsb(a);
            score = SCORE[KING][s->on_square(dst, s->them)];
            mlist -> push(src, dst, ATTACK, score);
        }

        if (checks) return;

        Color t = s->us;
        if ((KINGSIDE_CASTLE_MASK[t] & s->occ()) == 0 &&
            s->k_castle())
        {
            int src, dst;
            if (t) { src = 59; dst = 57; }
            else   { src = 3; dst = 1;   }
            if (!s->is_attacked(Square(src-1)) && !s->is_attacked(Square(src-2)))
            {
                mlist->push(src, dst, KING_CAST, C);
            }
        }
        if ((QUEENSIDE_CASTLE_MASK[t] & s->occ()) == 0 &&
            s->q_castle())
        {
            int src, dst;
            if (t) { src = 59; dst = 61; }
            else   { src = 3; dst = 5;   }
            if (!s->is_attacked(Square(src+1)) && !s->is_attacked(Square(src+2)))
            {
                mlist->push(src, dst, QUEEN_CAST, C);
            }
        }
    }

    void init()
    {
        initmagicmoves();
    }
}