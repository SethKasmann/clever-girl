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
            // Check for pins. If pins are found, add legal legal moves
            // that either:
            //     1. Attack the pinned piece.
            //     2. Move along the pinned ray.
            if (checks)
            {
                //std::cout << "pushing evasions...\n";
                push_evasions(s, mlist, checkers);
            }
            else
            {
                //std::cout << "pushing regular...\n";
                //push_pawn_attacks(s, pinned);
                push_pawn_moves(s, mlist);
                push_knight_moves(s, mlist);
                push_bishop_moves(s, mlist);
                push_rook_moves(s, mlist);
                push_queen_moves(s, mlist);
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
        check_pins(s, mlist);
    }

    void check_pins(State * s, MoveList * mlist)
    {
        U64 pin;
        const Square k = s->p_king_sq();
        // get pins
        //e_diagonal_sliders     = s->e_diagonal_sliders();
        //e_non_diagonal_sliders = s->e_non_diagonal_sliders();

        pin = s->get_pins();

        if (!pin) return;
        
        for (Move m = *mlist->c; mlist->c < mlist->e; m = *mlist->c)
        {
            if (get_src(m) & pin
             && !(coplanar[get_src(m)][get_dst(m)] & s->p_king()))
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
        push_knight_moves(s, mlist, ray, checker);        
        push_bishop_moves(s, mlist, ray, checker);        
        push_rook_moves(s, mlist, ray, checker);        
        push_queen_moves(s, mlist, ray, checker);        
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

    void push_knight_moves(State * s, MoveList * mlist, U64 ray, U64 checker)
    {
        U64 m, a;//, n;
        Square dst;//src, dst;
        int i, score;

        for (i = 0; i < s->piece_count[s->us][KNIGHT]; ++i)
        {
            m   = knight_moves[s->piece_list[s->us][KNIGHT][i]] & (ray | checker);
            a   = m & s->e_occ();
            m  &= s->empty();
            while (a)
            {
                dst   = pop_lsb(a);
                score = SCORE[KNIGHT][s->on_square(dst, s->them)];
                mlist -> push(s->piece_list[s->us][KNIGHT][i], dst, ATTACK, score);
            }
            while (m)
            {
                //dst   = pop_lsb(m);
                mlist -> push(s->piece_list[s->us][KNIGHT][i], pop_lsb(m), QUIET, Q);
            }
        }
    }

    void push_bishop_moves(State * s, MoveList * mlist, U64 ray, U64 checker)
    {
        U64 m, a;//, n;
        Square dst;//src, dst;
        int i, score;

        for (i = 0; i < s->piece_count[s->us][BISHOP]; ++i)
        {
            m   = Bmagic(s->piece_list[s->us][BISHOP][i], s->occ()) & (ray | checker);
            a   = m & s->e_occ();
            m  &= s->empty();
            while (a)
            {
                dst   = pop_lsb(a);
                score = SCORE[BISHOP][s->on_square(dst, s->them)];
                mlist -> push(s->piece_list[s->us][BISHOP][i], dst, ATTACK, score);
            }
            while (m)
            {
                //dst   = pop_lsb(m);
                mlist -> push(s->piece_list[s->us][BISHOP][i], pop_lsb(m), QUIET, Q);
            }
        }
    }

    void push_rook_moves(State * s, MoveList * mlist, U64 ray, U64 checker)
    {
        U64 m, a;//, n;
        Square dst;//src, dst;
        int i, score;

        for (i = 0; i < s->piece_count[s->us][ROOK]; ++i)
        {
            m   = Rmagic(s->piece_list[s->us][ROOK][i], s->occ()) & (ray | checker);
            a   = m & s->e_occ();
            m  &= s->empty();
            while (a)
            {
                dst   = pop_lsb(a);
                score = SCORE[ROOK][s->on_square(dst, s->them)];
                mlist -> push(s->piece_list[s->us][ROOK][i], dst, ATTACK, score);
            }
            while (m)
            {
                //dst   = pop_lsb(m);
                mlist -> push(s->piece_list[s->us][ROOK][i], pop_lsb(m), QUIET, Q);
            }
        }
    }

    void push_queen_moves(State * s, MoveList * mlist, U64 ray, U64 checker)
    {
        U64 m, a;//, n;
        Square dst;//src, dst;
        int i, score;

        for (i = 0; i < s->piece_count[s->us][QUEEN]; ++i)
        {
            m   = Qmagic(s->piece_list[s->us][QUEEN][i], s->occ()) & (ray | checker);
            a   = m & s->e_occ();
            m  &= s->empty();
            while (a)
            {
                dst   = pop_lsb(a);
                score = SCORE[QUEEN][s->on_square(dst, s->them)];
                mlist -> push(s->piece_list[s->us][QUEEN][i], dst, ATTACK, score);
            }
            while (m)
            {
                //dst   = pop_lsb(m);
                mlist -> push(s->piece_list[s->us][QUEEN][i], pop_lsb(m), QUIET, Q);
            }
        }
    }

    void push_king_moves(State * s, MoveList * mlist, int checks)
    {
        U64 m, a;
        int score;
        Square src, dst;

        src = s->p_king_sq();

        m = s->valid_king_moves();
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