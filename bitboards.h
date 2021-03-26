#pragma once
#ifndef BITBOARDS_H
#define BITBOARDS_H

#include <stdint.h> //uint16_t

#include "types.h"

//[Color][Square]
extern Bitboard pawn_attack_bitboards[2][64];

extern Bitboard king_attack_bitboards[64];

extern Bitboard knight_attack_bitboards[64];

extern Bitboard file_bitboards[64];

extern Bitboard rank_bitboards[64];

extern Bitboard bishop_occupancy_bitboards[64];

extern Bitboard rook_occupancy_bitboards[64];

extern Bitboard white_side;
extern Bitboard black_side;

extern Bitboard isolated_pawn_bitboards[64];
extern Bitboard doubled_pawn_bitboards[2][64];
extern Bitboard passed_pawn_bitboards[2][64];

//[from][to], rays that start at from (exclusive) and end at to (inclusive)
extern Bitboard ray_bitboards[64][64];

//[from][to], rays that start at from (exclusive), go through to, but end at the end of the board
extern Bitboard extended_ray_bitboards[64][64];

extern int manhattan_distance[64][64];

extern void init_bitboards();

inline Bitboard pawn_attack_bb(Color c, Square s)
{
    return pawn_attack_bitboards[c][s];
}

inline Bitboard king_attack_bb(Square s)
{
    return king_attack_bitboards[s];
}

inline Bitboard knight_attack_bb(Square s)
{
    return knight_attack_bitboards[s];
}

extern Bitboard rook_attack_bb(Square s, Bitboard blockers);

extern Bitboard bishop_attack_bb(Square s, Bitboard blockers);

inline Bitboard queen_attack_bb(Square s, Bitboard blockers)
{
    return rook_attack_bb(s, blockers) | bishop_attack_bb(s, blockers);
}

inline Bitboard file_bb(Square s)
{
    return file_bitboards[s];
}

inline Bitboard rank_bb(Square s)
{
    return rank_bitboards[s];
}

template<Direction>
extern Bitboard shift(Bitboard b);

extern void print_bitboard(Bitboard b);

inline int popcount(Bitboard b) {
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
  return (int)_mm_popcnt_u64(b);
#else // Assumed gcc or compatible compiler
  return __builtin_popcountll(b);
#endif
}

#if defined(__GNUC__)  // GCC, Clang, ICC
inline Square lsb(Bitboard b) {
  return Square(__builtin_ctzll(b));
}
inline Square msb(Bitboard b) {
  return Square(63 ^ __builtin_clzll(b));
}
#elif defined(_MSC_VER)  // MSVC
#ifdef _WIN64  // MSVC, WIN64
inline Square lsb(Bitboard b) {
  unsigned long idx;
  _BitScanForward64(&idx, b);
  return (Square) idx;
}

inline Square msb(Bitboard b) {
  unsigned long idx;
  _BitScanReverse64(&idx, b);
  return (Square) idx;
}
#else  // MSVC, WIN32
inline Square lsb(Bitboard b) {
  unsigned long idx;
  if (b & 0xffffffff) {
      _BitScanForward(&idx, int32_t(b));
      return Square(idx);
  } else {
      _BitScanForward(&idx, int32_t(b >> 32));
      return Square(idx + 32);
  }
}

inline Square msb(Bitboard b) {
  unsigned long idx;
  if (b >> 32) {
      _BitScanReverse(&idx, int32_t(b >> 32));
      return Square(idx + 32);
  } else {
      _BitScanReverse(&idx, int32_t(b));
      return Square(idx);
  }
}
#endif
#else  // Compiler is neither GCC nor MSVC compatible
#error "Compiler not supported."
#endif

inline Square pop_lsb(Bitboard* b) {
  const Square s = lsb(*b);
  *b &= *b - 1;
  return s;
}

#endif //!BITBOARDS_H