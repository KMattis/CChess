#include <stdio.h>

#include "bitboards.h"
#include "movegen.h"

int PIECE_CAPTURE_VALUES[13] = {0, 1, 2, 2, 3, 4, 5, 1, 2, 2, 3, 4, 5 };

MoveExt *add_king_moves(Position *pos, MoveExt *move_list, bool only_captures)
{
    Piece our_king = make_piece(KING, pos->color_to_move);
    Square king_square = lsb(pos->piece_bitboard[our_king]);
    Bitboard king_moves = king_attack_bb(king_square) & ~pos->current_state->attack_bitboards[~pos->color_to_move] & ~pos->color_bitboard[pos->color_to_move];
    if(only_captures) king_moves &= pos->color_bitboard[~pos->color_to_move];
    while (king_moves)
    {
        int target = pop_lsb(&king_moves);
        Piece captured = pos->board[target];
        int score = captured ? PIECE_CAPTURE_VALUES[captured] * 10000000 - PIECE_CAPTURE_VALUES[our_king] * 1000000 : 0;
        *move_list++ = MoveExt(make_move(king_square, target, our_king, captured), score);
    }

    return move_list;
}

MoveExt *add_casteling_moves(Position *pos, MoveExt *move_list)
{
    if (pos->color_to_move == white)
    {
        if(pos->current_state->casteling_rights & WHITE_KINGSIDE_CASTELING)
        {
            Bitboard f1_g1 = (1ull << f1) | (1ull << g1);
            if ((f1_g1 & pos->current_state->attack_bitboards[~pos->color_to_move]) == 0 && (f1_g1 & pos->current_state->blocker_bitboard) == 0)
                *move_list++ = MoveExt(make_move_casteling(e1, g1, WHITE_KING), 0);
        }
        if(pos->current_state->casteling_rights & WHITE_QUEENSIDE_CASTELING)
        {
            if(pos->board[e1] != WHITE_KING) printf("BRU");
            Bitboard c1_d1 = (1ull << c1) | (1ull << d1);
            Bitboard b1_c1_d1 = (1ull << b1) | c1_d1;
            if ((c1_d1 & pos->current_state->attack_bitboards[~pos->color_to_move]) == 0 && (b1_c1_d1 & pos->current_state->blocker_bitboard) == 0)
                *move_list++ = MoveExt(make_move_casteling(e1, c1, WHITE_KING), 0);
        }
    }
    else
    {
        if(pos->current_state->casteling_rights & BLACK_KINGSIDE_CASTELING)
        {
            Bitboard f8_g8 = (1ull << f8) | (1ull << g8);
            if ((f8_g8 & pos->current_state->attack_bitboards[~pos->color_to_move]) == 0 && (f8_g8 & pos->current_state->blocker_bitboard) == 0)
                *move_list++ = MoveExt(make_move_casteling(e8, g8, BLACK_KING), 0);
        }
        if(pos->current_state->casteling_rights & BLACK_QUEENSIDE_CASTELING)
        {
            Bitboard c8_d8 = (1ull << c8) | (1ull << d8);
            Bitboard b8_c8_d8 = (1ull << b8) | c8_d8;
            if ((c8_d8 & pos->current_state->attack_bitboards[~pos->color_to_move]) == 0 && (b8_c8_d8 & pos->current_state->blocker_bitboard) == 0)
                *move_list++ = MoveExt(make_move_casteling(e8, c8, BLACK_KING), 0);
        }
    }
    return move_list;
}

