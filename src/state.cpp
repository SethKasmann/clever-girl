#include "state.h"
#include "zobrist.h"
#include <utility>

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
                mPawnKey ^= Zobrist::key(c, pawn, s);
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
    mCheckSquares.fill({});
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

bool State::isLegal(Move pMove) const
{
    Square src = getSrc(pMove);
    Square dst = getDst(pMove);

// -------------------------------------------------------------------------- //
//                                                                            //
// Check if the piece is pinned. If so, confirm it is moving along it's       //
// pin ray.                                                                   //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (square_bb[src] & mPinned[mUs]
        && !(coplanar[src][dst] & getPieceBB<king>(mUs)))
        return false;

// -------------------------------------------------------------------------- //
//                                                                            //
// Check for en-passant. Remove both the attacking pawn and the captured      //
// pawn from the occupancy and see if the king is left in check. This avoids  //
// the rare scenario where both pawns are pinned to the king by a slider.     //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (onSquare(src) == pawn && square_bb[dst] & mEnPassant)
    {
        U64 change = mUs == white ? square_bb[src] | square_bb[dst - 8]
                                  : square_bb[src] | square_bb[dst + 8];
        if (check(change))
            return false;
    }

// -------------------------------------------------------------------------- //
//                                                                            //
// Check for king non-castling king moves. See if the king is left in check   //
// after the move is made. Since castling moves are a more difficult case     //
// only legal castling moves are generated.                                   //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (onSquare(src) == king && !isCastle(pMove))
    {
        U64 change = square_bb[src];
        if (isAttacked(dst, mUs, change))
            return false;
    }

    return true;
}

// -------------------------------------------------------------------------- //
//                                                                            //
// isValid checks if a move is a valid move in the current state. This is     //
// used to check if a best move from the transposition table or a killer move //
// is valid before trying it. The key here is if the move is valid, we can    //
// try the move before generating the move list. If the move causes a beta    //
// cutoff in the search routine, we may not need to generate moves at all.    //
//                                                                            //
// -------------------------------------------------------------------------- //
bool State::isValid(Move pMove, U64 pValidMoves) const
{
    assert(getSrc(pMove) < no_sq);
    assert(getDst(pMove) < no_sq);

    Square src, dst;
    U64 ray;
    src = getSrc(pMove);
    dst = getDst(pMove);

// -------------------------------------------------------------------------- //
//                                                                            //
// The first line of defense is to simply check if the move is a null move.   //
// Killer moves are initialized as null moves, so this can occur if no killer //
// has been stored yet.                                                       //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (pMove == nullMove)
        return false;

// -------------------------------------------------------------------------- //
//                                                                            //
// Check for a bad promotion. If the move is a promotion then the piece to be //
// moved must be a pawn.                                                      //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (getPiecePromo(pMove) && (onSquare(src) != pawn))
        return false;

// -------------------------------------------------------------------------- //
//                                                                            //
// Confirm that if a move is a castle we are actually moving the king.        //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (isCastle(pMove) && onSquare(src) != king)
        return false;

// -------------------------------------------------------------------------- //
//                                                                            //
// Three of the more obvious checks a move has to pass:                       //
//   1. We occupy the source square.                                          //
//   2. We do not occupy the destination square.                              //
//   3. We are not capturing the enemy king.                                  //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (!(square_bb[src] & getOccupancyBB(mUs)) || 
        (square_bb[dst]  & getOccupancyBB(mUs)) ||
        dst == getKingSquare(mThem))
        return false;

    switch (onSquare(src))
    {

// -------------------------------------------------------------------------- //
//                                                                            //
// Pawns are the trickiest, so there are a number of cases to check.          //
//   1. If the pawn is promoting, make sure the move is a promotion.          //
//   2. If the move is en-passant, we need to confirm that the piece we are   //
//      capturing is a valid move square, and that capturing the enemy pawn   //
//      does not leave our king in check. This edge case will not be picked   //
//      up by the pin detection - since both pawns could be on the same       //
//      horizontal ray as the king.                                           //
//   3. If the pawn pushed, make sure the destination is empty and valid.     //
//   4. If the pawn double pushed, confirm the destination square is empty    //
//      and valid, and that the square between the source and destination is  //
//      empty.                                                                //
//   5. If the pawn is attacking, confirm the enemy occupys the destination.  //
//                                                                            //
// -------------------------------------------------------------------------- //
        case pawn:
        {
            if ((square_bb[dst] & (Rank_8 | Rank_1)) && !getPiecePromo(pMove))
                return false;
            if (square_bb[dst] & mEnPassant)
            {
                return pawn_push[mThem][dst] & pValidMoves;
            }
            std::pair<Square, Square> advance = std::minmax(src, dst);
            switch (advance.second - advance.first)
            {
                case 8:
                    return square_bb[dst] & getEmptyBB() & pValidMoves;
                case 16:
                    return square_bb[dst] & getEmptyBB() & pValidMoves &&
                           between_hor[src][dst] & getEmptyBB();
                case 7:
                case 9:
                    return square_bb[dst] & getOccupancyBB(mThem) & pValidMoves;
                default:
                    return false;
            }
        }

// -------------------------------------------------------------------------- //
//                                                                            //
// Knights, bishops, rooks, and queens are simple. Just check if the attack   //
// bitboard from the source square bitwise ANDs with the destination square   //
// and the valid moves bitboard.                                              //
//                                                                            //
// -------------------------------------------------------------------------- //
        case knight:
            return getAttackBB<knight>(src) & square_bb[dst] & pValidMoves;
        case bishop:
            return getAttackBB<bishop>(src) & square_bb[dst] & pValidMoves;
        case rook:
            return getAttackBB<rook>(src) & square_bb[dst] & pValidMoves;
        case queen:
            return getAttackBB<queen>(src) & square_bb[dst] & pValidMoves;

// -------------------------------------------------------------------------- //
//                                                                            //
// For king moves we first need to check if the move is a castling move. If   //
// so, there are four checks to confirm the move is valid in the current      //
// state:                                                                     //
//   1. The castle rights are valid.                                          //
//   2. There are no pieces between the king and rook.                        //
//   3. Confirm the square next to the king is not attacked (since you cannot //
//      castle through check).                                                //
//   4. Confirm the castle will not leave the king in check.                  //
//                                                                            //
// If the move is not a castling move, just make sure the destination will    //
// bitwise AND withe valid king moves bitboard.                               //
//                                                                            //
// -------------------------------------------------------------------------- //
        case king:
        {
            Square k = getKingSquare(mUs);
            if (isCastle(pMove))
            {
                if (src > dst)
                {
                    return (canCastleKingside() 
                    && !(between_hor[k][k-3] & getOccupancyBB())
                    && !attacked(k-1) 
                    && !attacked(k-2));
                }
                else
                {
                    return (canCastleQueenside()
                    && !(between_hor[k][k+4] & getOccupancyBB())
                    && !attacked(k+1)
                    && !attacked(k+2));
                }
            }
            return square_bb[dst];
        }
    }
