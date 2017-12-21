#include "evaluation.h"

// -------------------------------------------------------------------------- //
//                                                                            //
// Constructor for the Evaluation object. This object will take a state and   //
// calculate the score in centipawns from the perspective of the side to      //
// move.                                                                      //
//                                                                            //
// -------------------------------------------------------------------------- //
Evaluate::Evaluate(const State& pState)
: mState(pState)
, mMaterial{}
, mPawnStructure{}
, mMobility{}
, mKingSafety{}
, mAttacks{}
, mPieceAttacksBB{}
, mAllAttacksBB{}
{

// -------------------------------------------------------------------------- //
//                                                                            //
// If any pawns are on the board, check if there is a valid entry in the      //
// pawn hash table. The pawn key for the current state is used to check if    //
// the value in the hash table matches the current state. If a pawn is        //
// neither moved or captured, we can use some of the same evaluation          //
// information. If a valid is found, we can use calculations for:             //
//   1. Pawn Structure                                                        //
//   2. Pawn Material                                                         //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (mState.getPieceCount<pawn>())
    {
        const PawnEntry* pawnEntry = probe(mState.getPawnKey());
        if (pawnEntry && pawnEntry->getKey() == mState.getPawnKey())
        {
            mPawnStructure = pawnEntry->getStructure();
            mMaterial = pawnEntry->getMaterial();
        }
        else
        {
// -------------------------------------------------------------------------- //
//                                                                            //
// If no pawn entry is found in the hash table, evaluate pawns for both       // 
// players and store the result as a new entry in the hash table.             //
//                                                                            //
// -------------------------------------------------------------------------- //
            evalPawns(white);
            evalPawns(black);
            store(mState.getPawnKey(), mPawnStructure, mMaterial);
        }
    }

// -------------------------------------------------------------------------- //
//                                                                            //
// For evaluation concepts that have different weights for mid-game and       //
// end-game (such as PST), a game phase is used to interpolate these values   //
// and avoid any evaluation discontinuity. This is also known as a tapered    //
// evaluation. The game phase is between 0 - 255, closer to 0 puts more       //
// weight on the mid-game, closer to 255 puts more weight on the endgame.     //
//                                                                            //
// -------------------------------------------------------------------------- //
    float phase = totalPhase
                - mState.getPieceCount<pawn>()   * pawnPhase
                - mState.getPieceCount<knight>() * knightPhase
                - mState.getPieceCount<bishop>() * bishopPhase
                - mState.getPieceCount<rook>()   * rookPhase
                - mState.getPieceCount<queen>()  * queenPhase;

    mGamePhase = (phase * 256 + (totalPhase / 2)) / totalPhase;

    evalPieces(white);
    evalPieces(black);

// -------------------------------------------------------------------------- //
//                                                                            //
// Add pawn attacks to the attacks bitboards. These can't be stored in the    //
// pawn hash, since the same configuration of pawns could give different      //
// attack sets depening on the location of non-pawn pieces.                   //
//                                                                            //
// -------------------------------------------------------------------------- //
    mPieceAttacksBB[white][pawn] |= (mState.getPieceBB<pawn>(white) & Not_a_file) << 9
        & mState.getOccupancyBB();
    mPieceAttacksBB[white][pawn] |= (mState.getPieceBB<pawn>(white) & Not_h_file) << 7
        & mState.getOccupancyBB();
    mPieceAttacksBB[black][pawn] |= (mState.getPieceBB<pawn>(black) & Not_a_file) >> 7
        & mState.getOccupancyBB();
    mPieceAttacksBB[black][pawn] |= (mState.getPieceBB<pawn>(black) & Not_h_file) >> 9
        & mState.getOccupancyBB();
    mAllAttacksBB[white] |= mPieceAttacksBB[white][pawn];
    mAllAttacksBB[black] |= mPieceAttacksBB[black][pawn];

    evalAttacks(white);
    evalAttacks(black);

    Color c = mState.getOurColor();
    mScore = mMobility[c]      - mMobility[!c]
          + mKingSafety[c]    - mKingSafety[!c]
          + mPawnStructure[c] - mPawnStructure[!c]
          + mMaterial[c]      - mMaterial[!c];

    mScore += ((mState.getPstScore(middle) * (256 - mGamePhase))
           + mState.getPstScore(late) * mGamePhase) / 256;

// -------------------------------------------------------------------------- //
//                                                                            //
// Tempo is added to the score to give the side to move a slight advantage.   //
//                                                                            //
// -------------------------------------------------------------------------- //
    mScore += tempo;
}

