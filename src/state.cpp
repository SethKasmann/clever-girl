#include "state.h"
#include "zobrist.h"

// ----------------------------------------------------------------------------
// Copy constructor.
// ----------------------------------------------------------------------------

State::State(const State & s)
: mUs(s.mUs)
, mThem(s.mThem)
, mFiftyMoveRule(s.mFiftyMoveRule)
, mCastleRights(s.mCastleRights)
, mKey(s.mKey)
, mPawnKey(s.mPawnKey)
, mCheckers(s.mCheckers)
, mEnPassant(s.mEnPassant)
, mPinned(s.mPinned)
, mOccupancy(s.mOccupancy)
, mPieceIndex(s.mPieceIndex)
, mBoard(s.mBoard)
, mPstScore(s.mPstScore)
, mPieces(s.mPieces)
, mPieceCount(s.mPieceCount)
, mPieceList(s.mPieceList)
{}

void State::operator=(const State & s)
{
    mUs = s.mUs;
    mThem = s.mThem;
    mFiftyMoveRule = s.mFiftyMoveRule;
    mCastleRights = s.mCastleRights;
    mKey = s.mKey;
    mPawnKey = s.mPawnKey;
    mCheckers = s.mCheckers;
    mEnPassant = s.mEnPassant;
    mPinned = s.mPinned;
    mOccupancy = s.mOccupancy;
    mPieceIndex = s.mPieceIndex;
    mBoard = s.mBoard;
    mPieces = s.mPieces;
    mPieceCount = s.mPieceCount;
    mPstScore = s.mPstScore;
    mPieceList = s.mPieceList;
}

// ---------------------------------------------------------------------------- //
// Constructor to initialize a state based on the position's FEN string.        //
//                                                                              //
// Explination of FEN notation from wikipedia:                                  //
//                                                                              //
// "Forsyth–Edwards Notation (FEN) is a standard notation for describing a      //
// particular board position of a chess game. The purpose of FEN is to provide  //
// all the necessary information to restart a game from a particular position." //
// ---------------------------------------------------------------------------- //

State::State(const std::string & fen)
{
    int i, enpass, position;
    std::string::const_iterator it;
    Square s;
    Color c;
    PieceType p;

    init();

    position = 0;
    for (it = fen.begin(); it < fen.end(); ++it)
    {
        if (isdigit(*it))
            position += *it - '0';
        else if (isalpha(*it))
        {
            c = isupper(*it) ? white : black;
            s = last_sq - position;
            char t = tolower(*it);
            p = t == 'p' ? pawn
              : t == 'n' ? knight
              : t == 'b' ? bishop
              : t == 'r' ? rook
              : t == 'q' ? queen
              : king;
            addPiece(c, p, s);
            mKey ^= Zobrist::key(c, p, s);
            if (p == pawn)
                mPawnKey ^= Zobrist::key(c, p, s);
            position++;
        }
        else if (*it == ' ')
        {
            ++it;
            break;
        }
    }
    if (*it == 'w')
    {
        mUs = white;
        mThem = black;
    }
    else
    {
        mUs = black;
        mThem = white;
        mKey ^= Zobrist::key();
    }

    enpass = -1;
    for (++it; it < fen.end(); ++it)
    {
        if (*it == 'K')
            mCastleRights += w_king_castle;
        else if (*it == 'Q')
            mCastleRights += w_queen_castle;
        else if (*it == 'k')
            mCastleRights += b_king_castle;
        else if (*it == 'q')
            mCastleRights += b_queen_castle;
        else if (isalpha(*it))
        {
            enpass = 'h' - *it;
            ++it;
            enpass += 8 * (*it - '1');
        }
    }
    mKey ^= Zobrist::key(mCastleRights);

    if (enpass > -1)
    {
        //enpass += square_bb[enpass] & Rank_3 ? 8 : -8;
        mEnPassant = square_bb[enpass];
        mKey ^= Zobrist::key(get_file(mEnPassant));
    }

    // Initialize pins.
    setPins(white);
    setPins(black);

    // Initialize checkers.
    setCheckers();
}

