#include "material.h"


namespace material{
    int material_key_offsets[13];
    int material_scores[9 * 9 * 3 * 3 * 3 * 3 * 3 * 3 * 2 * 2];

    const int PIECE_VALUES[13] = { 0, 100, 325, 325, 550, 1100, 0, -100, -325, -325, -550, -1100, 0 };
    const int BISHOP_PAIR_BONUS = 30;

    void init()
    {
        material_key_offsets[WHITE_QUEEN]  = 1;
        material_key_offsets[BLACK_QUEEN]  = 2;
        material_key_offsets[WHITE_ROOK]   = 2 * 2;
        material_key_offsets[BLACK_ROOK]   = 2 * 2 * 3;
        material_key_offsets[WHITE_BISHOP] = 2 * 2 * 3 * 3;
        material_key_offsets[BLACK_BISHOP] = 2 * 2 * 3 * 3 * 3;
        material_key_offsets[WHITE_KNIGHT] = 2 * 2 * 3 * 3 * 3 * 3;
        material_key_offsets[BLACK_KNIGHT] = 2 * 2 * 3 * 3 * 3 * 3 * 3;
        material_key_offsets[WHITE_PAWN]   = 2 * 2 * 3 * 3 * 3 * 3 * 3 * 3;
        material_key_offsets[BLACK_PAWN]   = 2 * 2 * 3 * 3 * 3 * 3 * 3 * 3 * 8;
        
        //Precompute all standard material configuration scores
        for(int wp = 0; wp <= 8; wp++)
        for(int bp = 0; bp <= 8; bp++)
        for(int wn = 0; wn <= 2; wn++)
        for(int bn = 0; bn <= 2; bn++)
        for(int wb = 0; wb <= 2; wb++)
        for(int bb = 0; bb <= 2; bb++)
        for(int wr = 0; wr <= 2; wr++)
        for(int br = 0; br <= 2; br++)
        for(int wq = 0; wq <= 1; wq++)
        for(int bq = 0; bq <= 1; bq++)
        {
            int material[13];
            material[NO_PIECE] = 0;
            material[WHITE_PAWN] = wp;
            material[BLACK_PAWN] = bp;
            material[WHITE_KNIGHT] = wn;
            material[BLACK_KNIGHT] = bn;
            material[WHITE_BISHOP] = wb;
            material[BLACK_BISHOP] = bb;
            material[WHITE_ROOK] = wr;
            material[BLACK_ROOK] = br;
            material[WHITE_QUEEN] = wq;
            material[BLACK_QUEEN] = bq;
            material[WHITE_KING] = 1;
            material[BLACK_KING] = 1;

            int material_key = material_array_to_index(material);
            material_scores[material_key] = material::evaluate_material_config(material);
        }
    }

    int material_array_to_index(int material[])
    {
        return 
          material_key_offsets[WHITE_QUEEN]  * material[WHITE_QUEEN]
        + material_key_offsets[BLACK_QUEEN]  * material[BLACK_QUEEN]
        + material_key_offsets[WHITE_ROOK]   * material[WHITE_ROOK]
        + material_key_offsets[BLACK_ROOK]   * material[BLACK_ROOK]
        + material_key_offsets[WHITE_BISHOP] * material[WHITE_BISHOP]
        + material_key_offsets[BLACK_BISHOP] * material[BLACK_BISHOP]
        + material_key_offsets[WHITE_KNIGHT] * material[WHITE_KNIGHT]
        + material_key_offsets[BLACK_KNIGHT] * material[BLACK_KNIGHT]
        + material_key_offsets[WHITE_PAWN]   * material[WHITE_PAWN]
        + material_key_offsets[BLACK_PAWN]   * material[BLACK_PAWN];
    }

    void material_key_add_piece(unsigned int *key, Piece piece)
    {
        *key += material_key_offsets[piece];
    }

    void material_key_remove_piece(unsigned int *key, Piece piece)
    {
        *key -= material_key_offsets[piece];
    }

    int evaluate_material_config(int material[])
    {
        int score = 0;
        for(int i = 0; i < 13; i++)
        {
            score += material[i] * PIECE_VALUES[i];
        }

        //Bishop pair scores
        if(material[WHITE_BISHOP] >= 2)
            score += BISHOP_PAIR_BONUS;
        if(material[BLACK_BISHOP] >= 2)
            score -= BISHOP_PAIR_BONUS;

        return score;
    }
}