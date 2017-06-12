#include "state.h"
#include "zobrist.h"

// ----------------------------------------------------------------------------
// Copy constructor.
// ----------------------------------------------------------------------------

State::State(const State & s)
    : fmr(s.fmr), castle(s.castle), ep(s.ep), 
      key(s.key), us(s.us),         them(s.them)
{
    std::memcpy(board,       s.board,       sizeof board);
    std::memcpy(piece_count, s.piece_count, sizeof piece_count);
    std::memcpy(piece_list,  s.piece_list,  sizeof piece_list);
    std::memcpy(piece_index, s.piece_index, sizeof piece_index);
    std::memcpy(pieces,      s.pieces,      sizeof pieces);
    std::memcpy(occupancy,   s.occupancy,   sizeof occupancy);
}

void State::operator=(const State & s)
{
    us =     s.us;
    them =   s.them;
    castle = s.castle;
    key =    s.key;
    fmr =    s.fmr;
    ep =     s.ep;
    std::memcpy(board,       s.board,       sizeof board);
    std::memcpy(piece_count, s.piece_count, sizeof piece_count);
    std::memcpy(piece_list,  s.piece_list,  sizeof piece_list);
    std::memcpy(piece_index, s.piece_index, sizeof piece_index);
    std::memcpy(pieces,      s.pieces,      sizeof pieces);
    std::memcpy(occupancy,   s.occupancy,   sizeof occupancy);
}

// ----------------------------------------------------------------------------
// Constructor to initialize a state based on the position's FEN string.
//
// Explination of FEN notation from wikipedia:
//
// "Forsyth–Edwards Notation (FEN) is a standard notation for describing a 
// particular board position of a chess game. The purpose of FEN is to provide 
// all the necessary information to restart a game from a particular position."
// ----------------------------------------------------------------------------

State::State(std::string & fen) 
  : fmr(0), castle(0), board(), piece_count(), piece_list(), piece_index(), 
    pieces(), occupancy(), ep(0), key(0), us(white), them(black)
{
    int i, enpass, position;
    std::string::iterator it;
    Square s;
    Color c;
    PieceType p;

    for (p = pawn; p < none; ++p)
    {
        for (i = 0; i < Piece_max; ++i)
        {
            piece_list[white][p][i] = no_sq;
            piece_list[black][p][i] = no_sq;
        }
    }
    for (s = first_sq; s <= last_sq; ++s)
    {
        board[white][s] = none;
        board[black][s] = none;
    }
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
            add_piece(c, p, s);
            position++;
        }
        else if (*it == ' ')
        {
            ++it;
            break;
        }
    }
    us   = *it == 'w' ? white : black;
    them = !us;

    enpass = -1;
    for (++it; it < fen.end(); ++it)
    {
        if (*it == 'K')
            castle += w_king_castle;
        else if (*it == 'Q')
            castle += w_queen_castle;
        else if (*it == 'k')
            castle += b_king_castle;
        else if (*it == 'q')
            castle += b_queen_castle;
        else if (isalpha(*it))
        {
            enpass = 'h' - *it;
            ++it;
            enpass += 8 * (*it - '1');
        }
    }

    if (enpass > -1)
    {
        enpass += square_bb[enpass] & Rank_3 ? 8 : -8;
        ep = square_bb[enpass];
    }

    Zobrist::init_pieces(this);
}

// ----------------------------------------------------------------------------
// Function to return a bitboard of all pinned pieces for the current player.
// ----------------------------------------------------------------------------

U64 State::get_pins() const
{
    int i;
    U64 ray, pin = 0;
    for (i = 0; i < piece_count[them][bishop]; ++i)
    {
        ray = between_dia[piece_list[them][bishop][i]][king_sq(us)];
        if (pop_count(ray & occ()) == 1 && ray & occ(us))
            pin |= ray & occ(us);
    }
    for (i = 0; i < piece_count[them][rook]; ++i)
    {
        ray = between_hor[piece_list[them][rook][i]][king_sq(us)];
        if (pop_count(ray & occ()) == 1 && ray & occ(us))
            pin |= ray & occ(us);
    }
    for (i = 0; i < piece_count[them][queen]; ++i)
    {
        ray = between_dia[piece_list[them][queen][i]][king_sq(us)]
            | between_hor[piece_list[them][queen][i]][king_sq(us)];
        if (pop_count(ray & occ()) == 1 && ray & occ(us))
            pin |= ray & occ(us);
    }
    return pin;
}

