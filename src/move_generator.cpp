
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

void MoveList::pushQuietChecks(State& s, Check ch)
{
    Square dst;
    U64 m, ray, promo, pawnChecks, discovered;
    int dir;

    discovered = s.getDiscoveredChecks(s.getOurColor());

    if (s.getOurColor() == white)
    {
        promo = Rank_8;
        dir = 8;
    }
    else
    {
        promo = Rank_1;
        dir = -8;
    }

    pawnChecks = s.getAttackBB<pawn>(s.getKingSquare(s.getTheirColor()), s.getTheirColor());

    for (Square src : s.getPieceList<pawn>(s.getOurColor()))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & promo)
        {
            if (square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_h_file 
              & s.getAttackBB<knight>(s.getKingSquare(s.getTheirColor())))
                push(makeMove(src, dst+1, knight));
            if (square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_a_file
              & s.getAttackBB<knight>(s.getKingSquare(s.getTheirColor())))
                push(makeMove(src, dst-1, knight));
            if (dst & s.getEmptyBB() & ch.ray & s.getAttackBB<knight>(s.getKingSquare(s.getTheirColor())))
                push(makeMove(src, dst, knight));
        }
        else
        {
            if (square_bb[dst] & s.getEmptyBB())
            {
                if (square_bb[dst] & ch.ray)
                {
                    if (square_bb[dst] & pawnChecks)
                        push(makeMove(src, dst));
                    else if (square_bb[src] & discovered 
                          && !(coplanar[src][dst] & s.getPieceBB<king>(s.getTheirColor())))
                        push(makeMove(src, dst));
                }
                if (pawn_dbl_push[s.getOurColor()][src] & square_bb[dst+dir] & s.getEmptyBB())
                {
                    if (square_bb[dst+dir] & ch.ray)
                    {
                        if (square_bb[dst+dir] & pawnChecks)
                            push(makeMove(src, dst+dir));
                        else if (square_bb[src] & discovered 
                              && !(coplanar[src][dst+dir] & s.getPieceBB<king>(s.getTheirColor())))
                            push(makeMove(src, dst+dir));
                    }
                }
            }
        }
    }

    for (Square src : s.getPieceList<knight>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<knight>(src) & ch.ray & s.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            if (s.getAttackBB<knight>(dst) & s.getPieceBB<king>(s.getTheirColor()))
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered)
                push(makeMove(src, dst));
        }
    }
    for (Square src : s.getPieceList<bishop>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<bishop>(src) & ch.ray & s.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            ray = between_dia[dst][s.getKingSquare(s.getTheirColor())];
            if (ray && pop_count(ray & s.getOccupancyBB()) == 0)
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered 
                  && !(coplanar[src][dst] & s.getPieceBB<king>(s.getTheirColor())))
                push(makeMove(src, dst));
        }
    }
    for (Square src : s.getPieceList<rook>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<rook>(src) & ch.ray & s.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            ray = between_hor[dst][s.getKingSquare(s.getTheirColor())];
            if (ray && pop_count(ray & s.getOccupancyBB()) == 0)
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered 
                  && !(coplanar[src][dst] & s.getPieceBB<king>(s.getTheirColor())))
                push(makeMove(src, dst));
        }
    }
    for (Square src : s.getPieceList<queen>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<queen>(src) & ch.ray & s.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            ray = between_hor[dst][s.getKingSquare(s.getTheirColor())]
                | between_dia[dst][s.getKingSquare(s.getTheirColor())];
            if (ray && pop_count(ray & s.getOccupancyBB()) == 0)
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered 
                  && !(coplanar[src][dst] & s.getPieceBB<king>(s.getTheirColor())))
                push(makeMove(src, dst));
        }
    }
}

template<Color C>
void MoveList::pushPawnAttacks(State& s, Check & ch)
{
    const U64 Promo = C == white ? Rank_8 : Rank_1;
    const int dir   = C == white ? 8      : -8;

    Square dst;
    int start = mSize;

    for (Square src : s.getPieceList<pawn>(C))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & Promo)
        {
            if (square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_h_file)
                push(makeMove(src, dst+1, queen));
            if (square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_a_file)
                push(makeMove(src, dst-1, queen));
            if (dst & s.getEmptyBB() & ch.ray)
                push(makeMove(src, dst, queen));
        }
        else
        {
            if (square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_h_file)
                push(makeMove(src, dst+1));
            if (square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_a_file)
                push(makeMove(src, dst-1));
        }
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
            push(makeMove(pop_lsb(a), dst));
        }
    }
}

