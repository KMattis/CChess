#include <chrono>
#include <thread>

#include "search.h"
#include "evaluation.h"
#include "movegen.h"
#include "movepick.h"
#include "material.h"
#include "tt.h"
#include "uci.h"

const int NULL_MOVE_DEPTH_REDUCTION = 3;

TranspositionTable *tt = nullptr;

/*void pick_init(MoveList *list, Position *pos, SearchResult *res, TranspositionTableEntry *tte)
{
    for(int index = 0; index < list->size; index++)
    {
        Move move = list->moveList[index].move;
        list->moveList[index].score += res->CutoffHistory[from_square(move)][to_square(move)];
        
        if(tte != nullptr && move == tte->pv_move)
            list->moveList[index].score = 1000000000; //PV or refutation moves get searched first
        else if(move == res->killers[0][pos->current_state->ply])
            list->moveList[index].score =  90000;
        else if(move == res->killers[1][pos->current_state->ply])
            list->moveList[index].score =  80000;
    }
}*/

void add_killer(Position *pos, SearchResult *res, Move killer)
{
    res->killers[1][pos->current_state->ply] = res->killers[0][pos->current_state->ply];
    res->killers[0][pos->current_state->ply] = killer;
}

/*void pick_next_move(int move_num, MoveList *list) {
	MoveExt temp;
	int index = 0;
	int bestScore = -10000000;
	int bestNum = -1;

	for(index = move_num; index < list->size; index++) {
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
}*/

void do_search(Depth min_depth, Depth max_depth, Position *pos, int timeleft)
{
    using namespace std::chrono;
    long int total_nodes = 0;
    
    if(tt == nullptr)
        tt = new TranspositionTable(1 << 25); //512 MB 

    SearchResult *res = new SearchResult();
    res->start_ply = pos->current_state->ply;
    res->aborted = false;

    //Abort the search after timeleft milliseconds
    std::thread t([=]() {
        std::this_thread::sleep_for(milliseconds(timeleft));
        res->aborted = true;
    });
    t.detach();

    high_resolution_clock::time_point start = high_resolution_clock::now();
    for(int depth = min_depth; depth < max_depth; depth++)
    {
        res->search_depth = depth;

        int delta = 17;
        int alpha = depth == min_depth ? -INFINITY : res->score - delta;
        int beta = depth == min_depth ? INFINITY : res->score + delta;

        while(true) {
            //Reset search result data
            res->fh = 0;
            res->fhf = 0;
            res->nodes = 0;

            res->score = search<PV>(depth, alpha, beta, pos, res);
            total_nodes += res->nodes;
            
            if(alpha < res->score && res->score < beta)
            {
                break;
            }
            delta = delta * 5 / 4;
            alpha = res->score - delta;
            beta = res->score + delta;
            printf("Aspiration window failed, new delta: %i, score:%i\n", delta, res->score);
        }

        if(!res->aborted)
            //Setup pv for next iteration
            res->pv_length = tt->find_pv(pos, res->pv);

        high_resolution_clock::time_point end = high_resolution_clock::now();
        duration<double, std::milli> time_span = end - start;

        if(!res->aborted)
        {
            uci::send_depth_info(res, (int) 1000 * (total_nodes / time_span.count()));
            uci::send_hashtable_info(tt->get_used_percentage());
            printf("Used: %i, Collisions: %i, Hits: %i\n", tt->get_used(), tt->collissions, tt->hits);
            //printf("info string hashcolissions %i used %i hits %i\n", tt->collissions, tt->get_used(), tt->hits);
            //printf("info string ordering %.2f\n", res->fhf / (float) res->fh);
        }

        //TODO calculate remaining time from timeleft
        if(res->aborted) {
            break;
        }
    }

    uci::send_best_move(res->pv[0]);
    pos->do_move(res->pv[0]);

    //Free the tt memory
    //delete tt;
    delete res;
}

