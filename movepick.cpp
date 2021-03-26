#include "movepick.h"

MovePicker::MovePicker(Position *pos, TranspositionTableEntry *tte, Move killer0, Move killer1, SearchResult *res)
{
    this->tte = tte;
    this->stage = TT_MOVE;
    this->pos = pos;
    this->killer0 = killer0;
    this->killer1 = killer1;
    this->res = res;
}

MovePicker::~MovePicker()
{
    if(this->list != nullptr) delete this->list;
}

void MovePicker::pick_next_move() 
{
	MoveExt temp;
	int bestScore = -10000000;
	int bestNum = -1;

	for(int index = move_num; index < list->size; index++) {
		if(list->moveList[index].score > bestScore) {
			bestScore = list->moveList[index].score;
			bestNum = index;
		}
	}

    ASSERT(bestScore > -10000000);
    ASSERT(bestNum >= 0)
    ASSERT(list->size > 0)
	temp = list->moveList[move_num];
	list->moveList[move_num] = list->moveList[bestNum];
	list->moveList[bestNum] = temp;
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
                if(pos->is_legal(tte->pv_move) && pos->is_pseudo_legal(tte->pv_move))
                {
                    num_legal_moves++;
                    return tte->pv_move;
                }
            }
            goto start;
        case KILLER_MOVE_0:
            stage = KILLER_MOVE_1;
            if(killer0 != NO_MOVE && (tte == nullptr || tte->pv_move != killer0) && pos->is_legal(killer0) && pos->is_pseudo_legal(killer0))
            {
                num_legal_moves++;
                return killer0;
            }
            goto start;
        case KILLER_MOVE_1:
            stage = MOVE_LIST_INIT;
            if(killer1 != NO_MOVE && killer0 != killer1 && (tte == nullptr || tte->pv_move != killer1) && pos->is_legal(killer1) && pos->is_pseudo_legal(killer1))
            {
                num_legal_moves++;
                return killer1;
            }
            goto start;
        case MOVE_LIST_INIT:
            stage = MOVE_LIST;
            if(list != nullptr) goto start;
            list = new MoveList(pos);
            move_num = -1;
            for(int index = 0; index < list->size; index++)
            {      
                Move move = list->moveList[index].move;
                list->moveList[index].score += res->CutoffHistory[from_square(move)][to_square(move)];
            }
            goto start;
        case MOVE_LIST:
            move_num++;
            if(move_num >= list->size) return NO_MOVE;
            pick_next_move();
            Move move = list->moveList[move_num].move;
            if( (tte != nullptr && tte->pv_move == move))
                //Skip this move
                goto start;
            if(pos->is_legal(move))
            {
                num_legal_moves++;
                return move;
            }
            goto start;
    }
}