void State::init()
{
    mUs = white;
    mThem = black;
    mFiftyMoveRule = 0;
    mCastleRights = 0;
    mKey = 0;
    mPawnKey = 0;
    mCheckers = 0;
    mEnPassant = 0;
    mPinned.fill({});
    mOccupancy.fill({});
    mPieceIndex.fill({});
    mBoard.fill(none);
    mPieces.fill({});
    mPieceCount.fill({});
    mPstScore.fill({});
    for (auto i = mPieceList.begin(); i != mPieceList.end(); ++i)
        for (auto j = i->begin(); j != i->end(); ++j)
            j->fill(no_sq);
}

// ----------------------------------------------------------------------------
// Function to return a bitboard of all pinned pieces for the current player.
// ----------------------------------------------------------------------------

void State::setPins(Color c)
{
    U64 pinners, ray;
    Square kingSq = getKingSquare(c);
    mPinned[c] = 0;

    pinners = bishopMoves[kingSq] & (getPieceBB<bishop>(!c) | getPieceBB<queen>(!c));

    while (pinners)
    {
        ray = between_dia[pop_lsb(pinners)][kingSq] & getOccupancyBB();
        if (pop_count(ray) == 1)
            mPinned[c] |= ray & getOccupancyBB(c);
    }

    pinners = rookMoves[kingSq] & (getPieceBB<rook>(!c) | getPieceBB<queen>(!c));

    while (pinners)
    {
        ray = between_hor[pop_lsb(pinners)][kingSq] & getOccupancyBB();
        if (pop_count(ray) == 1)
            mPinned[c] |= ray & getOccupancyBB(c);
    }
}

U64 State::getDiscoveredChecks(Color c) const
{
    U64 pinners, ray, discover = 0;
    Square kingSq = getKingSquare(!c);

    pinners = bishopMoves[kingSq] & (getPieceBB<bishop>(c) | getPieceBB<queen>(c));

    while (pinners)
    {
        ray = between_dia[pop_lsb(pinners)][kingSq] & getOccupancyBB();
        if (pop_count(ray) == 1)
            discover |= ray & getOccupancyBB(c);
    }

    pinners = rookMoves[kingSq] & (getPieceBB<rook>(c) | getPieceBB<queen>(c));

    while (pinners)
    {
        ray = between_hor[pop_lsb(pinners)][kingSq] & getOccupancyBB();
        if (pop_count(ray) == 1)
            discover |= ray & getOccupancyBB(c);
    }

    return discover;
}

bool State::isLegal(Move_t pMove) const
{
    if (!mPinned[mUs])
        return true;

    Square src = getSrc(pMove);
    Square dst = getDst(pMove);

    if (square_bb[src] & mPinned[mUs]
     && !(coplanar[src][dst] & getPieceBB<king>(mUs)))
        return false;

    return true;
}

