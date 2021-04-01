#include "bitboards.h"
#include "evaluation.h"
#include "material.h"

const int PAWN_SCORES[32] = {
      0,   0,   0,   0,
     20,  10, -20, -30,
     10,  10,  10,   0,
      0,   0,  10,  30,
     10,  10,  10,  20,
     15,  15,  20,  30,
     40,  40,  40,  40,
      0,   0,   0,   0
};

const int KNIGHT_SCORES[32] = {
    -40, -20, -10, -10,
    -20,  -5,   0,   0,
    -10,   0,  10,  10,
    -10,   0,  10,  20,
    -10,   0,  10,  20,
    -10,   0,  10,  10,
    -20,  -5,   0,   0,
    -40, -20, -10, -10,    
};

const int BISHOP_SCORES[32] = {
    -30, -20, -10,   0,
    -20, -10,   0,  10,
    -10,   0,  10,  20,
      0,  10,  20,  30,
      0,  10,  20,  30,
    -10,   0,  10,  20,
    -20, -10,   0,  10,
    -30, -20, -10,   0,  
};


const int ROOK_SCORES[32] = {
    -10,   0,  10,  20,
    -10,   0,  10,  20,
      0,   0,   0,  20,
      0,   0,   0,  20,
      0,   0,   0,  20,
      0,   0,   0,  20,
     25,  25,  25,  25,
      0,   0,   0,  20,
};

const int KING_SCORES[32] = {
     10,  30,   0, -30,
      5,  10, -10, -50,
    -40, -40, -40, -40,
    -40, -40, -40, -40,
    -40, -40, -40, -40,
    -40, -40, -40, -40,
    -40, -40, -40, -40,
    -40, -40, -40, -40,
};

const int WHITE_MAPPER[64] = 
{
     0,  1,  2,  3,  3,  2,  1,  0,
     4,  5,  6,  7,  7,  6,  5,  4,
     8,  9, 10, 11, 11, 10,  9,  8,
    12, 13, 14, 15, 15, 14, 13, 12,
    16, 17, 18, 19, 19, 18, 17, 16,
    20, 21, 22, 23, 23, 22, 21, 20,
    24, 25, 26, 27, 27, 26, 25, 24,
    28, 29, 30, 31, 31, 30, 29, 28,
};

const int BLACK_MAPPER[64] = 
{
    28, 29, 30, 31, 31, 30, 29, 28,
    24, 25, 26, 27, 27, 26, 25, 24,
    20, 21, 22, 23, 23, 22, 21, 20,
    16, 17, 18, 19, 19, 18, 17, 16,
    12, 13, 14, 15, 15, 14, 13, 12,
     8,  9, 10, 11, 11, 10,  9,  8,
     4,  5,  6,  7,  7,  6,  5,  4,
     0,  1,  2,  3,  3,  2,  1,  0,
};

const int QUEEN_DISTANCE_MULTIPLIER = 5;
const int ROOK_DISTANCE_MULTIPLIER = 3;
const int BISHOP_DISTANCE_MULTIPLIER = 2;
const int KNIGHT_DISTANCE_MULTIPLIER = 1;
const int DISTANCE_BONUS = 3;

const int OUTPOST_BONUS = 50;

const int ISOLATED_PAWN_BONUS = -31;
const int DOUBLED_PAWN_BONUS = -27;
const int PASSED_PAWN_BONUS[8] = { 0, 15, 31, 99, 175, 229, 253 };

const int SPACE_BONUS = 3;

const int KING_IN_CENTER_BONUS = -70;
const int KING_PAWNSHIELD_BONUS = 21; //per pawn
const int KING_HALF_OPEN_FILE_BONUS = -21; //only if on flank

//[is_open us][is_open them]
const int ROOK_OPEN_FILE_BONUS[2][2] = {{ 80, 50 }, { 20, -20 }};

