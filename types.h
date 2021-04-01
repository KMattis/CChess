#ifndef TYPES_H
#define TYPES_H

#ifdef DEBUG
#define ASSERT(cond) if(!(cond)) printf("Condition failed, %s, at line: %d\n", #cond, __LINE__);
#else
#define ASSERT(cond)
#endif

//Enums
enum Color {
    white, black
};

inline Color operator~(Color c){
    return (Color) (1 - (int) c);
}

enum PieceType {
    NO_PIECE_TYPE = 0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum Piece {
    NO_PIECE = 0, WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
                 BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING
};

#define color_of(piece) ((Color) (piece - 1) / 6)
#define piece_type_of(piece) ((PieceType) ((piece - 1) % 6) + 1)
#define make_piece(pieceType, color) ((Piece) (pieceType + (color * 6)))
#define piece_type(piece) ((PieceType) (piece % 6))

enum Square {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,
    NO_SQUARE = -1
};



enum Direction {
    N = 8, S = -8, W = -1, E = 1, NE = 9, SE = -7, NW = 7, SW = -9
};

inline Square operator-(Square s, Direction d)
{
    return (Square)((int)s - (int)d);
}

inline Square operator+(Square s, Direction d)
{
    return (Square)((int)s + (int)d);
}

//Define bitboard type
typedef unsigned long long Bitboard;

//Bitboard manipulation macros

//sets the square to true
#define set_square(bitboard, square) (bitboard |= (1ull << (square)))

//Returns 0 if the square in the bitboard is not set, not zero otherwise
#define get_square(bitboard, square) (bitboard & (1ull << (square)))

//sets the square to false
#define pop_square(bitboard, square) (bitboard &= ~(1ull << (square)))

//Move type and move related macros
typedef unsigned int Move;
const Move NO_MOVE = 0;

/*
 * 0000 0000 0000 0000 0000 0000 0011 1111 //from
 * 0000 0000 0000 0000 0000 1111 1100 0000 //to
 * 0000 0000 0000 0000 1111 0000 0000 0000 //moved
 * 0000 0000 0000 1111 0000 0000 0000 0000 //captured
 * 0000 0000 0011 0000 0000 0000 0000 0000 //promoted (only type) - KNIGHT
 * 0000 0000 0100 0000 0000 0000 0000 0000 //is promotion
 * 0000 0000 1000 0000 0000 0000 0000 0000 //is en_passent
 * 0000 0001 0000 0000 0000 0000 0000 0000 //is casteling
 * 0000 0010 0000 0000 0000 0000 0000 0000 //is double pawn
 */

const unsigned int FROM_INDEX = 0;
const unsigned int FROM_MASK = 0x3F << FROM_INDEX;
const unsigned int TO_INDEX = 6;
const unsigned int TO_MASK = 0x3F << TO_INDEX;
const unsigned int MOVED_INDEX = 12;
const unsigned int MOVED_MASK = 0xF << MOVED_INDEX;
const unsigned int CAPTURED_INDEX = 16;
const unsigned int CAPTURED_MASK = 0xF << CAPTURED_INDEX;
const unsigned int PROMOTED_INDEX = 20;
const unsigned int PROMOTED_MASK = 0x3 << PROMOTED_INDEX;
const unsigned int IS_PROMOTION_INDEX = 22; 
const unsigned int IS_PROMOTION_MASK = 0x1 << IS_PROMOTION_INDEX;
const unsigned int IS_EN_PASSENT_INDEX = 23;
const unsigned int IS_EN_PASSENT_MASK = 0x1 << IS_EN_PASSENT_INDEX;
const unsigned int IS_CASTELING_INDEX = 24;
const unsigned int IS_CASTELING_MASK = 0x1 << IS_CASTELING_INDEX;
const unsigned int IS_DOUBLE_PAWN_INDEX = 25;
const unsigned int IS_DOUBLE_PAWN_MASK = 0x1 << IS_DOUBLE_PAWN_INDEX;

#define from_square(move) ((Square)((move & FROM_MASK) >> FROM_INDEX))
#define to_square(move) ((Square)((move & TO_MASK) >> TO_INDEX))
#define moved_piece(move) ((Piece)((move & MOVED_MASK) >> MOVED_INDEX))
#define captured_piece(move) ((Piece)((move & CAPTURED_MASK) >> CAPTURED_INDEX))
#define promoted_piece(move) ((PieceType)(((move & PROMOTED_MASK) >> PROMOTED_INDEX) + (int)KNIGHT))
#define is_promotion(move) (move & IS_PROMOTION_MASK)
#define is_en_passent(move) (move & IS_EN_PASSENT_MASK)
#define is_casteling(move) (move & IS_CASTELING_MASK)
#define is_double_pawn(move) (move & IS_DOUBLE_PAWN_MASK)

#define make_move(from, to, moved, captured) ((from << FROM_INDEX) | (to << TO_INDEX) | (moved << MOVED_INDEX) | (captured << CAPTURED_INDEX))
#define make_move_promotion(from, to, moved, captured, promoted) ((from << FROM_INDEX) | (to << TO_INDEX) | (moved << MOVED_INDEX) | (captured << CAPTURED_INDEX) | ((promoted - (int) KNIGHT) << PROMOTED_INDEX) | IS_PROMOTION_MASK)
#define make_move_en_passent(from, to, moved) ((from << FROM_INDEX) | (to << TO_INDEX) | (moved << MOVED_INDEX) | IS_EN_PASSENT_MASK)
#define make_move_casteling(from, to, moved) ((from << FROM_INDEX) | (to << TO_INDEX) | (moved << MOVED_INDEX) | IS_CASTELING_MASK)
#define make_move_double_pawn(from, to, moved) ((from << FROM_INDEX) | (to << TO_INDEX) | (moved << MOVED_INDEX) | IS_DOUBLE_PAWN_MASK)

//Casteling rights
typedef unsigned int casteling_rights;

const unsigned int WHITE_KINGSIDE_CASTELING  = 0x1;
const unsigned int WHITE_QUEENSIDE_CASTELING = 0x2;
const unsigned int BLACK_KINGSIDE_CASTELING  = 0x4;
const unsigned int BLACK_QUEENSIDE_CASTELING = 0x8;

const unsigned int WHITE_CASTELING = WHITE_KINGSIDE_CASTELING | WHITE_QUEENSIDE_CASTELING;
const unsigned int BLACK_CASTELING = BLACK_KINGSIDE_CASTELING | BLACK_QUEENSIDE_CASTELING;
const unsigned int CASTELING[2] = { WHITE_CASTELING, BLACK_CASTELING };

typedef short Depth;
typedef unsigned long long Key;

#endif //!TYPES_H