int Evaluate::getScore() const
{
    return mScore;
}

template<PieceType PT>
int Evaluate::outpost(Square p, Color c)
{
    int score;
    // To be an outpost, the piece must be supported by a friendly pawn
    // and unable to be attacked by an opponents pawn.
    if (   !(p & outpost_area[c])
        || !(pawn_attacks[!c][p] & mState.getPieceBB<pawn>(c))
        || in_front[c][p] & adj_files[p] & mState.getPieceBB<pawn>(!c))
        return 0;

    score = PieceSquareTable::outpost[PT == bishop][c][p];

    // Extra bonus if the outpost cannot be captured by a minor piece.
    if (   !mState.getPieceBB<knight>(!c)
        && !(mState.getPieceBB<bishop>(!c) & squares_of_color(p)))
        score *= 2;

    return score;
}

void Evaluate::evalPawns(const Color c)
{
    const int dir = c == white ? 8 : -8;

    // Pawn evaluation.
    for (Square p : mState.getPieceList<pawn>(c))
    {
        if (p == no_sq)
            break;

        mMaterial[c] += Pawn_wt;
        // Check if the pawn is a passed pawn.
        if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & mState.getPieceBB<pawn>(!c)))
            mPawnStructure[c] += Passed;
        // Look for candidate and backwards pawns since the logic is related.
        // First check for a half open file.
        else if (!(file_bb[p] & in_front[c][p] & mState.getPieceBB<pawn>(!c)))
        {
            int sentries, helpers;
            assert(p < 56 && p > 7);
            // Check, if this pawn were to push, if there are any enemy pawns
            // defending that square.
            sentries = pop_count(pawn_attacks[c][p + dir] & mState.getPieceBB<pawn>(!c));
            if (sentries > 0)
            {
                // If the enemy has defenders, check if we helper pawns defending
                // that square.
                helpers = pop_count(pawn_attacks[!c][p + dir] & mState.getPieceBB<pawn>(c));
                // If there are more helpers then sentries, we have a candidate
                // passer.
                if (helpers >= sentries)
                    mPawnStructure[c] += Candidate;
                // If there are less helpers then sentries, check for a backwards
                // pawn. First, see if there are no pawns eligible to defend it.
                else if (!(~in_front[c][p] & adj_files[p] & mState.getPieceBB<pawn>(c)))
                {
                    // Make sure the backwards pawn is defending at least 1 pawn.
                    if (pawn_attacks[c][p] & mState.getPieceBB<pawn>(c))
                    {
                        // If there are two sentries, this backwards pawn is
                        // nearly imposible to push.
                        if (sentries == 2)
                            mPawnStructure[c] += Full_backwards;
                        else
                            mPawnStructure[c] += Backwards;
                    }
                }
            }
        }

        // Check if the pawn is isolated
        if (!(adj_files[p] & mState.getPieceBB<pawn>(c)))
            mPawnStructure[c] += Isolated;
        // Check if the pawn is connected
        else if (rank_bb[p - dir] & mState.getPieceBB<pawn>(c) & adj_files[p])
            mPawnStructure[c] += Connected;

        // Check if the pawn is doubled.
        if (pop_count(file_bb[p] & mState.getPieceBB<pawn>(c)) > 1)
            mPawnStructure[c] += Doubled;
    }
}

