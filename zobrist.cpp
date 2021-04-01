#include <random>

#include "zobrist.h"

namespace zobrist
{
    Key get_random_key()
    {
        return (((Key)(unsigned int)rand() << 32) + ((Key)(unsigned int)rand() << 24) + ((Key)(unsigned int)rand() << 16) + (Key)(unsigned int)rand());
    }

    Key piece_data[13][64];
    Key color_data;
    Key casteling_data[16];
    Key en_passent_data[64];

    void init()
    {
        srand(1605199911);
        color_data = get_random_key();
        for(int piece = 0; piece < 13; piece++)
        {
            for(int square = 0; square < 64; square++)
            {
                if(piece == 0)
                    piece_data[piece][square] = 0;
                else 
                    piece_data[piece][square] = get_random_key();
            }
        }
        for(int i = 0; i < 16; i++)
            casteling_data[i] = get_random_key();
        for(int i = 0; i < 64; i++)
            en_passent_data[i] = get_random_key();
    }

    void change_piece(Key *key, Piece piece, Square square)
    {
        *key ^= piece_data[piece][square];
    }

    void change_color_to_move(Key *key)
    {
        *key ^= color_data;
    }

    void change_casteling(Key *key, int casteling)
    {
        *key ^= casteling_data[casteling];
    }

    void change_en_passent(Key *key, Square en_passent_square)
    {
        *key ^= en_passent_data[en_passent_square];
    }
}