// -------------------------------------------------------------------------- //
//                                                                            //
// If we didn't return inside the switch, something went wrong.               //
//                                                                            //
// -------------------------------------------------------------------------- //
    assert(false);
}

bool State::givesCheck(Move pMove) const
{
    Square src = getSrc(pMove);
    Square dst = getDst(pMove);
    PieceType piece = onSquare(src);

    // Direct check.
    if (getCheckSquaresBB(piece) & square_bb[dst])
        return true;

    // Discovered check.
    if ((getDiscoveredChecks(mUs) & square_bb[src]) &&
        !(coplanar[src][dst] & getPieceBB<king>(mThem)))
        return true;

    if (isEnPassant(pMove))
    {
        U64 change = square_bb[src] | square_bb[dst];
        change |= mUs == white ? square_bb[dst - 8] : square_bb[dst + 8];
        return check(change, mThem);
    }
    else if (isCastle(pMove))
    {
        Square rookSquare = src > dst ? dst + 1 : dst - 1;
        return getAttackBB(rook, rookSquare, getOccupancyBB() ^ square_bb[src]) &
               getPieceBB<king>(mThem);
    }
    else if (isPromotion(pMove))
    {
        PieceType promo = getPiecePromo(pMove);
        return getAttackBB(promo, dst, getOccupancyBB() ^ square_bb[src]) &
               getPieceBB<king>(mThem);
    }
    return false;
}