int evaluate(Position *pos)
{
    int score = pos->current_state->is_standard_material_config ? material::material_scores[pos->current_state->material_key] : material::evaluate_material_config(pos->material);

    Bitboard outposts_white = (pos->piece_bitboard[WHITE_KNIGHT] | pos->piece_bitboard[WHITE_BISHOP]) //outposts are bishops and knights
                            &  pos->current_state->pawn_attack_bitboards[white] //that are on a square controlled by our pawns
                            & ~pos->current_state->pawn_attack_bitboards[black] //that is not attacked by an opponent pawn
                            &  black_side; //that is on the opponents sied of the board
    Bitboard outposts_black = (pos->piece_bitboard[BLACK_KNIGHT] | pos->piece_bitboard[BLACK_BISHOP]) //outposts are bishops and knights
                            &  pos->current_state->pawn_attack_bitboards[black] //that are on a square controlled by our pawns
                            & ~pos->current_state->pawn_attack_bitboards[white] //that is not attacked by an opponent pawn
                            &  white_side; //that is on the opponents sied of the board
    score += OUTPOST_BONUS * popcount(outposts_white);
    score -= OUTPOST_BONUS * popcount(outposts_black);

    Square white_king_square = lsb(pos->piece_bitboard[WHITE_KING]);
    Square black_king_square = lsb(pos->piece_bitboard[BLACK_KING]);
    score += KING_SCORES[WHITE_MAPPER[white_king_square]];
    score -= KING_SCORES[BLACK_MAPPER[black_king_square]];

    int king_distance = 0;

    Bitboard white_rooks = pos->piece_bitboard[WHITE_ROOK];
    Bitboard black_rooks = pos->piece_bitboard[BLACK_ROOK];
    while(white_rooks)
    {
        Square rook_square = pop_lsb(&white_rooks);
        bool is_open_white = pos->piece_bitboard[WHITE_PAWN] & file_bitboards[rook_square];
        bool is_open_black = pos->piece_bitboard[BLACK_PAWN] & file_bitboards[rook_square];
        score += ROOK_OPEN_FILE_BONUS[is_open_white][is_open_black];
        score += ROOK_SCORES[WHITE_MAPPER[rook_square]];
        king_distance += (14 - manhattan_distance[rook_square][black_king_square]) * ROOK_DISTANCE_MULTIPLIER;
    }
    while(black_rooks)
    {
        Square rook_square = pop_lsb(&black_rooks);
        bool is_open_white = pos->piece_bitboard[WHITE_PAWN] & file_bitboards[rook_square];
        bool is_open_black = pos->piece_bitboard[BLACK_PAWN] & file_bitboards[rook_square];
        score -= ROOK_OPEN_FILE_BONUS[is_open_black][is_open_white];
        score -= ROOK_SCORES[BLACK_MAPPER[rook_square]];
        king_distance -= (14 - manhattan_distance[rook_square][white_king_square]) * ROOK_DISTANCE_MULTIPLIER;
    }

    Bitboard white_knights = pos->piece_bitboard[WHITE_KNIGHT];
    Bitboard black_knights = pos->piece_bitboard[BLACK_KNIGHT];
    while(white_knights)
    {
        Square knight_square = pop_lsb(&white_knights);
        score += KNIGHT_SCORES[WHITE_MAPPER[knight_square]];
        king_distance += (14 - manhattan_distance[knight_square][black_king_square]) * KNIGHT_DISTANCE_MULTIPLIER;
    }
    while(black_knights)
    {
        Square knight_square = pop_lsb(&black_knights);
        score -= KNIGHT_SCORES[BLACK_MAPPER[knight_square]];
        king_distance -= (14 - manhattan_distance[knight_square][white_king_square]) * KNIGHT_DISTANCE_MULTIPLIER;
    }

    Bitboard white_bishops = pos->piece_bitboard[WHITE_BISHOP];
    Bitboard black_bishops = pos->piece_bitboard[BLACK_BISHOP];
    while(white_bishops)
    {
        Square bishop_square = pop_lsb(&white_bishops);
        score += BISHOP_SCORES[WHITE_MAPPER[bishop_square]];
        king_distance += (14 - manhattan_distance[bishop_square][black_king_square]) * BISHOP_DISTANCE_MULTIPLIER;
    }
    while(black_bishops)
    {
        Square bishop_square = pop_lsb(&black_bishops);
        score -= BISHOP_SCORES[BLACK_MAPPER[bishop_square]];
        king_distance -= (14 - manhattan_distance[bishop_square][white_king_square]) * BISHOP_DISTANCE_MULTIPLIER;
    }

    Bitboard white_queens = pos->piece_bitboard[WHITE_QUEEN];
    Bitboard black_queens = pos->piece_bitboard[BLACK_QUEEN];
    while(white_queens)
    {
        Square queen_square = pop_lsb(&white_queens);
        king_distance += (14 - manhattan_distance[queen_square][black_king_square]) * QUEEN_DISTANCE_MULTIPLIER;
    }
    while(black_queens)
    {
        Square queen_square = pop_lsb(&black_queens);
        king_distance -= (14 - manhattan_distance[queen_square][white_king_square]) * QUEEN_DISTANCE_MULTIPLIER;
    }

    Bitboard white_pawns = pos->piece_bitboard[WHITE_PAWN];
    Bitboard black_pawns = pos->piece_bitboard[BLACK_PAWN];

    if(pos->piece_bitboard[WHITE_KING] & center_files)
    {
        score += KING_IN_CENTER_BONUS;
    }
    else if(pos->piece_bitboard[WHITE_KING] & queenside_flank)
    {
        score += KING_PAWNSHIELD_BONUS * popcount(white_pawns & white_queenside_pawnshield);
        for(int i = 0; i <= 2; i++)
        {
            if(!(white_pawns & file_bitboards[i]))
                score += KING_HALF_OPEN_FILE_BONUS;
        }
    }
    else //if(pos->piece_bitboard[WHITE_KING] & kingside_flank)
    {
        score += KING_PAWNSHIELD_BONUS * popcount(white_pawns & white_kingside_pawnshield);
        for(int i = 5; i <= 7; i++)
        {
            if(!(white_pawns & file_bitboards[i]))
                score += KING_HALF_OPEN_FILE_BONUS;
        }
    }

    if(pos->piece_bitboard[BLACK_KING] & center_files)
    {
        score -= KING_IN_CENTER_BONUS;
    }
    else if(pos->piece_bitboard[BLACK_KING] & queenside_flank)
    {
        score -= KING_PAWNSHIELD_BONUS * popcount(black_pawns & black_queenside_pawnshield);
        for(int i = 0; i <= 2; i++)
        {
            if(!(black_pawns & file_bitboards[i]))
                score -= KING_HALF_OPEN_FILE_BONUS;
        }
    }
    else //if(pos->piece_bitboard[BLACK_KING] & kingside_flank)
    {
        score -= KING_PAWNSHIELD_BONUS * popcount(black_pawns & black_kingside_pawnshield);
        for(int i = 5; i <= 7; i++)
        {
            if(!(black_pawns & file_bitboards[i]))
                score -= KING_HALF_OPEN_FILE_BONUS;
        }
    }

    while(white_pawns)
    {
        Square pawn_square = pop_lsb(&white_pawns);
        int rank = pawn_square / 8;

        if(isolated_pawn_bitboards[pawn_square] & pos->piece_bitboard[WHITE_PAWN])
            score += ISOLATED_PAWN_BONUS;
        if(doubled_pawn_bitboards[white][pawn_square] & pos->piece_bitboard[WHITE_PAWN])
            score += DOUBLED_PAWN_BONUS;
        if(!(passed_pawn_bitboards[white][pawn_square] & pos->piece_bitboard[BLACK_PAWN]))
            score += PASSED_PAWN_BONUS[rank];
        score += PAWN_SCORES[WHITE_MAPPER[pawn_square]];
    }
    while(black_pawns)
    {
        Square pawn_square = pop_lsb(&black_pawns);
        int rank = pawn_square / 8;

        if(isolated_pawn_bitboards[pawn_square] & pos->piece_bitboard[BLACK_PAWN])
            score -= ISOLATED_PAWN_BONUS;
        if(doubled_pawn_bitboards[black][pawn_square] & pos->piece_bitboard[BLACK_PAWN])
            score -= DOUBLED_PAWN_BONUS;
        if(!(passed_pawn_bitboards[black][pawn_square] & pos->piece_bitboard[WHITE_PAWN]))
            score -= PASSED_PAWN_BONUS[7 - rank];
        score -= PAWN_SCORES[BLACK_MAPPER[pawn_square]];
    }


    //Space scores: Space is the amount of squares attacked, which are also in the enemies terretory
    int space_white = popcount(pos->current_state->attack_bitboards[white] | pos->color_bitboard[white]);
    int space_black = popcount(pos->current_state->attack_bitboards[black] | pos->color_bitboard[black]);

    score += (space_white - space_black) * SPACE_BONUS;
    score += king_distance * DISTANCE_BONUS;

    int preference = pos->color_to_move == white ? 1 : -1;

    return preference * score;
}