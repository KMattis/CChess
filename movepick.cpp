#include "movepick.h"
#include "io.h"
#include "bitboards.h"

#include <algorithm>

MovePicker::MovePicker(Position *pos, TranspositionTableEntry *tte, Move killer0, Move killer1, SearchResult *res, bool only_captures)
{
    this->tte = tte;
    this->stage = TT_MOVE;
    this->pos = pos;
    this->killer0 = killer0;
    this->killer1 = killer1;
    this->res = res;
    this->only_captures = only_captures;
}

MovePicker::~MovePicker()
{
    if(this->list != nullptr) delete this->list;
}

Move MovePicker::next_move()
{
    start:
    switch(stage)
    {
        case TT_MOVE:
            stage = KILLER_MOVE_0;
            if(tte != nullptr && tte->pv_move != NO_MOVE)
            {
                if(only_captures && (!captured_piece(tte->pv_move))) goto start;
                if(pos->is_legal(tte->pv_move) && pos->is_pseudo_legal(tte->pv_move))
                {
                    num_legal_moves++;
                    return tte->pv_move;
                }
            }
            goto start;
        case KILLER_MOVE_0:
            stage = KILLER_MOVE_1;
            if(only_captures && !captured_piece(killer0)) goto start;
            if(killer0 != NO_MOVE && (tte == nullptr || tte->pv_move != killer0) && pos->is_legal(killer0) && pos->is_pseudo_legal(killer0))
            {
                num_legal_moves++;
                return killer0;
            }
            goto start;
        case KILLER_MOVE_1:
            stage = MOVE_LIST_INIT;
            if(only_captures && !captured_piece(killer1)) goto start;
            if(killer1 != NO_MOVE && killer0 != killer1 && (tte == nullptr || tte->pv_move != killer1) && pos->is_legal(killer1) && pos->is_pseudo_legal(killer1))
            {
                num_legal_moves++;
                return killer1;
            }
            goto start;
        case MOVE_LIST_INIT:
            stage = MOVE_LIST;
            if(list != nullptr) goto start;
            list = new MoveList(pos, this->only_captures);
            move_num = -1;
            for(int index = 0; index < list->size; index++)
            {      
                Move move = list->moveList[index].move;
                list->moveList[index].score += res->CutoffHistory[from_square(move)][to_square(move)];
            }
            std::sort(list->moveList, list->moveList + list->size, [](const MoveExt &a, const MoveExt &b ) { return a.score > b.score; });
            goto start;
        case MOVE_LIST:
            move_num++;
            if(move_num >= list->size) return NO_MOVE;
            Move move = list->moveList[move_num].move;
            if((tte != nullptr && tte->pv_move == move) || move == killer0 || move == killer1)
                //Skip this move, we have already searched it   
                goto start;
            #ifdef DEBUG //In debug move, check if move is pseudo legal
            if(!pos->is_pseudo_legal(move)) 
            {
                printf("----BEGIN----\n");
                printf("Move: %s, is_casteling: %s, moved_piece: %i, captured_piece: %i, piece on from: %i, piece on to: %i, color_to_move: %i, from: %i, to: %i\n", 
                        io::move_to_string(move), 
                        is_casteling(move) ? "true" : "false", 
                        moved_piece(move), 
                        captured_piece(move),
                        pos->board[from_square(move)],
                        pos->board[to_square(move)],
                        pos->color_to_move,
                        from_square(move),
                        to_square(move));
                print_bitboard(pos->current_state->attack_bitboards[~pos->color_to_move]);
                printf("\n");
                print_bitboard(pos->current_state->blocker_bitboard);
                printf("\n");
                printf("-----END-----\n");
            }
            #endif
            if(pos->is_legal(move))
            {
                num_legal_moves++;
                return move;
            }
            goto start;
    }
    return NO_MOVE;
}