void Evaluate::evalPieces(const Color c)
{
    U64 moves, pins;
    U64 mobilityNet;
    int king_threats = 0;
    U64 bottomRank = c == white ? Rank_1 : Rank_8;
    Square kingSq = mState.getKingSquare(c);

    // The mobilityNet is all empty squares not attacked by enemy pawns.
    mobilityNet = mState.getEmptyBB();
    if (c == white)
        mobilityNet &= ~((mState.getPieceBB<pawn>(black) & Not_a_file) >> 7
                       | (mState.getPieceBB<pawn>(black) & Not_h_file) >> 9);
    else
        mobilityNet &= ~((mState.getPieceBB<pawn>(white) & Not_a_file) << 9
                       | (mState.getPieceBB<pawn>(white) & Not_h_file) << 7);

    // Get the pinned pieces for the current player.
    pins = mState.getPinsBB(c);

    // Knight evaluation.
    for (Square p : mState.getPieceList<knight>(c))
    {
        if (p == no_sq)
            break;
        mMaterial[c] += Knight_wt;
        mMaterial[c] += outpost<knight>(p, c);

        if (square_bb[p] & pins)
        {
            mMobility[c] += knightMobility[0];
            continue;
        }

        moves = mState.getAttackBB<knight>(p);
        mPieceAttacksBB[c][knight] |= moves & mState.getOccupancyBB();
        mAllAttacksBB[c] |= mPieceAttacksBB[c][knight];

        mMobility[c] += knightMobility[pop_count(moves & mobilityNet)];

        if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet)
            king_threats += Knight_th;
    }

    // Bishop evaluation.
    for (Square p : mState.getPieceList<bishop>(c))
    {
        if (p == no_sq)
            break;
        mMaterial[c] += Bishop_wt;
        mMaterial[c] += outpost<bishop>(p, c);

        moves = mState.getAttackBB<bishop>(p);

        if (square_bb[p] & pins)
            moves &= coplanar[p][kingSq];

        mPieceAttacksBB[c][bishop] = moves & mState.getOccupancyBB();
        mAllAttacksBB[c] |= mPieceAttacksBB[c][bishop];

        if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet)
            king_threats += Bishop_th;

        mMobility[c] += bishopMobility[pop_count(moves & mobilityNet)];

        // Check for a bad bishop (penalty for pawns on the same square);
        mMaterial[c] += BadBishop * 
            pop_count(squares_of_color(p) & mState.getPieceBB<pawn>(c));
    }

    // Rook evaluation.
    for (Square p : mState.getPieceList<rook>(c))
    {
        if (p == no_sq)
            break;
        mMaterial[c] += Rook_wt;

        moves = mState.getAttackBB<rook>(p);
        if (square_bb[p] & pins)
            moves &= coplanar[p][kingSq];

        mPieceAttacksBB[c][rook] = moves & mState.getOccupancyBB();
        mAllAttacksBB[c] |= mPieceAttacksBB[c][rook];

        if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet)
            king_threats += Rook_th;

        mMobility[c] += rookMobility[pop_count(moves & mobilityNet)];

        // Check if the rook is trapped.
        // TODO: more testing to confirm this is working propertly.
        if (mState.getPieceBB<king>(c) & bottomRank 
            && square_bb[p] & bottomRank)
        {
            if ((kingSq > p && !mState.canCastleKingside(c) &&
                square_bb[kingSq] & Rightside) ||
                (kingSq < p && !mState.canCastleQueenside(c) &&
                square_bb[kingSq] & Leftside)) 
            {
                if (pop_count(moves & mobilityNet & ~(bottomRank)) <= 3)
                    mMaterial[c] += TrappedRook;
            }
        }
    }

    // Queen evaluation.
    for (Square p : mState.getPieceList<queen>(c))
    {
        if (p == no_sq)
            break;
        mMaterial[c] += Queen_wt;
        moves = mState.getAttackBB<queen>(p);
        if (square_bb[p] & pins)
            moves &= coplanar[p][kingSq];

        mPieceAttacksBB[c][queen] = moves & mState.getOccupancyBB();
        mAllAttacksBB[c] |= mPieceAttacksBB[c][queen];

        if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet)
            king_threats += Queen_th;

        // Calculate queen mobility.
        mMobility[c] += queenMobility[pop_count(moves & mobilityNet)];
    }

    // King evaluation.
    mKingSafety[!c] -= Safety_table[king_threats];
}

