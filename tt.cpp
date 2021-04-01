#include "tt.h"
#include "movegen.h"
#include "search.h"

int TranspositionTableEntry::get_score(int alpha, int beta, Depth depth)
{
    if(this->depth < depth)
        return LOOKUP_FAILED;

    switch(this->type)
    {
        case Exact: return this->score;
        case LowerBound: 
            if(this->score >= beta) return this->score;
            else return LOOKUP_FAILED;
        case UpperBound:
            if(this->score <= alpha) return this->score;
            else return LOOKUP_FAILED;
        default:
            throw "Invalid entry type!";
    }
}

TranspositionTableEntry *TranspositionTable::get_entry(Key key)
{
    Key index = key & (this->num_entries - 1);
    TranspositionTableEntry entry = this->data[index];
    if(entry.key == key)
    {
        this->hits++;
        return &(this->data[index]);
    }
    return nullptr;
}


void TranspositionTable::store(Key key, TranspositionTableEntryType type, int score, Move pv_move, Depth depth, int insertion_ply)
{
    Key index = key & (this->num_entries - 1);
    if(this->data[index].key == 0)
    {
        //There is no entry, store the new one
        this->data[index].key = key;
        this->data[index].type = type;
        this->data[index].score = score;
        this->data[index].pv_move = pv_move;
        this->data[index].depth = depth;
        this->data[index].insertion_ply = insertion_ply;
        this->used_entries++;
    }
    else if(this->data[index].key != key)
    {
        //Overwrite existing entry if we searched later or to a deeper depth
        if(insertion_ply > this->data[index].insertion_ply || depth > this->data[index].depth)
        {
            this->data[index].key = key;
            this->data[index].type = type;
            this->data[index].score = score;
            this->data[index].pv_move = pv_move;
            this->data[index].depth = depth;
        }

        this->collissions++;
    }
    else
    { 
        //The old entry and the new one describe the same position
        if(insertion_ply > this->data[index].insertion_ply)
        {
            //Overwrite old data
            this->data[index].key = key;
            this->data[index].type = type;
            this->data[index].score = score;
            this->data[index].pv_move = pv_move;
            this->data[index].depth = depth;
            this->data[index].insertion_ply = insertion_ply;
        }
        else if(depth >= this->data[index].depth)
        {
            //we searched at least as deep as the old entry
            if(type == Exact)
            {
                this->data[index].type = Exact;
                this->data[index].score = score;
                this->data[index].pv_move = pv_move;
                this->data[index].depth = depth;
            }
            else if (type == LowerBound && this->data[index].type == LowerBound && this->data[index].score < score)
            {
                //we found a better lower bound
                this->data[index].score = score;
                this->data[index].pv_move = pv_move;
                this->data[index].depth = depth;
            }
            else if (type == UpperBound && this->data[index].type == UpperBound && this->data[index].score > score)
            {
                //we found a better upper bound
                this->data[index].score = score;
                this->data[index].pv_move = pv_move;
                this->data[index].depth = depth;
            }
            else if (type == LowerBound && this->data[index].type == UpperBound)
            {
                //we prefer LowerBounds over UpperBounds as they lead to cutoffs
                this->data[index].score = score;
                this->data[index].pv_move = pv_move;
                this->data[index].depth = depth;
                this->data[index].type = LowerBound;
            }
        }
    }
}

int TranspositionTable::find_pv(Position *pos, Move *pv)
{
    int num_pv_moves = 0;

    TranspositionTableEntry *entry;
    while((entry = this->get_entry(pos->current_state->position_key)) != nullptr)
    {
        if(entry->type != Exact) break;
        if(entry->pv_move == 0)
        {
            printf("Exact stored no move?\n");
            break;
        }

        //Check if move exists and is legal (to avoid key hash hits)
        MoveList moves(pos, false);
        bool exists = false;
        for(int i = 0; i < moves.size; i++)
            if(moves.moveList[i].move == entry->pv_move)
                exists = true;
        if(!exists || !pos->is_legal(entry->pv_move))
            break;

        num_pv_moves++;
        pos->do_move(entry->pv_move);
        *pv = entry->pv_move;
        pv++;
        if(num_pv_moves >= MAX_PV_LENGTH)
            break; //Only store MAX_PV_LENGTH many pv moves
    }

    for(int i = 0; i < num_pv_moves; i++)
        pos->undo_move();
    return num_pv_moves;
}