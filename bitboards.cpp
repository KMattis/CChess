#include <stdio.h>

#include "bitboards.h"
#include "magics.h"



//Precomputed tables used for magic bitboard sliding attacks
Bitboard rook_attack_table[64][4096];
Bitboard bishop_attack_table[64][1024];

Bitboard pawn_attack_bitboards[2][64];
Bitboard king_attack_bitboards[64];
Bitboard knight_attack_bitboards[64];
Bitboard file_bitboards[64];
Bitboard rank_bitboards[64];
Bitboard bishop_occupancy_bitboards[64];
Bitboard rook_occupancy_bitboards[64];

//[from][to]
Bitboard ray_bitboards[64][64];
Bitboard extended_ray_bitboards[64][64];

Bitboard white_side;
Bitboard black_side;

Bitboard isolated_pawn_bitboards[64];
Bitboard doubled_pawn_bitboards[2][64];
Bitboard passed_pawn_bitboards[2][64];

Bitboard kingside_flank;
Bitboard queenside_flank;
Bitboard center_files;
Bitboard white_kingside_pawnshield;
Bitboard white_queenside_pawnshield;
Bitboard black_kingside_pawnshield;
Bitboard black_queenside_pawnshield;

int manhattan_distance[64][64];

template<>
Bitboard shift<N>(Bitboard b)
{
    return b << 8;
}

template<>
Bitboard shift<S>(Bitboard b)
{
    return b >> 8;
}

template<>
Bitboard shift<W>(Bitboard b)
{
    return (b >> 1) & ~file_bitboards[7];
}

template<>
Bitboard shift<E>(Bitboard b)
{
    return (b << 1) & ~file_bitboards[0];
}

template<>
Bitboard shift<NW>(Bitboard b)
{
    return (b << 7) & ~file_bitboards[7];
}

template<>
Bitboard shift<NE>(Bitboard b)
{
    return (b << 9) & ~file_bitboards[0];
}

template<>
Bitboard shift<SW>(Bitboard b)
{
    return (b >> 9) & ~file_bitboards[7];
}

template<>
Bitboard shift<SE>(Bitboard b)
{
    return (b >> 7) & ~file_bitboards[0];
}

Bitboard get_blockers_from_index(int index, Bitboard mask) {
  Bitboard blockers = 0ull;
  int bits = popcount(mask);
  for (int i = 0; i < bits; i++) {
    int bitPos = pop_lsb(&mask);
    if (index & (1 << i)) {
      blockers |= (1ull << bitPos);
    }
  }
  return blockers;
}