int State::see(Move m) const
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
        gain[d] = PieceValue[pawn];
    else if (getPiecePromo(m) == queen)
        gain[d] = Queen_wt - Pawn_wt;
    else
        gain[d] = PieceValue[onSquare(dst)];
    occupancy = getOccupancyBB();
    attackers = allAttackers(dst);
    // Get X ray attacks and add in pawns from attackers.
    xRay = getXRayAttacks(dst);

    while (from)
    {
        // Update the target piece and the square it came from.
        src = get_lsb(from);
        target = onSquare(src);

        // Update the depth and color.
        d++;

        // Storing the potential gain, if defended.
        gain[d] = PieceValue[target] - gain[d - 1];

        // Prune if the gain cannot be improved.
        if (std::max(-gain[d - 1], gain[d]) < 0)
            break;

        // Remove the from bit to simulate making the move.
        attackers ^= from;
        occupancy ^= from;

        // If the target is not a night, piece movement could cause a discovered
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
                if (!(between[mayAttack][dst] & occupancy))
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
        {
            from = get_lsb_bb(attackers & getPieceBB<king>(color));
            // Break if the king would be attacked.
            if (attackers & getOccupancyBB(!color))
                break;
        }
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

void State::make_t(Move pMove)
{
    assert(pMove != nullMove);
    assert(getSrc(pMove) < no_sq);
    assert(getDst(pMove) < no_sq);
    Square src, dst;
    PieceType moved, captured;
    bool epFlag = false;


    src = getSrc(pMove);
    dst = getDst(pMove);
    moved = onSquare(src);
    assert(moved != none);
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
        if (captured == pawn)
            mPawnKey ^= Zobrist::key(mThem, pawn, dst);
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

            mKey ^= Zobrist::key(get_file(mEnPassant));
            epFlag = true;
        }
        else if (getPiecePromo(pMove))
        {
            mPawnKey ^= Zobrist::key(mUs, pawn, dst);
            removePiece(mUs, pawn, dst);
            addPiece(mUs, getPiecePromo(pMove), dst);
        }
        else if (mEnPassant & square_bb[dst])
        {
            Square epCapture = (mUs == white) ? dst - 8 : dst + 8;
            mPawnKey ^= Zobrist::key(mThem, pawn, epCapture);
            removePiece(mThem, pawn, epCapture);
        }
    }

    if (!epFlag)
        mEnPassant = 0;

    // Update castle rights.
    mCastleRights &= Castle_rights[src];
    mCastleRights &= Castle_rights[dst];

    mKey ^= Zobrist::key(mCastleRights);

    assert(!check());
    swapTurn();

    setPins(white);
    setPins(black);
    setCheckers();
}

void State::makeNull()
{
    assert(mCheckers == 0);
    // Remove the ep file and castle rights from the zobrist key.
    if (mEnPassant)
        mKey ^= Zobrist::key(get_file(mEnPassant));

    // Reset the en-passant square.
    mEnPassant = 0;

    // Swap the turn.
    swapTurn();

    // Set check squares.
    mCheckSquares[pawn] = getAttackBB<pawn>(getKingSquare(mThem), mThem);
    mCheckSquares[knight] = getAttackBB<knight>(getKingSquare(mThem));
    mCheckSquares[bishop] = getAttackBB<bishop>(getKingSquare(mThem));
    mCheckSquares[rook] = getAttackBB<rook>(getKingSquare(mThem));
    mCheckSquares[queen] = mCheckSquares[bishop] | mCheckSquares[rook];
}

bool State::insufficientMaterial() const
{
    bool ret = false;

// -------------------------------------------------------------------------- //
//                                                                            //
// First check that there are no pawns or major pieces on the board.          //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (getPieceCount<pawn>() +
        getPieceCount<rook>() +
        getPieceCount<queen>() == 0)
    {
        switch (getPieceCount<knight>())
        {
// -------------------------------------------------------------------------- //
//                                                                            //
// If no knights are present, check if all remaining bishops are on the same  //
// color square.                                                              //
//                                                                            //
// -------------------------------------------------------------------------- //
            case 0:
                if ((getPieceBB<bishop>() & Dark_squares) == 
                        getPieceBB<bishop>() ||
                    (getPieceBB<bishop>() & Light_squares) == 
                        getPieceBB<bishop>())
                    ret = true;
                break;
// -------------------------------------------------------------------------- //
//                                                                            //
// If only 1 knight is present, check if there are no bishops on the board.   //
//                                                                            //
// -------------------------------------------------------------------------- //                
            case 1:
                if (!getPieceCount<bishop>())
                    ret = true;
                break;
// -------------------------------------------------------------------------- //
//                                                                            //
// For two knights, check if both sides have a knight and neither side has a  //
// bishop.                                                                    //
//                                                                            //
// -------------------------------------------------------------------------- //  
            case 2:
                if (getPieceCount<knight>(white) &&
                    getPieceCount<knight>(black) &&
                    !getPieceCount<bishop>())
                    ret = true;
                break;
            default:
                break;
        }
    }
    return ret;
}

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

    if (s.mUs == white)
        o << "White to move.\n";
    else 
        o << "Black to move.\n";
    
    //o << "Color(mUs)" << s.mUs << '\n';
    //o << "Color(mThem)" << s.mThem << '\n';

    /*
    print_bb(s.getOccupancyBB(white));
    print_bb(s.getOccupancyBB(black));
    */
    //std::cout << s.getKey() << '\n';

    return o;
}