template<NodeType T>
int search(Depth depth, int alpha, int beta, Position *pos, SearchResult *res)
{
    if(res->aborted)
        return DRAW;
    
    //50 moves rule draw detection
    if(pos->current_state->fifty_moves > 100)
    {
        return DRAW;
    }

    State *s = pos->current_state;
    Key current_key = s->position_key;

    int num_repetitions = 0;
    while(s->last_state != nullptr)
    {
        s = s->last_state;
        if(s->position_key == current_key)
            num_repetitions++;
        if(num_repetitions >= 2)
            return DRAW;
    }


    if(depth <= 0 || pos->current_state->ply - res->start_ply >= 2 * res->search_depth)
    {
        return qsearch(alpha, beta, pos, res);
    }

    res->nodes++;

    //Tablebase probe. TODO is this correct?
    TranspositionTableEntry *tte = tt->get_entry(pos->current_state->position_key);
    int ttScore = tte == nullptr ? LOOKUP_FAILED : tte->get_score(alpha, beta, depth);
    if(ttScore != LOOKUP_FAILED)
    {
        if(ttScore >= beta)
            return beta;
        if(ttScore > alpha)
            alpha = ttScore;
    }

    //Futility pruning
    if(depth < 9) //Futility prune at shallow depths
    {
        int current_score = evaluate(pos);
        if(current_score + (200 * depth) + 100 < alpha)
        {
            //Eval is bad enough that we can prune
            return qsearch(alpha, beta, pos, res);
        }
    }

    //Null move pruning (only if not in check)
    if(!pos->current_state->in_check //We are not in check
        && depth > NULL_MOVE_DEPTH_REDUCTION + 1 //We are not too shallow, so we can actually reduce the depth
        && pos->current_state->move != 0 //Last move was not a nullmove
    )
    {
        pos->do_null_move();
        //Do ZW-search after null move, we only want to know if result is above beta
        int score = -search<Cut>(depth - NULL_MOVE_DEPTH_REDUCTION - 1, -beta, -beta+1, pos, res);
        pos->undo_null_move();

        if(score >= beta)
        {
            tt->store(pos->current_state->position_key, LowerBound, beta, NO_MOVE, depth, res->start_ply);
            return beta;
        }
    }

    bool pv_search = true;
    MovePicker mp(pos, tte, res->killers[0][pos->current_state->ply], res->killers[1][pos->current_state->ply], res);

    //Multicut
    if (depth >= 5 && T == Cut) 
    {
        int c = 0;
        Move move;
        while(mp.legal_moves() < 6 && (move = mp.next_move()) != NO_MOVE)
        {
            int score = -search<All>(depth-1-3, -beta, -beta + 1, pos, res);
            if (score >= beta) 
            {
                if (++c == 3)
                {
                    return beta; // mc-prune
                }
            }
        }
        mp.reset();
    }

    Move move, best_move = NO_MOVE;
    while((move = mp.next_move()) != NO_MOVE)
    {
        if(res->start_ply == pos->current_state->ply)
        {
            uci::send_move_info(mp.legal_moves(), move, res->search_depth);
        }

        pos->do_move(move);
        int score;

        Depth reduction = 0;

        if(    mp.legal_moves() > 4          //Reduce moves located at the end of the move ordering
            && !pos->current_state->in_check //Do not reduce while in check
            && !captured_piece(move)         //Do not reduce captures
            && !is_promotion(move)           //Do not reduce promotions
            && alpha + 1 == beta             //Do only reduce in Scout searches
            && depth >= 4                    //Do not reduce at shallow depths
        )
        {
            //TODO is this good?
            if(mp.legal_moves() < 10) reduction++;
            else reduction += depth / 3;
        }

        //extensions for interesting moves
        Piece captured_piece = captured_piece(move);
        if(captured_piece == WHITE_QUEEN || captured_piece == BLACK_QUEEN) reduction -= 2;
        if(captured_piece == WHITE_ROOK  || captured_piece == BLACK_ROOK) reduction --;

        if(pos->current_state->in_check) reduction --;

        if(pv_search || alpha + 1 == beta || depth <= 2)
        {
            //Do not scout in pv_search mode, in ZW-Search or at shallow depths
            score = -search<(NodeType)-T>(depth - 1 - reduction, -beta, -alpha, pos, res);
        }
        else
        {
            //Do a scout search if we already found a good move
            score = -search<Cut>(depth - 1 - reduction, -alpha-1, -alpha, pos, res);
            if(score > alpha)
                score = -search<(NodeType)-T>(depth - 1 - reduction, -beta, -alpha, pos, res);
        }

        pos->undo_move();

        if(score >= beta)
        {
            res->fh++;
            if(mp.legal_moves() == 1)
                res->fhf++;
            res->CutoffHistory[from_square(move)][to_square(move)]++;
            add_killer(pos, res, move);
            tt->store(pos->current_state->position_key, LowerBound, beta, move, depth, res->start_ply);
            return beta;
        }
        else if(score > alpha)
        {
            alpha = score;
            pv_search = false;
            best_move = move;
        }
    }

    if(!mp.legal_moves())
    {
        //Checkmate if check, otherwise stalemate
        if(pos->current_state->in_check)
        {
            return -CHECKMATE + pos->current_state->ply;
        }
        else
        {
            return DRAW;
        }
    }

    //Tablebase storing
    tt->store(pos->current_state->position_key, 
              best_move == NO_MOVE ? UpperBound : Exact, 
              alpha, 
              best_move, 
              depth, 
              res->start_ply);

    return alpha;
}