Bitboard get_rook_attacks_slow(int square, Bitboard blockers)
{
    Bitboard attacks = 0ull;

    int rank = square / 8;
    int file = square % 8;

    for(int i = 1; rank + i < 8; i++)
    {
        int target = (rank + i) * 8 + file;
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    for(int i = 1; rank - i >= 0; i++)
    {
        int target = (rank - i) * 8 + file;
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    for(int i = 1; file + i < 8; i++)
    {
        int target = rank * 8 + (file + i);
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    for(int i = 1; file - i >= 0; i++)
    {
        int target = rank * 8 + (file - i);
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    return attacks;
}

Bitboard get_bishop_attacks_slow(int square, Bitboard blockers)
{
    Bitboard attacks = 0ull;

    int rank = square / 8;
    int file = square % 8;

    for(int i = 1; rank + i < 8 && file + i < 8; i++)
    {
        int target = (rank + i) * 8 + (file + i);
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    for(int i = 1; rank - i >= 0 && file + i < 8; i++)
    {
        int target = (rank - i) * 8 + (file + i);
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    for(int i = 1; rank + i < 8 && file - i >= 0; i++)
    {
        int target = (rank + i) * 8 + (file - i);
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    for(int i = 1; rank - i >= 0 && file - i >= 0; i++)
    {
        int target = (rank - i) * 8 + (file - i);
        set_square(attacks, target);
        if(get_square(blockers, target))
            break;
    }

    return attacks;
} 

void init_rook_magic_table() {
    for (int square = 0; square < 64; square++) {
        for (int blockerIndex = 0; blockerIndex < (1 << (64 - rook_index_bits[square])); blockerIndex++) {
            Bitboard blockers = get_blockers_from_index(blockerIndex, rook_occupancy_bitboards[square]);
            rook_attack_table[square][(blockers * rook_magics[square]) >> rook_index_bits[square]] = get_rook_attacks_slow(square, blockers);
        }
    }
}

void init_bishop_magic_table() {
    for (int square = 0; square < 64; square++) {
        for (int blockerIndex = 0; blockerIndex < (1 << (64 - bishop_index_bits[square])); blockerIndex++) {
            Bitboard blockers = get_blockers_from_index(blockerIndex, bishop_occupancy_bitboards[square]);
            bishop_attack_table[square][(blockers * bishop_magics[square]) >> bishop_index_bits[square]] = get_bishop_attacks_slow(square, blockers);
        }
    }
}

void init_bitboards()
{
    //Rank & file bitboards
    for (int square1 = 0; square1 < 64; square1++)
    {
        for (int square2 = 0; square2 < 64; square2++)
        {
            int r1 = square1 / 8;
            int f1 = square1 % 8;
            int r2 = square2 / 8;
            int f2 = square2 % 8;

            if(r1 == r2)
                set_square(rank_bitboards[square1], square2);
            if(f1 == f2)
                set_square(file_bitboards[square1], square2);
        }
    }

    for(int square = 0; square < 64; square++)
    {
        int rank = square / 8;
        int file = square % 8;

        Bitboard square_bb = 1ull << square;

        //King attack bitboards
        king_attack_bitboards[square] = shift<N> (square_bb) 
                                      | shift<S> (square_bb)
                                      | shift<W> (square_bb)
                                      | shift<E> (square_bb)
                                      | shift<NW>(square_bb)
                                      | shift<NE>(square_bb)
                                      | shift<SW>(square_bb)
                                      | shift<SE>(square_bb);

        //Pawn attack bitboards
        pawn_attack_bitboards[white][square] = shift<NW>(square_bb) | shift<NE>(square_bb);
        pawn_attack_bitboards[black][square] = shift<SW>(square_bb) | shift<SE>(square_bb);

        //Knight attack bitboards
        knight_attack_bitboards[square] = shift<N>(shift<NE>(square_bb))
                                        | shift<N>(shift<NW>(square_bb))
                                        | shift<W>(shift<NW>(square_bb))
                                        | shift<W>(shift<SW>(square_bb))
                                        | shift<S>(shift<SW>(square_bb))
                                        | shift<S>(shift<SE>(square_bb))
                                        | shift<E>(shift<SE>(square_bb))
                                        | shift<E>(shift<NE>(square_bb));
        
        //Rook occupancy bitboards
        for(int i = 1; i < 7; i++)
        {
            if(i != rank)
                set_square(rook_occupancy_bitboards[square], 8 * i + file);

            if(i != file)
                set_square(rook_occupancy_bitboards[square], 8 * rank + i);
        }

        //Bishop occupancy bitboards
        for(int i = 1; i < 7; i++)
        {
            if(rank - i > 0 && file - i > 0)
                set_square(bishop_occupancy_bitboards[square], 8 * (rank - i) + (file - i));

            if(rank - i > 0 && file + i < 7)
                set_square(bishop_occupancy_bitboards[square], 8 * (rank - i) + (file + i));

            if(rank + i < 7 && file - i > 0)
                set_square(bishop_occupancy_bitboards[square], 8 * (rank + i) + (file - i));

            if(rank + i < 7 && file + i < 7)
                set_square(bishop_occupancy_bitboards[square], 8 * (rank + i) + (file + i));
        }
    }

    init_rook_magic_table();
    init_bishop_magic_table();

    //ray bitboards
    for(int from = 0; from < 64; from++)
    {
        const int frank = from / 8;
        const int ffile = from % 8;
        for(int to = 0; to < 64; to++)
        {
            if(to == from) continue;

            const int trank = to / 8;
            const int tfile = to % 8;

            if(frank == trank)
            {
                if(ffile < tfile)
                {
                    for(int i = 1; ffile + i <= 7; i++)
                    {
                        set_square(extended_ray_bitboards[from][to], from + i);
                        if(ffile + i <= tfile)
                            set_square(ray_bitboards[from][to], from + i);
                    }
                }
                else
                {
                    for(int i = 1; ffile - i >= 0; i++)
                    {
                        set_square(extended_ray_bitboards[from][to], from - i);
                        if(ffile - i >= tfile)
                            set_square(ray_bitboards[from][to], from - i);
                    }
                }
            }
            else if(ffile == tfile)
            {
                if(frank < trank)
                {
                    for(int i = 1; frank + i <= 7; i++)
                    {
                        set_square(extended_ray_bitboards[from][to], from + i * 8);
                        if(frank + i <= trank)
                            set_square(ray_bitboards[from][to], from + i * 8);
                    }
                }
                else
                {
                    for(int i = 1; frank - i >= 0; i++)
                    {
                        set_square(extended_ray_bitboards[from][to], from - i * 8);
                        if(frank - i >= trank)
                           set_square(ray_bitboards[from][to], from - i * 8);
                    }
                }
            }
            else if(frank - trank == ffile - tfile)
            {
                if(frank < trank){
                    for (int i = 1; frank + i <= 7 && ffile + i <= 7; i++) 
                    {
                        set_square(extended_ray_bitboards[from][to], from + (i * 8 + i));
                        if(frank + i <= trank)
                           set_square(ray_bitboards[from][to], from + (i * 8 + i));
                    } 
                }
                else
                {
                    for (int i = 1; frank - i >= 0 && ffile - i >= 0; i++) 
                    {
                        set_square(extended_ray_bitboards[from][to], from - (i * 8 + i));
                        if(frank - i >= trank)
                            set_square(ray_bitboards[from][to], from - (i * 8 + i));
                    } 
                }
            }
            else if(frank - trank == - ffile + tfile)
            {
                if(frank < trank){
                    for (int i = 1; frank + i <= 7 && ffile -i >= 0; i++) 
                    {
                        set_square(extended_ray_bitboards[from][to], from + i * 8 - i);
                        if(frank + i <= trank)
                           set_square(ray_bitboards[from][to], from + i * 8 - i);
                    } 
                }
                else
                {
                    for (int i = 1; frank - i >= 0 && ffile + i <= 7; i++) 
                    {
                        set_square(extended_ray_bitboards[from][to], from - i * 8 + i);
                        if(frank - i >= trank)
                           set_square(ray_bitboards[from][to], from - i * 8 + i);
                    } 
                }
            }
        }
    }

    white_side = rank_bitboards[a1] | rank_bitboards[a2] | rank_bitboards[a3] | rank_bitboards[a4];
    black_side = rank_bitboards[a5] | rank_bitboards[a6] | rank_bitboards[a7] | rank_bitboards[a8];

    for(int square = 0; square < 64; square++)
    {
        int file = square % 8;
        doubled_pawn_bitboards[white][square] = ray_bitboards[square][7*8 + file];
        doubled_pawn_bitboards[black][square] = ray_bitboards[square][file];

        passed_pawn_bitboards[white][square] = ray_bitboards[square][7*8 + file];
        passed_pawn_bitboards[black][square] = ray_bitboards[square][file];

        if(file > 0)
        {
            isolated_pawn_bitboards[square] |= file_bitboards[file - 1];
            passed_pawn_bitboards[white][square] |= ray_bitboards[square - 1][7 * 8 + file - 1];
            passed_pawn_bitboards[black][square] |= ray_bitboards[square - 1][file - 1];
        }

        if(file < 7)
        {
            isolated_pawn_bitboards[square] |= file_bitboards[file + 1];
            passed_pawn_bitboards[white][square] |= ray_bitboards[square + 1][7 * 8 + file + 1];
            passed_pawn_bitboards[black][square] |= ray_bitboards[square + 1][file + 1];
        }        
    }

    //distance tables
    for(int from = 0; from < 64; from++)
    {
        for(int to = 0; to < 64; to++)
        {
            int frank = from / 8;
            int ffile = from % 8;
            int trank = to / 8;
            int tfile = to % 8;
            int r_dist = frank > trank ? frank - trank : trank - frank;
            int f_dist = ffile > tfile ? ffile - tfile : tfile - ffile;

            manhattan_distance[from][to] = r_dist + f_dist;
        }
    }

    //flank bitboards
    queenside_flank = file_bitboards[a1] | file_bitboards[b1] | file_bitboards[c1];
    kingside_flank  = file_bitboards[f1] | file_bitboards[g1] | file_bitboards[h1];
    center_files    = file_bitboards[d1] | file_bitboards[e1];
    white_kingside_pawnshield  = kingside_flank  & (rank_bitboards[a2] | rank_bitboards[a3]);
    white_queenside_pawnshield = queenside_flank & (rank_bitboards[a2] | rank_bitboards[a3]);
    black_kingside_pawnshield  = kingside_flank  & (rank_bitboards[a7] | rank_bitboards[a6]);
    black_queenside_pawnshield = queenside_flank & (rank_bitboards[a7] | rank_bitboards[a6]);
}

Bitboard bishop_attack_bb(Square s, Bitboard blockers)
{
    blockers &= bishop_occupancy_bitboards[s];
    Bitboard key = (blockers * bishop_magics[s]) >> bishop_index_bits[s];
    return bishop_attack_table[s][key];
}

Bitboard rook_attack_bb(Square s, Bitboard blockers)
{
    blockers &= rook_occupancy_bitboards[s];
    Bitboard key = (blockers * rook_magics[s]) >> rook_index_bits[s];
    return rook_attack_table[s][key];
}

void print_bitboard(Bitboard b)
{
    for(int rank = 7; rank >= 0; rank--)
    {
        for(int file = 0; file < 8; file++)
        {
            printf(" %c ", get_square(b, rank * 8 + file) ? '#' : '.');
        }
        printf("\n");
    }    
}