void Evaluate::evalAttacks(Color c)
{
    U64 attackedByPawn, hanging;
    // First check all enemy pieces attacked by a pawn.
    attackedByPawn = mPieceAttacksBB[!c][pawn] 
        & (mState.getOccupancyBB(c) ^ mState.getPieceBB<pawn>(c));

    while (attackedByPawn)
    {
        Square to = pop_lsb(attackedByPawn);
        if (mState.getAttackBB<pawn>(to, c) 
            & mState.getPieceBB<pawn>(!c) 
            & mAllAttacksBB[!c])
            mAttacks[c] += StrongPawnAttack;
        else
            mAttacks[c] += WeakPawnAttack;
    }

    // Check for hanging pieces - pieces that are attacked but not 
    // defended.
    hanging = mAllAttacksBB[!c] & mState.getOccupancyBB(c) 
            & ~mAllAttacksBB[c];

    mAttacks[c] += pop_count(hanging) * Hanging;
}

std::ostream& operator<<(std::ostream& o, const Evaluate& e)
{
    Color c = e.mState.getOurColor();
    std::string us = c == white ? "White" : "Black";
    std::string them = c == white ? "Black" : "White";
    int pstMid = e.mState.getPstScore(middle) * (256 - e.mGamePhase) / 256;
    int pstLate = e.mState.getPstScore(late) * e.mGamePhase / 256;

    o << e.mState
      << "-------------------------------------------------------------\n"
      << "| Evaluation Type |    " << us << "    |    " << them 
      << "    | Total       |\n"
      << "-------------------------------------------------------------\n"
      << "| Material        |"
      << std::setw(13) << e.mMaterial[ c] << "|"
      << std::setw(13) << e.mMaterial[!c] << "|"
      << std::setw(13) << e.mMaterial[ c] - e.mMaterial[!c] << "|\n"
      << "-------------------------------------------------------------\n"
      << "| Mobility        |" 
      << std::setw(13) << e.mMobility[ c] << "|" 
      << std::setw(13) << e.mMobility[!c] << "|"
      << std::setw(13) << e.mMobility[ c] - e.mMobility[!c] << "|\n"
      << "-------------------------------------------------------------\n"
      << "| Pawn Structure  |"
      << std::setw(13) << e.mPawnStructure[ c] << "|" 
      << std::setw(13) << e.mPawnStructure[!c] << "|"
      << std::setw(13) << e.mPawnStructure[ c] - e.mPawnStructure[!c] 
      << "|\n"
      << "-------------------------------------------------------------\n"
      << "| King Safety     |"
      << std::setw(13) << e.mKingSafety[ c] << "|" 
      << std::setw(13) << e.mKingSafety[!c] << "|"
      << std::setw(13) << e.mKingSafety[ c] - e.mKingSafety[!c] << "|\n"
      << "-------------------------------------------------------------\n"
      << "| Attacks         |"
      << std::setw(13) << e.mAttacks[ c] << "|" 
      << std::setw(13) << e.mAttacks[!c] << "|"
      << std::setw(13) << e.mAttacks[ c] - e.mAttacks[!c] << "|\n"
      << "-------------------------------------------------------------\n"
      << "| PST (mid/late)  |"
      << std::setw(13) << pstMid << "|" 
      << std::setw(13) << pstLate << "|"
      << std::setw(13) << pstMid + pstLate << "|\n"
      << "-------------------------------------------------------------\n"
      << "| Total           |             |             |"
      << std::setw(13) << e.mScore << "|\n"
      << "-------------------------------------------------------------\n";


      return o;
}