MoveExt *add_slider_moves(Position *pos, MoveExt *move_list, bool only_captures)
{
    //Queens count as bishops and rooks
    Bitboard bishops = pos->piece_bitboard[make_piece(BISHOP, pos->color_to_move)] | pos->piece_bitboard[make_piece(QUEEN, pos->color_to_move)];

    Bitboard rooks = pos->piece_bitboard[make_piece(ROOK, pos->color_to_move)] | pos->piece_bitboard[make_piece(QUEEN, pos->color_to_move)];

    while (bishops)
    {
        Square bishop_square = pop_lsb(&bishops);
        Bitboard bishop_attacks = bishop_attack_bb(bishop_square, pos->current_state->blocker_bitboard) & pos->current_state->checker_bitboard & ~pos->color_bitboard[pos->color_to_move];
        if(only_captures) bishop_attacks &= pos->color_bitboard[~pos->color_to_move];
        while (bishop_attacks)
        {
            Square target = pop_lsb(&bishop_attacks);
            Piece captured = pos->board[target];
            int score = captured ? PIECE_CAPTURE_VALUES[captured] * 10000000 - PIECE_CAPTURE_VALUES[pos->board[bishop_square]] * 1000000 : 0;
            *move_list++ = MoveExt(make_move(bishop_square, target, pos->board[bishop_square], captured), score);
        }
    }

    while (rooks)
    {
        Square rook_square = pop_lsb(&rooks);
        Bitboard rook_attacks = rook_attack_bb(rook_square, pos->current_state->blocker_bitboard) & pos->current_state->checker_bitboard & ~pos->color_bitboard[pos->color_to_move];
        if(only_captures) rook_attacks &= pos->color_bitboard[~pos->color_to_move];
        while (rook_attacks)
        {
            Square target = pop_lsb(&rook_attacks);
            Piece captured = pos->board[target];
            int score = captured ? PIECE_CAPTURE_VALUES[captured] * 10000000 - PIECE_CAPTURE_VALUES[pos->board[rook_square]] * 1000000 : 0;
            *move_list++ = MoveExt(make_move(rook_square, target, pos->board[rook_square], captured), score);
        }
    }

    return move_list;
}

MoveExt *add_knight_moves(Position *pos, MoveExt *move_list, bool only_captures)
{
    Piece our_knight = make_piece(KNIGHT, pos->color_to_move);
    //Pinned knights cannot move
    Bitboard knights = pos->piece_bitboard[our_knight] & ~pos->current_state->pinner_bitboard;

    while (knights)
    {
        Square knight_square = pop_lsb(&knights);
        //We need to evade checks
        Bitboard targets = knight_attack_bb(knight_square) & pos->current_state->checker_bitboard & ~pos->color_bitboard[pos->color_to_move];
        if(only_captures) targets &= pos->color_bitboard[~pos->color_to_move];
        while (targets)
        {
            Square target = pop_lsb(&targets);
            Piece captured = pos->board[target];
            int score = captured ? PIECE_CAPTURE_VALUES[captured] * 10000000 - PIECE_CAPTURE_VALUES[our_knight] * 1000000 : 0;
            *move_list++ = MoveExt(make_move(knight_square, target, our_knight, captured), score);
        }
    }

    return move_list;
}

inline MoveExt *make_promotions(Position *pos, Square from, Square to, MoveExt *move_list)
{
    Piece our_pawn = make_piece(PAWN, pos->color_to_move);
    Piece captured = pos->board[to];
    int score = captured ? PIECE_CAPTURE_VALUES[captured] * 10000000 - PIECE_CAPTURE_VALUES[our_pawn] * 1000000 : 0;
    move_list[0] = MoveExt(make_move_promotion(from, to, our_pawn, captured, QUEEN), score + 4);
    move_list[1] = MoveExt(make_move_promotion(from, to, our_pawn, captured, ROOK), score + 3);
    move_list[2] = MoveExt(make_move_promotion(from, to, our_pawn, captured, BISHOP), score + 2);
    move_list[3] = MoveExt(make_move_promotion(from, to, our_pawn, captured, KNIGHT), score + 1);

    return move_list + 4;
}