int qsearch(int alpha, int beta, Position *pos, SearchResult *res)
{
    if(res->aborted)
        return DRAW;
    
    res->nodes++;
    //We need no 50 moves check, because every move in qsearch is a capture or a pawn move

    TranspositionTableEntry *tte = tt->get_entry(pos->current_state->position_key);
    int ttScore = tte == nullptr ? LOOKUP_FAILED : tte->get_score(alpha, beta, -1);
    if(ttScore != LOOKUP_FAILED)
    {
        if(ttScore >= beta)
            return beta;
        if(ttScore > alpha)
            alpha = ttScore;
    }

    //Stand pat
    int stand_pat = evaluate(pos);
    if(stand_pat >= beta)
    {
        //tt->store(pos->current_state->position_key, LowerBound, beta, NO_MOVE, -1);
        return beta; //TODO: If stand pat is too good, we do not make a checkmate check
    }
    else if(stand_pat > alpha)
        alpha = stand_pat;
    else if(stand_pat + 900 < alpha) //TODO: get queen value from evaluation data
        //If our evaluation plus a queen is worse than alpha, we will not improve that much!
        //So we can safely return alpha. TODO: Not in endgames
        return alpha;
    
    MovePicker mp(pos, tte, res->killers[0][pos->current_state->ply], res->killers[1][pos->current_state->ply], res);

    int num_searched_moves = 0;

    Move best_move = NO_MOVE;
    Move move;
    //Loop over all legal capture or promotion moves
    while((move = mp.next_move()) != NO_MOVE)
    {
        Piece captured = captured_piece(move);
        if(captured || is_promotion(move))
        {
            num_searched_moves++;

            if(captured && stand_pat + 200 + material::PIECE_VALUES[piece_type_of(captured)] < alpha)
                continue;

            //Search captures or promotions
            pos->do_move(move);
            int score = -qsearch(-beta, -alpha, pos, res);
            pos->undo_move();

            if(score >= beta)
            {
                res->fh++;
                if(num_searched_moves == 1)
                    res->fhf++;
                res->CutoffHistory[from_square(move)][to_square(move)]++;
                add_killer(pos, res, move);
                tt->store(pos->current_state->position_key, LowerBound, beta, move, -1, res->start_ply);
                return beta;
            }
            else if(score > alpha)
            {
                alpha = score;
                best_move = move;
            }
        }
    }

    if(!mp.legal_moves())
    {
        //Checkmate if check, otherwise stalemate
        if(pos->current_state->in_check)
        {
            return -CHECKMATE + pos->current_state->ply;
        }
        else
        {
            return DRAW;
        }
    }

    tt->store(pos->current_state->position_key, 
            best_move == NO_MOVE ? UpperBound : Exact, 
            alpha, 
            best_move, 
            -1, 
            res->start_ply);

    return alpha;
}