// ----------------------------------------------------------------------------
// Make move function responsible for updating the state based on the source, 
// destination, and type of move.
// ----------------------------------------------------------------------------

void State::make(Move m)
{
    const Square src  = get_src(m);
    const Square dst  = get_dst(m);

    // Update the 50 move rule.
    board[us][src] == pawn || board[them][dst] != none ? 
        fmr = 0 : fmr++;

    // Remove the ep file and castle rights from the zobrist key.
    if (ep) Zobrist::ep(key, ep);
    Zobrist::castle(key, castle);

    switch (get_prop(m))
    {
        // Quiet moves.
        case quiet:
        {
            Zobrist::move(this, src, dst);
            move_piece(src, dst);
            ep = 0;
            break;
        }
        // Attacking moves.
        case attack:
        {
            Zobrist::move(this, src, dst);
            del_piece(them, dst);
            move_piece(src, dst);
            ep = 0;
            break;
        }
        // Double pawn push.
        case dbl_push:
        {
            Zobrist::move(this, src, dst);
            move_piece(src, dst);
            ep = square_bb[dst];
            Zobrist::ep(key, ep);
            break;
        }
        // King castle.
        case king_cast:
        {
            const int rook_src = src-3;
            const int rook_dst = dst+1;
            Zobrist::move(this, rook_src, rook_dst);
            Zobrist::move(this, src, dst);
            move_piece(rook_src, rook_dst);
            move_piece(src, dst);
            ep = 0;
            break;
        }
        // Queen castle.
        case queen_cast:
        {
            const int rook_src = src+4;
            const int rook_dst = dst-1;
            Zobrist::move(this, rook_src, rook_dst);
            Zobrist::move(this, src, dst);
            move_piece(rook_src, rook_dst);
            move_piece(src, dst);
            ep = 0;
            break;
        }
        // Queen promotion.
        case queen_promo:
        {
            Zobrist::promo(this, src, dst, queen);
            del_piece(us, src);
            add_piece(dst, queen);
            ep = 0;
            break;
        }
        // Knight underpromotion.
        case knight_promo:
        {
            Zobrist::promo(this, src, dst, knight);
            del_piece(us, src);
            add_piece(dst, knight);
            ep = 0;
            break;
        }
        // Rook underpromotion.
        case rook_promo:
        {
            Zobrist::promo(this, src, dst, rook);
            del_piece(us, src);
            add_piece(dst, rook);
            ep = 0;
            break;
        }
        // Bishop underpromotion.
        case bishop_promo:
        {
            Zobrist::promo(this, src, dst, bishop);
            del_piece(us, src);
            add_piece(dst, bishop);
            ep = 0;
            break;
        }
        // En-Passant.
        case en_passant:
        {
            Zobrist::en_passant(this, src, dst, get_lsb(ep));
            move_piece(src, dst);
            del_piece(them, get_lsb(ep));
            ep = 0;
            break;
        }
    }
    // Update castle rights.
    castle &= Castle_rights[src];
    castle &= Castle_rights[dst];

    // Add updated castle rights back into the zobrist key, and swap
    // turns.
    Zobrist::castle(key, castle);
    Zobrist::turn(key);
    assert(!check());
    swap_turn();
}

