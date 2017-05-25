#include "state.h"
#include "zobrist.h"

// ----------------------------------------------------------------------------
// Copy constructor.
// ----------------------------------------------------------------------------

State::State(const State & s)
    : fmr(s.fmr), castle(s.castle), en_passant(s.en_passant), 
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
    us =         s.us;
    them =       s.them;
    castle =     s.castle;
    key =        s.key;
    fmr =        s.fmr;
    en_passant = s.en_passant;
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

State::State(std::string & fen) : castle(0), en_passant(0), 
                                  us(WHITE), them(BLACK)
{
    fmr = 0; // NEED TO CHANGE THIS
    U64 bit = 0x8000000000000000ULL;
    int i = 0;
    int position = 0;
    bool set_pieces = true;
    for (PieceType p = PAWN; p < NONE; ++p)
    {
        pieces[WHITE][p]      = 0;
        pieces[BLACK][p]      = 0;
        piece_count[WHITE][p] = 0;
        piece_count[BLACK][p] = 0;
        for (int i = 0; i < PIECE_MAX; ++i)
        {
            piece_list[WHITE][p][i] = NO_SQ;
            piece_list[BLACK][p][i] = NO_SQ;
        }
    }
    for (Square s = H1; s <= A8; ++s)
    {
        board[WHITE][s] = NONE;
        board[BLACK][s] = NONE;
        piece_index[s]  = 0;
    }

    for (i; i < fen.size(); ++i)
    {
        if (!set_pieces) break;
        Square s = Square(63 - position);
        switch (fen[i])
        {
            case 'P':
                pieces[WHITE][PAWN] |= bit >> position;
                piece_list[WHITE][PAWN][piece_count[WHITE][PAWN]] = s;
                piece_index[s] = piece_count[WHITE][PAWN];
                piece_count[WHITE][PAWN]++;
                position++;
                break;
            case 'N':
                pieces[WHITE][KNIGHT] |= bit >> position;
                piece_list[WHITE][KNIGHT][piece_count[WHITE][KNIGHT]] = s;
                piece_index[s] = piece_count[WHITE][KNIGHT];
                piece_count[WHITE][KNIGHT]++;
                position++;
                break;
            case 'B':
                pieces[WHITE][BISHOP] |= bit >> position;
                piece_list[WHITE][BISHOP][piece_count[WHITE][BISHOP]] = s;
                piece_index[s] = piece_count[WHITE][BISHOP];
                piece_count[WHITE][BISHOP]++;
                position++;
                break;
            case 'R':
                pieces[WHITE][ROOK] |= bit >> position;
                piece_list[WHITE][ROOK][piece_count[WHITE][ROOK]] = s;
                piece_index[s] = piece_count[WHITE][ROOK];
                piece_count[WHITE][ROOK]++;
                position++;
                break;
            case 'Q':
                pieces[WHITE][QUEEN] |= bit >> position;
                piece_list[WHITE][QUEEN][piece_count[WHITE][QUEEN]] = s;
                piece_index[s] = piece_count[WHITE][QUEEN];
                piece_count[WHITE][QUEEN]++;
                position++;
                break;
            case 'K':
                pieces[WHITE][KING] |= bit >> position;
                piece_list[WHITE][KING][piece_count[WHITE][KING]] = s;
                piece_index[s] = piece_count[WHITE][KING];
                piece_count[WHITE][KING]++;
                position++;
                break;
            case 'p':
                pieces[BLACK][PAWN] |= bit >> position;
                piece_list[BLACK][PAWN][piece_count[BLACK][PAWN]] = s;
                piece_index[s] = piece_count[BLACK][PAWN];
                piece_count[BLACK][PAWN]++;
                position++;
                break;
            case 'n':
                pieces[BLACK][KNIGHT] |= bit >> position;
                piece_list[BLACK][KNIGHT][piece_count[BLACK][KNIGHT]] = s;
                piece_index[s] = piece_count[BLACK][KNIGHT];
                piece_count[BLACK][KNIGHT]++;
                position++;
                break;
            case 'b':
                pieces[BLACK][BISHOP] |= bit >> position;
                piece_list[BLACK][BISHOP][piece_count[BLACK][BISHOP]] = s;
                piece_index[s] = piece_count[BLACK][BISHOP];
                piece_count[BLACK][BISHOP]++;
                position++;
                break;
            case 'r':
                pieces[BLACK][ROOK] |= bit >> position;
                piece_list[BLACK][ROOK][piece_count[BLACK][ROOK]] = s;
                piece_index[s] = piece_count[BLACK][ROOK];
                piece_count[BLACK][ROOK]++;
                position++;
                break;
            case 'q':
                pieces[BLACK][QUEEN] |= bit >> position;
                piece_list[BLACK][QUEEN][piece_count[BLACK][QUEEN]] = s;
                piece_index[s] = piece_count[BLACK][QUEEN];
                piece_count[BLACK][QUEEN]++;
                position++;
                break;
            case 'k':
                pieces[BLACK][KING] |= bit >> position;
                piece_list[BLACK][KING][piece_count[BLACK][KING]] = s;
                piece_index[s] = piece_count[BLACK][KING];
                piece_count[BLACK][KING]++;
                position++;
                break;
            case '1':
                position += 1;
                break;
            case '2':
                position += 2;
                break;
            case '3':
                position += 3;
                break;
            case '4':
                position += 4;
                break;
            case '5':
                position += 5;
                break;
            case '6':
                position += 6;
                break;
            case '7':
                position += 7;
                break;
            case '8':
                position += 8;
                break;
            case ' ':
                set_pieces = false;
                break;
        }
    }

    if (fen[i] == 'w') 
    {
        us   = WHITE;
        them = BLACK;
    }
    else if (fen[i] == 'b') 
    {
        us   = BLACK;
        them = WHITE;
    }
    i += 2;

    occupancy[WHITE] = pieces[WHITE][PAWN]   | pieces[WHITE][KNIGHT]
                     | pieces[WHITE][BISHOP] | pieces[WHITE][ROOK]
                     | pieces[WHITE][QUEEN]  | pieces[WHITE][KING];
    occupancy[BLACK] = pieces[BLACK][PAWN]   | pieces[BLACK][KNIGHT]
                     | pieces[BLACK][BISHOP] | pieces[BLACK][ROOK]
                     | pieces[BLACK][QUEEN]  | pieces[BLACK][KING];

    int fen_table[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    int ep = -1;
    for (i; i < fen.size(); ++i)
    {
        switch (fen[i])
        {   
            case 'K':
                castle += 1;
                break;
            case 'Q':
                castle += 2;
                break;
            case 'k':
                castle += 4;
                break;
            case 'q':
                castle += 8;
                break;
            case 'a':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 7;
                break;
            case 'b':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 6;
                break;
            case 'c':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 5;
                break;
            case 'd':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 4;
                break;
            case 'e':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 3;
                break;
            case 'f':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 2;
                break;
            case 'g':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 1;
                break;
            case 'h':
                ep = fen_table[int(fen[i + 1]) - 49] * 8 + 0;
                break;
        }
        if (ep > -1)
        {
            i++;
            if (ep / 8 == 2)
            {
                ep += 8;
            }
            else
            {
                ep -= 8;
            }
            en_passant = 1ULL << ep;
        }
    }

    for (Square s = H1; s <= A8; ++s)
    {
        U64 bit = 1ULL << s;

        if (occ() & bit)
        {
            const Color c = occupancy[WHITE] & bit ? WHITE : BLACK;
            if (pieces[c][PAWN] & bit)
                board[c][s] = PAWN;
            else if (pieces[c][KNIGHT] & bit)
                board[c][s] = KNIGHT;
            else if (pieces[c][BISHOP] & bit)
                board[c][s] = BISHOP;
            else if (pieces[c][ROOK] & bit)
                board[c][s] = ROOK;
            else if (pieces[c][QUEEN] & bit)
                board[c][s] = QUEEN;
            else if (pieces[c][KING] & bit)
                board[c][s] = KING;
            else
                assert(false);
        }
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
    for (i = 0; i < piece_count[them][BISHOP]; ++i)
    {
        ray = between_dia[piece_list[them][BISHOP][i]][p_king_sq()];
        if (pop_count(ray & occ()) == 1 && ray & p_occ())
            pin |= ray & p_occ();
    }
    for (i = 0; i < piece_count[them][ROOK]; ++i)
    {
        ray = between_hor[piece_list[them][ROOK][i]][p_king_sq()];
        if (pop_count(ray & occ()) == 1 && ray & p_occ())
            pin |= ray & p_occ();
    }
    for (i = 0; i < piece_count[them][QUEEN]; ++i)
    {
        ray = between_dia[piece_list[them][QUEEN][i]][p_king_sq()]
            | between_hor[piece_list[them][QUEEN][i]][p_king_sq()];
        if (pop_count(ray & occ()) == 1 && ray & p_occ())
            pin |= ray & p_occ();
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
    board[us][src] == PAWN || board[them][dst] != NONE ? 
        fmr = 0 : fmr++;

    // Remove the ep file and castle rights from the zobrist key.
    if (en_passant) Zobrist::ep(key, en_passant);
    Zobrist::castle(key, castle);

    switch (get_prop(m))
    {
        // Quiet moves.
        case QUIET:
        {
            Zobrist::move(this, src, dst);
            move_piece(src, dst);
            en_passant = 0;
            break;
        }
        // Attacking moves.
        case ATTACK:
        {
            Zobrist::move(this, src, dst);
            del_piece(them, dst);
            move_piece(src, dst);
            en_passant = 0;
            break;
        }
        // Double pawn push.
        case DBL_PUSH:
        {
            Zobrist::move(this, src, dst);
            move_piece(src, dst);
            en_passant = 1ULL << dst;
            Zobrist::ep(key, en_passant);
            break;
        }
        // King castle.
        case KING_CAST:
        {
            const int rook_src = src-3;
            const int rook_dst = dst+1;
            Zobrist::move(this, rook_src, rook_dst);
            Zobrist::move(this, src, dst);
            move_piece(rook_src, rook_dst);
            move_piece(src, dst);
            en_passant = 0;
            break;
        }
        // Queen castle.
        case QUEEN_CAST:
        {
            const int rook_src = src+4;
            const int rook_dst = dst-1;
            Zobrist::move(this, rook_src, rook_dst);
            Zobrist::move(this, src, dst);
            move_piece(rook_src, rook_dst);
            move_piece(src, dst);
            en_passant = 0;
            break;
        }
        // Queen promotion.
        case QUEEN_PROMO:
        {
            Zobrist::promo(this, src, dst, QUEEN);
            del_piece(us, src);
            add_piece(dst, QUEEN);
            en_passant = 0;
            break;
        }
        // Knight underpromotion.
        case KNIGHT_PROMO:
        {
            Zobrist::promo(this, src, dst, KNIGHT);
            del_piece(us, src);
            add_piece(dst, KNIGHT);
            en_passant = 0;
            break;
        }
        // Rook underpromotion.
        case ROOK_PROMO:
        {
            Zobrist::promo(this, src, dst, ROOK);
            del_piece(us, src);
            add_piece(dst, ROOK);
            en_passant = 0;
            break;
        }
        // Bishop underpromotion.
        case BISHOP_PROMO:
        {
            Zobrist::promo(this, src, dst, BISHOP);
            del_piece(us, src);
            add_piece(dst, BISHOP);
            en_passant = 0;
            break;
        }
        // En-Passant.
        case EN_PASSANT:
        {
            Zobrist::en_passant(this, src, dst, get_lsb(en_passant));
            move_piece(src, dst);
            del_piece(them, get_lsb(en_passant));
            en_passant = 0;
            break;
        }
    }
    // Update castle rights.
    castle &= CASTLE_RIGHTS[src];
    castle &= CASTLE_RIGHTS[dst];

    // Add updated castle rights back into the zobrist key, and swap
    // turns.
    Zobrist::castle(key, castle);
    Zobrist::turn(key);
    assert(!in_check());
    swap_turn();
}

// ----------------------------------------------------------------------------
// Function to return a bitboard of all valid king moves for the current 
// player
// ----------------------------------------------------------------------------
U64 State::valid_king_moves() const
{
    U64 m;
    int i;

    const Dir L   = us == WHITE ? SW : NW;
    const Dir R   = us == WHITE ? SE : NE;

    // Remove king from occupancy to check squares attacked behind the king.
    const U64 o = (occupancy[us] | occupancy[them]) ^ pieces[us][KING];

    m = king_moves[piece_list[us][KING][0]];

    m &= ~(shift_e(pieces[them][PAWN], R) | shift_w(pieces[them][PAWN], L));

    for (i = 0; i < piece_count[them][KNIGHT]; ++i)
        m &= ~(knight_moves[piece_list[them][KNIGHT][i]]);

    for (i = 0; i < piece_count[them][BISHOP]; ++i)
        m &= ~(Bmagic(piece_list[them][BISHOP][i], o));

    for (i = 0; i < piece_count[them][ROOK]; ++i)
        m &= ~(Rmagic(piece_list[them][ROOK][i], o));

    for (i = 0; i < piece_count[them][QUEEN]; ++i)
        m &= ~(Qmagic(piece_list[them][QUEEN][i], o));

    m &= ~(king_moves[piece_list[them][KING][0]]);
    m &= ~(occupancy[us]);

    return m;
}


// ----------------------------------------------------------------------------
// Function to check whether the current player's king is in check. Used 
// mostly for debugging.
// ----------------------------------------------------------------------------

bool State::in_check()
{
    const Square k = piece_list[us][KING][0];
    return (pawn_attacks[us][k] & e_pawn())
         | (knight_moves[k] & e_knight())
         | (Bmagic(k, occ()) & (e_bishop() | e_queen()))
         | (Rmagic(k, occ()) & (e_rook() | e_queen()));
}

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
        switch (board[WHITE][i])
        {
            case PAWN:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("P");
                break;
            case KNIGHT:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("N");
                break;
            case BISHOP:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("B");
                break;
            case ROOK:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("R");
                break;
            case QUEEN:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("Q");
                break;
            case KING:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("K");
                break;
        }
        switch (board[BLACK][i])
        {
            case NONE:
                empty++;
                break;
            case PAWN:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("p");
                break; 
            case KNIGHT:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("n");
                break;     
            case BISHOP:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("b");
                break;      
            case ROOK:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("r");
                break;
            case QUEEN:
                if (empty > 0) EPD.append(std::to_string(empty));
                empty = 0;
                EPD.append("q");
                break;
            case KING:
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
    if (castle & W_KING_CASTLE)  EPD.append("K");
    if (castle & W_QUEEN_CASTLE) EPD.append("Q");
    if (castle & B_KING_CASTLE)  EPD.append("k");
    if (castle & B_QUEEN_CASTLE) EPD.append("q");
    EPD.append(" ");
    if (en_passant != 0)
    {
        int ep_sq = get_lsb(en_passant);
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
    const char * W_PAWN   = "\u2659";
    const char * W_KNIGHT = "\u2658";
    const char * W_BISHOP = "\u2657";
    const char * W_ROOK   = "\u2656";
    const char * W_QUEEN  = "\u2655";
    const char * W_KING   = "\u2654";
    const char * B_PAWN   = "\u265F";
    const char * B_KNIGHT = "\u265E";
    const char * B_BISHOP = "\u265D";
    const char * B_ROOK   = "\u265C";
    const char * B_QUEEN  = "\u265B";
    const char * B_KING   = "\u265A";
    const char * EMPTY    = " ";

    std::string nums[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};
    const std::string bar = " +-+-+-+-+-+-+-+-+";

    o << bar << std::endl;
    for (int i = 63; i >= 0; --i)
    {
        if (i % 8 == 7)
            o << nums[i / 8] << "|";

        o << (s.board[WHITE][i] == PAWN   ? W_PAWN
            : s.board[WHITE][i] == KNIGHT ? W_KNIGHT
            : s.board[WHITE][i] == BISHOP ? W_BISHOP
            : s.board[WHITE][i] == ROOK   ? W_ROOK
            : s.board[WHITE][i] == QUEEN  ? W_QUEEN
            : s.board[WHITE][i] == KING   ? W_KING
            : s.board[BLACK][i] == PAWN   ? B_PAWN
            : s.board[BLACK][i] == KNIGHT ? B_KNIGHT
            : s.board[BLACK][i] == BISHOP ? B_BISHOP
            : s.board[BLACK][i] == ROOK   ? B_ROOK
            : s.board[BLACK][i] == QUEEN  ? B_QUEEN
            : s.board[BLACK][i] == KING   ? B_KING
            : EMPTY) << "|";

        if (i % 8 == 0)
            o << '\n' << bar << '\n';
    }

    o << "  A B C D E F G H\n";

    return o;
}