int State::see(Move_t m) const
{
    Prop prop;
    Color color;
    Square src, dst, mayAttack;
    PieceType target;
    U64 attackers, from, occupancy, xRay, potential;
    int gain[32] = { 0 };
    int d = 0;

    color = mUs;
    src = getSrc(m);
    dst = getDst(m);
    from = square_bb[src];
    // Check if the move is en passant.
    if (onSquare(src) == pawn && square_bb[dst] & mEnPassant)
        gain[d] = getPieceValue(pawn);
    else if (getPiecePromo(m) == queen)
        gain[d] = Queen_wt - Pawn_wt;
    else
        gain[d] = getPieceValue(onSquare(dst));
    occupancy = getOccupancyBB();
    attackers = allAttackers(dst);
    // Get X ray attacks and add in pawns from attackers.
    xRay = getXRayAttacks(dst);


    while (from)
    {
        // Update the target piece and the square it came from.
        src = get_lsb(from);
        target = onSquare(src);

        // Break if the target is a king.
        if (target == king)
            break;

        // Update the depth and color.
        d++;

        // Storing the potential gain, if defended.
        gain[d] = getPieceValue(target) - gain[d - 1];

        // Prune if the gain cannot be improved.
        if (std::max(-gain[d - 1], gain[d]) < 0)
            break;

        // Remove the from bit to simulate making the move.
        attackers ^= from;
        occupancy ^= from;

        // If the target is not a night, piece movement could camUse a discovered
        // attacker.
        if (target != knight)
        {
            xRay &= occupancy;
            // Create a bitboard of potential discovered attackers.
            potential = coplanar[src][dst] & xRay;
            while (potential)
            {
                mayAttack = pop_lsb(potential);
                // If no bits are between the new square and the dst, there
                // is a new attacker.
                if (!(between[mayAttack][dst] & occupancy));
                {
                    attackers |= square_bb[mayAttack];
                    break;
                }
            }
        }

        // Get the next piece for the opposing player.
        color = !color;
        if (!(attackers & getOccupancyBB(color)))
            break;

        if (attackers & getPieceBB<pawn>(color))
            from = get_lsb_bb(attackers & getPieceBB<pawn>(color));
        else if (attackers & getPieceBB<knight>(color))
            from = get_lsb_bb(attackers & getPieceBB<knight>(color));
        else if (attackers & getPieceBB<bishop>(color))
            from = get_lsb_bb(attackers & getPieceBB<bishop>(color));
        else if (attackers & getPieceBB<rook>(color))
            from = get_lsb_bb(attackers & getPieceBB<rook>(color));
        else if (attackers & getPieceBB<queen>(color))
            from = get_lsb_bb(attackers & getPieceBB<queen>(color));
        else
            from = get_lsb_bb(attackers & getPieceBB<king>(color));
    }

    // Negamax the gain array to determine the final SEE value.
    while (--d > 0)
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);

    return gain[0];
}

// ----------------------------------------------------------------------------
// Make move function responsible for updating the state based on the source, 
// destination, and type of move.
// ----------------------------------------------------------------------------

void State::make_t(Move_t pMove)
{
    Square src, dst;
    PieceType moved, captured;
    bool epFlag = false;


    src = getSrc(pMove);
    dst = getDst(pMove);
    moved = onSquare(src);
    captured = onSquare(dst);

    // Update the Fifty Move Rule
    mFiftyMoveRule++;

    // Remove the ep file and castle rights from the zobrist key.
    if (mEnPassant)
        mKey ^= Zobrist::key(get_file(mEnPassant));

    mKey ^= Zobrist::key(mCastleRights);

    if (captured != none && !isCastle(pMove))
    {
        mFiftyMoveRule = 0;
        removePiece(mThem, captured, dst);
    }

    if (isCastle(pMove))
    {
        // Kingside Castle
        if (dst < src)
        {
            movePiece(mUs, rook, src-3, dst+1);
            movePiece(mUs, king, src, dst);
        }
        // Queenside Castle
        else
        {
            movePiece(mUs, rook, src+4, dst-1);
            movePiece(mUs, king, src, dst);
        }
    }
    else
        movePiece(mUs, moved, src, dst);

    if (moved == pawn)
    {
        mFiftyMoveRule = 0;
        mPawnKey ^= Zobrist::key(mUs, pawn, src, dst);

        // Check for double pawn push.
        if (int(std::max(src, dst)) - int(std::min(src, dst)) == 16)
        {
            mEnPassant = mUs == white ? square_bb[dst - 8]
                               : square_bb[dst + 8];

            mKey ^= Zobrist::key(get_file(dst));
            epFlag = true;
        }
        else if (getPiecePromo(pMove))
        {
            removePiece(mUs, pawn, dst);
            addPiece(mUs, getPiecePromo(pMove), dst);
        }
        else if (mEnPassant & square_bb[dst])
        {
            Square epCapture = mUs == white ? dst - 8 : dst + 8;
            mPawnKey ^= Zobrist::key(mUs, pawn, epCapture);
            removePiece(mThem, pawn, epCapture);
        }
    }

    if (!epFlag)
        mEnPassant = 0;

    // Update castle rights.
    mCastleRights &= Castle_rights[src];
    mCastleRights &= Castle_rights[dst];

    mKey ^= Zobrist::key(mCastleRights);
    mKey ^= Zobrist::key();

    assert(!check());
    swap_turn();

    setPins(white);
    setPins(black);
    setCheckers();
}

