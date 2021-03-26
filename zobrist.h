#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"

namespace zobrist
{
    //Setup random numbers
    void init();

    void change_piece(Key *key, Piece piece, Square square);
    void change_color_to_move(Key *key);
    void change_casteling(Key *key, int casteling);
    void change_en_passent(Key *key, Square en_passent_square);
}

#endif