#ifndef MATERIAL_H
#define MATERIAL_H

#include "types.h"

namespace material
{
    extern int material_scores[9 * 9 * 3 * 3 * 3 * 3 * 3 * 3 * 2 * 2];
    extern const int PIECE_VALUES[13];
    //Init default material constellations (up to 8 pawns, 2 knights, 2 bishops, 2 rooks, 1 queen per side)
    void init();

    //Returns the index corresponding to the material array.
    int material_array_to_index(int material[]);

    void material_key_add_piece(unsigned int *key, Piece piece);
    
    void material_key_remove_piece(unsigned int *key, Piece piece);

    int evaluate_material_config(int material[]);
}

#endif