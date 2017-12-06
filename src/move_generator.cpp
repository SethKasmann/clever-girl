
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

    //const Square* sq;
    Square dst;

    //for (sq = s.piece<pawn>(C); *sq != no_sq; ++sq)
    for (Square src : s.getPieceList<pawn>(C))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & Promo)
        {
            if (dst & Not_a_file && square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()))
            {
                mlist->push(makeMove(src, dst+1, queen));
                mlist->push(makeMove(src, dst+1, knight));
                mlist->push(makeMove(src, dst+1, rook));
                mlist->push(makeMove(src, dst+1, bishop));
            }
            if (dst & Not_h_file && square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()))
            {
                mlist->push(makeMove(src, dst-1, queen));
                mlist->push(makeMove(src, dst-1, knight));
                mlist->push(makeMove(src, dst-1, rook));
                mlist->push(makeMove(src, dst-1, bishop));
            }
            if (dst & s.getEmptyBB())
            {
                mlist->push(makeMove(src, dst, queen));
                mlist->push(makeMove(src, dst, knight));
                mlist->push(makeMove(src, dst, rook));
                mlist->push(makeMove(src, dst, bishop));
            }
        }
        else
        {
            if (square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()) && square_bb[dst] & Not_a_file)
            {
                mlist->push(makeMove(src, dst+1));
            }
            if (square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()) && square_bb[dst] & Not_h_file)
            {
                mlist->push(makeMove(src, dst-1));
            }
            if (square_bb[dst] & s.getEmptyBB())
            {
                mlist->push(makeMove(src, dst));
                if (pawn_dbl_push[C][src] && (dst+dir) & s.getEmptyBB())
                {
                    mlist->push(makeMove(src, dst+dir));
                }
            }
        }
    }

    
    if (ch.checks)
    {
        for (Move_t m = *mlist->c; mlist->c < mlist->e; m = *mlist->c)
        {
            if (!(getDst(m)&ch.ray||getDst(m)&ch.checker))
                *mlist->c = *(--mlist->e);
            else
                mlist->c++;
        }
        mlist->c = mlist->_m;
    }

    U64 a, en_pass;
    // En passant.

    if (s.getEnPassantBB() && ch.checker & pawn_push[!C][get_lsb(s.getEnPassantBB())])
    {
        dst = get_lsb(s.getEnPassantBB());
        a = pawn_attacks[!C][dst] & s.getPieceBB<pawn>(C);
        while (a)
        {
            if (s.check(pawn_push[!C][dst] | get_lsb_bb(a)))
                return;
            mlist->push(makeMove(pop_lsb(a), dst));
        }
    }
    /*
    if (s.ep & ch.checker)
    {
        assert(s.ep & ch.checker);
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
    }*/
}

// ----------------------------------------------------------------------------
// Push all pseudo-legal moves and attacks for Knights, Bishops, Rooks, and 
// Queens.
// ----------------------------------------------------------------------------

template <PieceType P>
void push_moves(State & s, MoveList * mlist, Check & ch)
{
    U64 m, a;
    int score;
    //for (src = s.piece<P>(s.getOurColor()); *src != no_sq; ++src)
    for (Square src : s.getPieceList<P>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<P>(src) & (ch.ray | ch.checker);
        a  = m & s.getOccupancyBB(s.getTheirColor());
        m &= s.getEmptyBB();
        while (a)
        {
            score = Score[P][s.onSquare(get_lsb(a))];
            mlist -> push(makeMove(src, pop_lsb(a)));
        }
        while (m)
        {
            U64 ray;
            bool gives_check = false;
            Square dst = pop_lsb(m);
            if (P == knight)
                gives_check = Knight_moves[dst] & s.getPieceBB<king>(s.getTheirColor());
            else if (P == bishop)
            {
                ray = between_dia[dst][s.getKingSquare(s.getTheirColor())];
                if (ray && pop_count(ray & s.getOccupancyBB()) == 0)
                    gives_check = true;
            }
            else if (P == rook)
            {
                ray = between_hor[dst][s.getKingSquare(s.getTheirColor())];
                if (ray && pop_count(ray & s.getOccupancyBB()) == 0)
                    gives_check = true;
            }
            else
            {
                ray = between_dia[dst][s.getKingSquare(s.getTheirColor())] | between_hor[dst][s.getKingSquare(s.getTheirColor())];
                if (ray && pop_count(ray & s.getOccupancyBB()) == 0)
                    gives_check = true;
            }

            if (gives_check)
                mlist -> push(makeMove(src, dst));
            else
                mlist -> push(makeMove(src, dst));
        }
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

    k = s.getKingSquare(s.getOurColor());

    m = s.valid_king_moves();
    a = m & s.getOccupancyBB(s.getTheirColor());
    m ^= a;

    while (m) 
        mlist -> push(makeMove(k, pop_lsb(m)));

    while (a)
    {
        score = Score[king][s.onSquare(get_lsb(a))];
        mlist -> push(makeMove(k, pop_lsb(a)));
    }

    if (ch.checks) return;

    if (s.canCastleKingside() 
        && !(between_hor[k][k-3] & s.getOccupancyBB())
        && !s.attacked(k-1) 
        && !s.attacked(k-2))
        mlist->push(makeCastle(k, k-2));

    if (s.canCastleQueenside()
        && !(between_hor[k][k+4] & s.getOccupancyBB())
        && !s.attacked(k+1)
        && !s.attacked(k+2))
        mlist->push(makeCastle(k, k+2));
}

// ----------------------------------------------------------------------------
// Check if moves are legal by removing any moves from the moves list
// where a pinned piece moves off it's pin ray.
// ----------------------------------------------------------------------------

void check_legal(State & s, MoveList * mlist)
{
    U64 pin, dc;

    // Set pinned pieces and discovered check pieces.
    pin = s.getPins(s.getOurColor());//s.get_pins();
    dc = s.getDiscoveredChecks(s.getOurColor());

    if (!pin && !dc) return;
    
    // Run through the move list. Remove any moves where the source location
    // is a pinned piece and the king square is not on the line formed by the
    // source and destination locations.
    for (Move_t m = *mlist->c; mlist->c < mlist->e; m = *mlist->c)
    {
        // Check for pins.
        if (square_bb[getSrc(m)] & pin ? 
            !(coplanar[getSrc(m)][getDst(m)] & s.getPieceBB<king>(s.getOurColor())) : false)
        {
            *mlist->c = *(--mlist->e);
            continue;
        }

        // Check for discovered checks.
        /*
        if ((square_bb[get_src(m)] & dc) && get_score(m) < QC && !(coplanar[get_src(m)][get_dst(m)] & s.getPieceBB<king>(s.getTheirColor())))
        {
            set_score(mlist->c, QC);
        }
        */
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
        mlist->set();
        return;
    }
    else
    {
        s.getOurColor() == white ? push<white>(s, mlist, ch)
                      : push<black>(s, mlist, ch);
    }

    check_legal(s, mlist);
    mlist->set();
}

/*
std::ostream & operator << (std::ostream & o, const MoveList & mlist)
{
    const Move_t * i;
    o << "MoveList size=" << mlist.size() << '\n'
      << "{\n";
    for (i = mlist._m; i < mlist.e; ++i)
    {
        o << "  " << toString(*i) << '\n';
    }
    o << "}\n";
}*/