// ----------------------------------------------------------------------------
// Function to return a bitboard of all valid king moves for the current 
// player
// ----------------------------------------------------------------------------
U64 State::valid_king_moves() const
{
    U64 m;

    const Dir L   = mUs == white ? SW : NW;
    const Dir R   = mUs == white ? SE : NE;

    // Remove king from occupancy to check squares attacked behind the king.
    const U64 o = getOccupancyBB() ^ getPieceBB<king>(mUs);

    m = King_moves[getKingSquare(mUs)];
    m &= ~(shift_e(getPieceBB<pawn>(mThem), R) | shift_w(getPieceBB<pawn>(mThem), L));

    for (Square s : getPieceList<knight>(mThem))
    {
        if (s == no_sq)
            break;
        m &= ~(Knight_moves[s]);
    }

    for (Square s : getPieceList<bishop>(mThem))
    {
        if (s == no_sq)
            break;
        m &= ~(Bmagic(s, o));
    }

    for (Square s : getPieceList<rook>(mThem))
    {
        if (s == no_sq)
            break;
        m &= ~(Rmagic(s, o));
    }

    for (Square s : getPieceList<queen>(mThem))
    {
        if (s == no_sq)
            break;
        m &= ~(Qmagic(s, o));
    }

    m &= ~(King_moves[getKingSquare(mThem)]);
    m &= ~(getOccupancyBB(mUs));

    return m;
}


// ----------------------------------------------------------------------------
// Function to check whether the current player's king is in check. mUsed 
// mostly for debugging.
// ----------------------------------------------------------------------------
/*
bool State::in_check()
{
    const Square k = mPieceList[mUs][king][0];
    return (pawn_attacks[mUs][k] & e_pawn())
         | (Knight_moves[k] & e_knight())
         | (Bmagic(k, occ()) & (e_bishop() | e_queen()))
         | (Rmagic(k, occ()) & (e_rook() | e_queen()));
}
*/

// ----------------------------------------------------------------------------
// Function to print the board on the screen. mUsed for debugging.
// ----------------------------------------------------------------------------

std::ostream & operator << (std::ostream & o, const State & s)
{
    const char * W_pawn   = "\u2659";
    const char * W_knight = "\u2658";
    const char * W_bishop = "\u2657";
    const char * W_rook   = "\u2656";
    const char * W_queen  = "\u2655";
    const char * W_king   = "\u2654";
    const char * B_pawn   = "\u265F";
    const char * B_knight = "\u265E";
    const char * B_bishop = "\u265D";
    const char * B_rook   = "\u265C";
    const char * B_queen  = "\u265B";
    const char * B_king   = "\u265A";
    const char * Empty    = " ";

    std::string nums[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};
    const std::string bar = " +-+-+-+-+-+-+-+-+";

    o << bar << std::endl;
    for (int i = 63; i >= 0; --i)
    {
        U64 bit = square_bb[i];
        if (i % 8 == 7)
            o << nums[i / 8] << "|";

        o << (bit & s.getPieceBB<pawn>(white)   ? W_pawn
            : bit & s.getPieceBB<knight>(white) ? W_knight
            : bit & s.getPieceBB<bishop>(white) ? W_bishop
            : bit & s.getPieceBB<rook>(white)   ? W_rook
            : bit & s.getPieceBB<queen>(white)  ? W_queen
            : bit & s.getPieceBB<king>(white)   ? W_king
            : bit & s.getPieceBB<pawn>(black)   ? B_pawn
            : bit & s.getPieceBB<knight>(black) ? B_knight
            : bit & s.getPieceBB<bishop>(black) ? B_bishop
            : bit & s.getPieceBB<rook>(black)   ? B_rook
            : bit & s.getPieceBB<queen>(black)  ? B_queen
            : bit & s.getPieceBB<king>(black)   ? B_king
            : Empty) << "|";

        if (i % 8 == 0)
            o << '\n' << bar << '\n';
    }

    o << "  A B C D E F G H\n";

    /*
    o << "Color(mUs)" << s.mUs << '\n';
    o << "Color(mThem)" << s.mThem << '\n';

    print_bb(s.occ(white));
    print_bb(s.occ(black));
    */

    return o;
}