template<Color C>
void MoveList::pushPawnMoves(State& s, Check & ch)
{
    const U64 Promo = C == white ? Rank_8 : Rank_1;
    const int dir   = C == white ? 8      : -8;

    Square dst;
    int start = mSize;

    for (Square src : s.getPieceList<pawn>(C))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & Promo)
        {
            if (square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_h_file)
            {
                push(makeMove(src, dst+1, knight));
                push(makeMove(src, dst+1, rook));
                push(makeMove(src, dst+1, bishop));
            }
            if (square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()) & ch.checker & Not_a_file)
            {
                push(makeMove(src, dst-1, knight));
                push(makeMove(src, dst-1, rook));
                push(makeMove(src, dst-1, bishop));
            }
            if (dst & s.getEmptyBB() & ch.ray)
            {
                push(makeMove(src, dst, knight));
                push(makeMove(src, dst, rook));
                push(makeMove(src, dst, bishop));
            }
        }
        else
        {
            if (square_bb[dst] & s.getEmptyBB())
            {
                if (square_bb[dst] & ch.ray)
                    push(makeMove(src, dst));
                if (pawn_dbl_push[C][src] & square_bb[dst+dir] & s.getEmptyBB() & ch.ray)
                    push(makeMove(src, dst+dir));
            }
        }
    }
}

template<Color C>
void MoveList::push_pawn_moves(State & s, Check & ch)
{
    const U64 Promo = C == white ? Rank_8 : Rank_1;
    const int dir   = C == white ? 8      : -8;

    Square dst;

    for (Square src : s.getPieceList<pawn>(C))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & Promo)
        {
            if (dst & Not_a_file && square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()))
            {
                push(makeMove(src, dst+1, queen));
                push(makeMove(src, dst+1, knight));
                push(makeMove(src, dst+1, rook));
                push(makeMove(src, dst+1, bishop));
            }
            if (dst & Not_h_file && square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()))
            {
                push(makeMove(src, dst-1, queen));
                push(makeMove(src, dst-1, knight));
                push(makeMove(src, dst-1, rook));
                push(makeMove(src, dst-1, bishop));
            }
            if (dst & s.getEmptyBB())
            {
                push(makeMove(src, dst, queen));
                push(makeMove(src, dst, knight));
                push(makeMove(src, dst, rook));
                push(makeMove(src, dst, bishop));
            }
        }
        else
        {
            if (square_bb[dst+1] & s.getOccupancyBB(s.getTheirColor()) && square_bb[dst] & Not_a_file)
            {
                push(makeMove(src, dst+1));
            }
            if (square_bb[dst-1] & s.getOccupancyBB(s.getTheirColor()) && square_bb[dst] & Not_h_file)
            {
                push(makeMove(src, dst-1));
            }
            if (square_bb[dst] & s.getEmptyBB())
            {
                push(makeMove(src, dst));
                if (pawn_dbl_push[C][src] && (dst+dir) & s.getEmptyBB())
                {
                    push(makeMove(src, dst+dir));
                }
            }
        }
    }

    if (ch.checks)
    {
        for (int i = 0; i < mSize; ++i)
        {
            if (!(getDst(mList[i])&ch.ray||getDst(mList[i])&ch.checker))
                mList[i--] = mList[--mSize];
        }
    }
    /*
    if (ch.checks)
    {
        for (Move_t m = *c; c < e; m = *c)
        {
            if (!(getDst(m)&ch.ray||getDst(m)&ch.checker))
                *c = *(--e);
            else
                c++;
        }
        c = _m;
    }*/

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
            push(makeMove(pop_lsb(a), dst));
        }
    }
}

// ----------------------------------------------------------------------------
// Push all pseudo-legal moves and attacks for Knights, Bishops, Rooks, and 
// Queens.
// ----------------------------------------------------------------------------

template <PieceType P>
void MoveList::pushAttackMoves(State & s, Check & ch)
{
    U64 m;
    for (Square src : s.getPieceList<P>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<P>(src) & ch.checker & s.getOccupancyBB(s.getTheirColor());;
        while (m)
            push(makeMove(src, pop_lsb(m)));
    }
}

template <PieceType P>
void MoveList::pushMoves(State & s, Check & ch)
{
    U64 m;
    for (Square src : s.getPieceList<P>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<P>(src) & ch.ray & s.getEmptyBB();
        while (m)
            push(makeMove(src, pop_lsb(m)));
    }
}

