#ifndef POSITION_H
#define POSITION_H

#include <cstring>
#include <sstream>

#include "types.h"

using std::string;

struct State {
    State *last_state;
    Move move;
    int ply;
    int fifty_moves;
    Square en_passent;
    //The suqares attacked by the given color
    Bitboard attack_bitboards[2];
    Bitboard pawn_attack_bitboards[2];
    //The pieces of the moving color pinned to the king
    Bitboard pinner_bitboard;
    //The squares where we have to move to evade check. If not in check, then all bits are set
    Bitboard checker_bitboard;
    Bitboard blocker_bitboard;
    //The free squares
    Bitboard free;

    int in_check;
    int in_double_check;

    unsigned int casteling_rights;

    Key position_key;
    unsigned int material_key;
    bool is_standard_material_config;
};

struct Position {
    State *current_state = nullptr;

    Piece board[64];
    Color color_to_move;

    int material[13];

    Bitboard piece_bitboard[13];
    Bitboard color_bitboard[2];

    int en_passent_moves;

    void init(string &fen);

    void do_move(Move move);
    void undo_move();

    void do_null_move();
    void undo_null_move();

    bool is_legal(Move move);
    bool is_pseudo_legal(Move move);
private:
    void compute_bitboards();
};

#endif //!POSITION_H