MoveExt *add_pawn_moves(Position *pos, MoveExt *move_list, bool only_captures)
{
    //ASSERT(pos->current_state->checker_bitboard);

    Piece our_pawn = make_piece(PAWN, pos->color_to_move);

    Direction forwards = pos->color_to_move == white ? N : S;

    Bitboard en_passent_target = pos->current_state->en_passent != NO_SQUARE ? (1ull << pos->current_state->en_passent) : 0ull;

    Bitboard pawns = pos->piece_bitboard[make_piece(PAWN, pos->color_to_move)];

    //Capture-Moves
    Bitboard westward_attacks = pos->color_to_move == white ? shift<NW>(pawns) : shift<SW>(pawns);
    //We need to evade checks
    westward_attacks &= pos->current_state->checker_bitboard & (pos->color_bitboard[~pos->color_to_move] | en_passent_target);

    Bitboard eastward_attacks = pos->color_to_move == white ? shift<NE>(pawns) : shift<SE>(pawns);
    //We need to evade checks
    eastward_attacks &= pos->current_state->checker_bitboard & (pos->color_bitboard[~pos->color_to_move] | en_passent_target);

    while (westward_attacks)
    {
        Square target = pop_lsb(&westward_attacks);
        Square from = target - forwards - W;
        if (target > 7 && target < 56)
        {
            if (target == pos->current_state->en_passent)
                *move_list++ = MoveExt(make_move_en_passent(from, target, our_pawn), 10000000*PIECE_CAPTURE_VALUES[our_pawn] - 1000000*PIECE_CAPTURE_VALUES[our_pawn]);
            else
            {
                Piece captured = pos->board[target];
                int score = PIECE_CAPTURE_VALUES[captured] * 10000000 - PIECE_CAPTURE_VALUES[our_pawn] * 1000000;
                *move_list++ = MoveExt(make_move(from, target, our_pawn, captured), score);
            }
        }
        else
        {
            move_list = make_promotions(pos, from, target, move_list);
        }
    }

    while (eastward_attacks)
    {
        Square target = pop_lsb(&eastward_attacks);
        Square from = target - forwards - E;
        if (target > 7 && target < 56)
        {
            if (target == pos->current_state->en_passent)
                *move_list++ = MoveExt(make_move_en_passent(from, target, our_pawn), 10000000*PIECE_CAPTURE_VALUES[our_pawn] - 1000000*PIECE_CAPTURE_VALUES[our_pawn]);
            else
            {
                Piece captured = pos->board[target];
                int score = PIECE_CAPTURE_VALUES[captured] * 10000000 - PIECE_CAPTURE_VALUES[our_pawn] * 1000000;
                *move_list++ = MoveExt(make_move(from, target, our_pawn, captured), score);
            }
        }
        else
        {
            move_list = make_promotions(pos, from, target, move_list);
        }
    }

    if(only_captures) return move_list; //Early exit if in only capture mode

    Bitboard forward_once = pos->color_to_move == white ? shift<N>(pawns) : shift<S>(pawns);
    forward_once &= pos->current_state->free;

    Bitboard forward_twice = pos->color_to_move == white ? shift<N>(forward_once) : shift<S>(forward_once);
    forward_twice &= pos->current_state->free & rank_bitboards[pos->color_to_move == white ? 3 * 8 : 4 * 8];

    forward_once &= pos->current_state->checker_bitboard;
    while (forward_once)
    {
        Square target = pop_lsb(&forward_once);
        Square from = target - forwards;
        if (target > 7 && target < 56)
        {
            *move_list++ = MoveExt(make_move(from, target, our_pawn, NO_PIECE), 0);
        }
        else
        {
            move_list = make_promotions(pos, from, target, move_list);
        }
    }

    forward_twice &= pos->current_state->checker_bitboard;
    while (forward_twice)
    {
        Square target = pop_lsb(&forward_twice);
        Square from = target - forwards - forwards;
        *move_list++ = MoveExt(make_move_double_pawn(from, target, our_pawn), 0);
    }

    return move_list;
}

MoveExt *generate_moves(Position *pos, MoveExt *move_list, bool only_captures)
{
    //First, add the king moves
    move_list = add_king_moves(pos, move_list, only_captures);

    //if in double check, stop here, because only king moves are valid
    if (pos->current_state->in_double_check)
        return move_list;

    //add slider moves
    move_list = add_slider_moves(pos, move_list, only_captures);

    //add knight moves
    move_list = add_knight_moves(pos, move_list, only_captures);

    //add pawn moves
    move_list = add_pawn_moves(pos, move_list, only_captures);
    
    //if not in check, add casteling moves
    if (!pos->current_state->in_check && !only_captures) //Captures are never casteling moves
    {
        move_list = add_casteling_moves(pos, move_list);
    }

    return move_list;
}