// ----------------------------------------------------------------------------
// Function to return a bitboard of all valid king moves for the current 
// player
// ----------------------------------------------------------------------------
U64 State::valid_king_moves() const
{
    U64 m;
    const Square * s;

    const Dir L   = us == white ? SW : NW;
    const Dir R   = us == white ? SE : NE;

    // Remove king from occupancy to check squares attacked behind the king.
    const U64 o = occ() ^ piece_bb<king>(us);

    m = King_moves[king_sq(us)];
    m &= ~(shift_e(piece_bb<pawn>(them), R) | shift_w(piece_bb<pawn>(them), L));

    for (s = piece<knight>(them); *s != no_sq; ++s)
        m &= ~(Knight_moves[*s]);

    for (s = piece<bishop>(them); *s != no_sq; ++s)
        m &= ~(Bmagic(*s, o));

    for (s = piece<rook>(them); *s != no_sq; ++s)
        m &= ~(Rmagic(*s, o));

    for (s = piece<queen>(them); *s != no_sq; ++s)
        m &= ~(Qmagic(*s, o));

    m &= ~(King_moves[king_sq(them)]);
    m &= ~(occ(us));

    return m;
}


// ----------------------------------------------------------------------------
// Function to check whether the current player's king is in check. Used 
// mostly for debugging.
// ----------------------------------------------------------------------------
/*
bool State::in_check()
{
    const Square k = piece_list[us][king][0];
    return (pawn_attacks[us][k] & e_pawn())
         | (Knight_moves[k] & e_knight())
         | (Bmagic(k, occ()) & (e_bishop() | e_queen()))
         | (Rmagic(k, occ()) & (e_rook() | e_queen()));
}
*/

// ----------------------------------------------------------------------------
// Function to return the EPD string of the current position, used to query 
// the opening database.
// ----------------------------------------------------------------------------

const char * State::get_EPD() const
{
    std::string EPD = "";
    int empty = 0;
    for (int i = 63; i >= 0; --i)
    {
        if (empty == 8)
        {
            empty = 0;
            EPD.append("8");
        }
        if ((i + 1) % 8 == 0 && i != 63)
        {
            if (empty > 0) EPD.append(std::to_string(empty));
            empty = 0;
            EPD.append("/");
        }
        switch (board[white][i])
        {
            case pawn:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("P");
                break;
            case knight:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("N");
                break;
            case bishop:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("B");
                break;
            case rook:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("R");
                break;
            case queen:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("Q");
                break;
            case king:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("K");
                break;
        }
        switch (board[black][i])
        {
            case none:
                empty++;
                break;
            case pawn:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("p");
                break; 
            case knight:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("n");
                break;     
            case bishop:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("b");
                break;      
            case rook:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("r");
                break;
            case queen:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("q");
                break;
            case king:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("k");
                break;
        }
        if (i == 0 && empty > 0) EPD.append(std::to_string(empty));
    }
    EPD.append(" ");
    us ? EPD.append("b") : EPD.append("w");
    EPD.append(" ");
    if (castle & w_king_castle)  EPD.append("K");
    if (castle & w_queen_castle) EPD.append("Q");
    if (castle & b_king_castle)  EPD.append("k");
    if (castle & b_queen_castle) EPD.append("q");
    EPD.append(" ");
    if (ep != 0)
    {
        int ep_sq = get_lsb(ep);
        ep_sq / 8 == 3 ? ep_sq -= 8 : ep_sq += 8;
        EPD.append(SQ[ep_sq]);
    }
    else
    {
        EPD.append("-");
    }
    return EPD.c_str();
}

// ----------------------------------------------------------------------------
// Function to print the board on the screen. Used for debugging.
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
        if (i % 8 == 7)
            o << nums[i / 8] << "|";

        o << (s.board[white][i] == pawn   ? W_pawn
            : s.board[white][i] == knight ? W_knight
            : s.board[white][i] == bishop ? W_bishop
            : s.board[white][i] == rook   ? W_rook
            : s.board[white][i] == queen  ? W_queen
            : s.board[white][i] == king   ? W_king
            : s.board[black][i] == pawn   ? B_pawn
            : s.board[black][i] == knight ? B_knight
            : s.board[black][i] == bishop ? B_bishop
            : s.board[black][i] == rook   ? B_rook
            : s.board[black][i] == queen  ? B_queen
            : s.board[black][i] == king   ? B_king
            : Empty) << "|";

        if (i % 8 == 0)
            o << '\n' << bar << '\n';
    }

    o << "  A B C D E F G H\n";

    return o;
}