template <PieceType P>
void MoveList::push_moves(State & s, Check & ch)
{
    U64 m, a;
    for (Square src : s.getPieceList<P>(s.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = s.getAttackBB<P>(src) & (ch.ray | ch.checker);
        a  = m & s.getOccupancyBB(s.getTheirColor());
        m &= s.getEmptyBB();
        while (a)
            push(makeMove(src, pop_lsb(a)));
        while (m)
            push(makeMove(src, pop_lsb(m)));
    }
}

// ----------------------------------------------------------------------------
// Push all legal King moves and attacks. This also handles castling moves.
// ----------------------------------------------------------------------------

void MoveList::pushKingAttacks(State& s, Check& ch)
{
    U64 m;
    Square k, dst;

    k = s.getKingSquare(s.getOurColor());

    m = ch.validKingMoves & s.getOccupancyBB(s.getTheirColor());

    while (m) 
        push(makeMove(k, pop_lsb(m)));
}

void MoveList::pushKingMoves(State& s, Check& ch)
{
    U64 m;
    Square k, dst;

    k = s.getKingSquare(s.getOurColor());

    m = ch.validKingMoves & s.getEmptyBB();

    while (m) 
        push(makeMove(k, pop_lsb(m)));

    if (ch.checks) return;

    if (s.canCastleKingside() 
        && !(between_hor[k][k-3] & s.getOccupancyBB())
        && !s.attacked(k-1) 
        && !s.attacked(k-2))
        push(makeCastle(k, k-2));

    if (s.canCastleQueenside()
        && !(between_hor[k][k+4] & s.getOccupancyBB())
        && !s.attacked(k+1)
        && !s.attacked(k+2))
        push(makeCastle(k, k+2));    
}

void MoveList::push_king_moves(State & s, Check & ch)
{
    U64 m, a;
    Square k, dst;

    k = s.getKingSquare(s.getOurColor());

    m = ch.validKingMoves;
    a = m & s.getOccupancyBB(s.getTheirColor());
    m ^= a;

    while (m) 
        push(makeMove(k, pop_lsb(m)));

    while (a)
        push(makeMove(k, pop_lsb(a)));

    if (ch.checks) return;

    if (s.canCastleKingside() 
        && !(between_hor[k][k-3] & s.getOccupancyBB())
        && !s.attacked(k-1) 
        && !s.attacked(k-2))
        push(makeCastle(k, k-2));

    if (s.canCastleQueenside()
        && !(between_hor[k][k+4] & s.getOccupancyBB())
        && !s.attacked(k+1)
        && !s.attacked(k+2))
        push(makeCastle(k, k+2));
}

// ----------------------------------------------------------------------------
// Check if moves are legal by removing any moves from the moves list
// where a pinned piece moves off it's pin ray.
// ----------------------------------------------------------------------------

void MoveList::check_legal(State & s)
{
    U64 pin, dc;

    // Set pinned pieces and discovered check pieces.
    pin = s.getPinsBB(s.getOurColor());//s.get_pins();
    dc = s.getDiscoveredChecks(s.getOurColor());

    if (!pin && !dc) return;
    
    // Run through the move list. Remove any moves where the source location
    // is a pinned piece and the king square is not on the line formed by the
    // source and destination locations.
    for (int i = 0; i < mSize; ++i)
    {
        if (square_bb[getSrc(mList[i])] & pin ? 
            !(coplanar[getSrc(mList[i])][getDst(mList[i])] & s.getPieceBB<king>(s.getOurColor())) : false)
            mList[i--] = mList[--mSize];
    }
    /*
    for (Move_t m = *c; c < e; m = *c)
    {
        // Check for pins.
        if (square_bb[getSrc(m)] & pin ? 
            !(coplanar[getSrc(m)][getDst(m)] & s.getPieceBB<king>(s.getOurColor())) : false)
        {
            *c = *(--e);
            continue;
        }*/

        // Check for discovered checks.
        /*
        if ((square_bb[get_src(m)] & dc) && get_score(m) < QC && !(coplanar[get_src(m)][get_dst(m)] & s.getPieceBB<king>(s.getTheirColor())))
        {
            set_score(mlist->c, QC);
        }
        */
        //c++;
    //}
}

template<Color C>
void MoveList::push_all(State & s, Check & ch)
{
    //push_pawn_moves<C>(s, ch);
    /*
    pushPawnMoves<C>(s, ch);
    pushAttackMoves<knight>(s, ch);
    pushAttackMoves<bishop>(s, ch);
    pushAttackMoves<rook>(s, ch);
    pushAttackMoves<queen>(s, ch);
    pushKingMoves(s, ch);*/

    /*
    pushPawnAttacks<C>(s, ch);
    pushMoves<knight>(s, ch);
    pushMoves<bishop>(s, ch);
    pushMoves<rook>(s, ch);
    pushMoves<queen>(s, ch);
    pushKingAttacks(s, ch);
    */
    pushQuietChecks(s, ch);
    std::cout << s;
    std::cout << mSize << '\n';
    int z;
    std::cin >> z;

    /*
    push_moves<knight>(s, ch);
    push_moves<bishop>(s, ch);
    push_moves<rook  >(s, ch);
    push_moves<queen >(s, ch);
    */
    //push_king_moves(s, ch);
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

void MoveList::push_moves(State & s)
{

    Check ch(s);

    if (ch.checks == 2)
    {
        push_king_moves(s, ch);
        return;
    }
    else
    {
        s.getOurColor() == white ? push_all<white>(s, ch)
                                 : push_all<black>(s, ch);
    }

    check_legal(s);
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