#include <stdio.h>
#include <string.h>

#include <chrono>

#include "uci.h"
#include "position.h"
#include "movegen.h"
#include "search.h"



#define INPUTBUFFER 400 * 6

namespace uci
{
    char *move_to_string(Move move) {
        static char mv_str[6]; // = (char *) malloc(6);
        char pchar;

        Square from = from_square(move);
        Square to = to_square(move);
        int ff = from % 8;
        int rf = from / 8;
        int ft = to % 8;
        int rt = to / 8;


        if(is_promotion(move)) {
           int promoted = promoted_piece(move);
            if(promoted == QUEEN) pchar = 'q';
            if(promoted == ROOK) pchar = 'r';
            if(promoted == BISHOP) pchar = 'b';
            if(promoted == KNIGHT) pchar = 'n';
            sprintf(mv_str, "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
        } else {
            sprintf(mv_str, "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));
        }

        return mv_str;
    }

    void send_best_move(Move best_move)
    {
        printf("bestmove %s\n", uci::move_to_string(best_move));
        fflush(stdout);
    }

    void send_depth_info(SearchResult *res, int nps)
    {
        char *pv_string = (char *) malloc(2048);
        sprintf(pv_string, "%s", move_to_string(res->pv[0]));
        for(int i = 1; i < res->pv_length; i++)
        {
            sprintf(pv_string, "%s %s", pv_string, move_to_string(res->pv[i]));
        }

        printf("info depth %i nodes %li score cp %i pv %s nps %i\n", res->search_depth, res->nodes, res->score, pv_string, nps);
        fflush(stdout);

        free(pv_string);
    }

    void send_move_info(int move_num, Move move, Depth depth)
    {
        printf("info currmovenumber %i currmove %s depth %i\n", move_num, move_to_string(move), depth);
        fflush(stdout);
    }

    void send_hashtable_info(float percentage)
    {
        printf("info hashfull %i\n", (int)(percentage * 1000));
        fflush(stdout);  
    }


    void init()
    {
        printf("id name CHESS-TEST-V1\n");
        printf("id author Klaus Mattis\n");   
        printf("uciok\n");
    }

    string startpos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w QKqk - 1 0");

    void parse_and_make_move(char *ptr_char, Position *pos) {
        int from, to;

        if(ptr_char[1] > '8' || ptr_char[1] < '1') return;
        if(ptr_char[3] > '8' || ptr_char[3] < '1') return;
        if(ptr_char[0] > 'h' || ptr_char[0] < 'a') return;
        if(ptr_char[2] > 'h' || ptr_char[2] < 'a') return;

        from = (ptr_char[0] - 'a') + 8 * (ptr_char[1] - '1');
        to   = (ptr_char[2] - 'a') + 8 * (ptr_char[3] - '1');

        PieceType promoted = NO_PIECE_TYPE;
        if(ptr_char[4] == 'q') promoted = QUEEN;
        if(ptr_char[4] == 'r') promoted = ROOK;
        if(ptr_char[4] == 'n') promoted = KNIGHT;
        if(ptr_char[4] == 'b') promoted = BISHOP;

        MoveList moves(pos);
        for(int i = 0; i < moves.size; i++)
        {
            if(from_square(moves.moveList[i].move) == from && to_square(moves.moveList[i].move) == to)
            {   
                if((!is_promotion(moves.moveList[i].move) && promoted == NO_PIECE_TYPE) || promoted_piece(moves.moveList[i].move) == promoted)
                {
                    pos->do_move(moves.moveList[i].move);
                    return;
                }
            }
        }
        printf("Move not found! Maybe not valid? %s\n", move_to_string(make_move(from, to, NO_PIECE, NO_PIECE)));
    }

    void parse_pos(char *line, Position *pos)
    {
        char *ptr_char = line + 9;

        line = ptr_char;

        if(strncmp(line, "startpos", 8) == 0) {
            pos->init(startpos);
        } else {
            //NOT SUPPORTED
            printf("unsupported position type");
        }

        ptr_char = strstr(line, "moves");

        if(ptr_char != NULL) {
            ptr_char += 6;
            while(*ptr_char) {
                parse_and_make_move(ptr_char, pos);
                while(*ptr_char && *ptr_char != ' ')
                    ptr_char++;
                ptr_char++;
            }
        }
    }

    void go(Position *pos)
    {
        //Todo read timeleft
        do_search(3, 40, pos, 10000);
    }

    void loop()
    {
        char line[INPUTBUFFER];

        Position *pos = new Position();
        parse_pos("position startpos\n", pos);

        while(true)
        {
            fflush(stdout);
            memset(&line[0], 0, sizeof(line));
            if(!fgets(line, INPUTBUFFER, stdin))
                continue;

            if(line[0] == '\n')
                continue;

            if (!strncmp(line, "isready", 7)) {
                printf("readyok\n");
                continue;
            } else if (!strncmp(line, "position", 8)) {
                delete pos;
                pos = new Position();
                parse_pos(line, pos);
            } else if (!strncmp(line, "ucinewgame", 10)) {
                delete pos;
                pos = new Position();
                parse_pos("position startpos\n", pos);
            } else if (!strncmp(line, "go", 2)) {
                //Do search
                go(pos);
            } else if (!strncmp(line, "quit", 4)) {
                break;
            }
        }

        delete pos;
    }

}