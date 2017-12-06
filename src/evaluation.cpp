#include "evaluation.h"

std::array<PawnEntry, hash_size> pawnHash;

void init_eval()
{
    std::fill(pawnHash.begin(), pawnHash.end(), PawnEntry());
}

// Game phase scaling based on piece count.
float getGamePhase(const State& s)
{
    float phase;

    phase = totalPhase 
          - s.getPieceCount<pawn>()   * pawnPhase
          - s.getPieceCount<knight>() * knightPhase
          - s.getPieceCount<bishop>() * bishopPhase
          - s.getPieceCount<rook>()   * rookPhase
          - s.getPieceCount<queen>()  * queenPhase;

    return (phase * 256 + (totalPhase / 2)) / totalPhase;
}

// Get the scaled PST score for all pieces based on the current game phase.
float scaledPstScore(const State& s)
{
    float gamePhase = getGamePhase(s);
    return ((s.getPstScore(middle) * (256 - gamePhase)) + (s.getPstScore(late) * gamePhase)) / 256;
}

// Returns the score of a bishop or rook on an outpost square.
template<PieceType PT>
int outpost(const State & s, Square p, Color c)
{
    int score;
    // To be an outpost, the piece must be supported by a friendly pawn
    // and unable to be attacked by an opponents pawn.
    if (   !(p & outpost_area[c])
        || !(pawn_attacks[!c][p] & s.getPieceBB<pawn>(c))
        || in_front[c][p] & adj_files[p] & s.getPieceBB<pawn>(!c))
        return 0;

    score = PieceSquareTable::outpost[PT == bishop][c][p];

    // Extra bonus if the outpost cannot be captured by a minor piece.
    if (   !s.getPieceBB<knight>(!c)
        && !(s.getPieceBB<bishop>(!c) & squares_of_color(p)))
        score *= 2;

    return score;
}

// Check to see if a pawn push would fork two of the enemy major or minor
// pieces. Also includes forks involving the enemy's king.
bool pawn_fork(const State & s, Square p, Color c)
{
    assert(p < 56 && p > 7);
    Square push;
    
    push = p + (c == white ? 8 : -8);
    
    // Check to see if the push square is not occupied, if the pawn is not on
    // the edge of the board, and if the push would fork two enemy non-pawn 
    // pieces.
    if (   push & s.getOccupancyBB()
        || !(push & Center_files)
        || pawn_attacks[c][push] & (s.getPieceBB<pawn>(!c) | s.getEmptyBB() | s.getOccupancyBB(c)))
        return false;

    // Check to see if the pawn is pinned.
    if (s.check(square_bb[p], c))
        return false;

    // Return true if the push square is defended.
    return s.defended(push, c);
}

int eval_pawns(const State & s, const Color c)
{
    const int dir = c == white ? 8 : -8;
    int score = Draw;

    // Pawn evaluation.
    for (Square p : s.getPieceList<pawn>(c))
    {
        if (p == no_sq)
            break;
        score += Pawn_wt;
        // Check if the pawn is a passed pawn.
        if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & s.getPieceBB<pawn>(!c)))
            score += Passed;
        // Look for candidate and backwards pawns since the logic is related.
        // First check for a half open file.
        else if (!(file_bb[p] & in_front[c][p] & s.getPieceBB<pawn>(!c)))
        {
            int sentries, helpers;
            assert(p < 56 && p > 7);
            // Check, if this pawn were to push, if there are any enemy pawns
            // defending that square.
            sentries = pop_count(pawn_attacks[c][p + dir] & s.getPieceBB<pawn>(!c));
            if (sentries > 0)
            {
                // If the enemy has defenders, check if we helper pawns defending
                // that square.
                helpers = pop_count(pawn_attacks[!c][p + dir] & s.getPieceBB<pawn>(c));
                // If there are more helpers then sentries, we have a candidate
                // passer.
                if (helpers >= sentries)
                    score += Candidate;
                // If there are less helpers then sentries, check for a backwards
                // pawn. First, see if there are no pawns eligible to defend it.
                else if (!(~in_front[c][p] & adj_files[p] & s.getPieceBB<pawn>(c)))
                {
                    // Make sure the backwards pawn is defending at least 1 pawn.
                    if (pawn_attacks[c][p] & s.getPieceBB<pawn>(c))
                    {
                        // If there are two sentries, this backwards pawn is
                        // nearly imposible to push.
                        if (sentries == 2)
                            score += Full_backwards;
                        else
                            score += Backwards;
                    }
                }
            }
        }

        // Check if the pawn is isolated
        if (!(adj_files[p] & s.getPieceBB<pawn>(c)))
            score += Isolated;
        // Check if the pawn is connected
        else if (rank_bb[p - dir] & s.getPieceBB<pawn>(c) & adj_files[p])
            score += Connected;

        // Check if the pawn is doubled.
        if (pop_count(file_bb[p] & s.getPieceBB<pawn>(c)) > 1)
            score += Doubled;
    }
    return score;
}

