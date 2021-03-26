#include <stdio.h>

#include "bitboards.h"
#include "movegen.h"
#include "search.h"
#include "evaluation.h"
#include "uci.h"
#include "zobrist.h"
#include "tt.h"
#include "material.h"

const char *SQUARE_NAMES[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

void print_move(Move move)
{
    Square from = from_square(move);
    Square to = to_square(move);
    const char *promotions = "nbrq";
    char promotion_char = is_promotion(move) ? promotions[promoted_piece(move) - KNIGHT] : ' ';
    printf("%s%s%c", SQUARE_NAMES[from], SQUARE_NAMES[to], promotion_char);
}

TranspositionTable *ttable;
int i = 0;

int count_positions(int startdepth, int depth, Position *pos)
{
    if(depth == 0) return 1;

    MoveList move_list(pos);

    int count = 0;
    for(int move_num = 0; move_num < move_list.size; move_num++)
    {
        Move move = move_list.moveList[move_num].move;
        if(pos->is_legal(move))
        {
            pos->do_move(move);
           //s printf("%i - %i : %llu\n", from_square(move), to_square(move), pos->current_state->position_key);
            int c = count_positions(startdepth, depth - 1, pos);
            if(depth == startdepth){
                print_move(move);
                printf(": %i, depth: %i \n", c, depth);
            }
            pos->undo_move();

            count += c;
        }
    }
    return count;
}


int main()
{
    init_bitboards();

    zobrist::init();
    material::init();

    uci::init();
    uci::loop();

    /*
    ttable = new TranspositionTable(1<<20);

    Position *pos = new Position();

    string startpos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w QKqk - 1 0");
    string pos5("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    string pos3("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    string pos5_("rnbq111r/pp1Pbkpp/2p5/8/8/P7/1PP1NnPP/RNBQK2R b KQ - 1 8");

    pos->init(pos5);

    for(int depth = 0; depth < 6; depth++)
    {
        i = 0;
        delete ttable;
        ttable = new TranspositionTable(1 << 20);
        printf("Perft depth %i: %i\n", depth, count_positions(-1, depth, pos));
        //SearchResult *res = start_search(depth, pos);
        //printf("Depth: %i, Evaluation: %i, Nodes: %li, Ordering: %.2f\n", depth, res->score, res->nodes, res->fhf / (float)res->fh);
        //delete res;
    }   
    
    delete ttable;
    delete pos;
    */
}