int eval(const State & s, const Color c)
{
    U64 moves, pins;
    U64 mobilityNet;
    int score = Draw;
    int king_threats = 0;
    Square kingSq = s.getKingSquare(c);

    // The moveNet is all empty squares not attacked by enemy pawns.
    mobilityNet = s.getEmptyBB();
    if (c == white)
        mobilityNet &= ~((s.getPieceBB<pawn>(black) & Not_a_file) >> 7
                       | (s.getPieceBB<pawn>(black) & Not_h_file) >> 9);
    else
        mobilityNet &= ~((s.getPieceBB<pawn>(white) & Not_a_file) << 9
                       | (s.getPieceBB<pawn>(white) & Not_h_file) << 7);

    // Get the pinned pieces for the current player.
    pins = s.getPins(c);

    // Knight evaluation.
    for (Square p : s.getPieceList<knight>(c))
    {
        if (p == no_sq)
            break;
        score += Knight_wt;
        if (s.getAttackBB<knight>(p) & king_net_bb[!c][s.getKingSquare(!c)])
            king_threats += Knight_th;
        score += outpost<knight>(s, p, c);

        // Calculate knight mobility.
        if (square_bb[p] & pins)
            score += knightMobility[0];
        else
        {
            moves = s.getAttackBB<knight>(p) & mobilityNet;
            score += knightMobility[pop_count(moves)];
        }
    }

    // Bishop evaluation.
    for (Square p : s.getPieceList<bishop>(c))
    {
        if (p == no_sq)
            break;
        score += Bishop_wt;
        if (s.getAttackBB<bishop>(p) & king_net_bb[!c][s.getKingSquare(!c)])
            king_threats += Bishop_th;
        score += outpost<bishop>(s, p, c);

        // Calculate bishop mobility
        moves = s.getAttackBB<bishop>(p) & mobilityNet;
        if (square_bb[p] & pins)
            moves &= between_dia[p][kingSq];
        score += bishopMobility[pop_count(moves)];
    }

    // Rook evaluation.
    for (Square p : s.getPieceList<rook>(c))
    {
        if (p == no_sq)
            break;
        score += Rook_wt;
        if (s.getAttackBB<rook>(p) & king_net_bb[!c][s.getKingSquare(!c)])
            king_threats += Rook_th;

        // Calculate rook mobility.
        moves = s.getAttackBB<rook>(p) & mobilityNet;
        if (square_bb[p] & pins)
            moves &= between_hor[p][kingSq];
        score += rookMobility[pop_count(moves)];
    }

    // Queen evaluation.
    for (Square p : s.getPieceList<queen>(c))
    {
        if (p == no_sq)
            break;
        score += Queen_wt;
        if (s.getAttackBB<queen>(p) & king_net_bb[!c][s.getKingSquare(!c)])
            king_threats += Queen_th;

        // Calculate queen mobility.
        moves = s.getAttackBB<queen>(p) & mobilityNet;
        if (square_bb[p] & pins)
        {
            moves &= between_hor[p][kingSq] ? between_hor[p][kingSq]
                                            : between_dia[p][kingSq];
        }
        score += queenMobility[pop_count(moves)];
    }

    // King evaluation.
    score += Safety_table[king_threats];

    return score;
}

int evaluate(const State & s)
{
    int finalScore;
    int pawnScore = 0;
    PawnEntry* pawnEntry;

    finalScore = scaledPstScore(s);

    // Pawn Evaluation
    if (s.getPieceCount<pawn>(s.getOurColor()) + s.getPieceCount<pawn>(s.getTheirColor()))
    {
        //Probe the pawn hash.
        pawnEntry = probe(s.getPawnKey());
        if (pawnEntry && pawnEntry->mKey == s.getPawnKey())
        {
            pawnScore = (pawnEntry->mColor == s.getOurColor()) ?  pawnEntry->mScore 
                                                    : -pawnEntry->mScore;
        }
        else
        {
            pawnScore = eval_pawns(s, s.getOurColor()) - eval_pawns(s, s.getTheirColor());
            store(s.getPawnKey(), pawnScore, s.getOurColor());
        }
    }
    finalScore += pawnScore;

    finalScore += eval(s, s.getOurColor()) - eval(s, s.getTheirColor()) + tempo;

	return finalScore;
}