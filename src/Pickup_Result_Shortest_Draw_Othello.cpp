/*
	Shortest Draw Othello

	@file Pickup_Result_Shortest_Draw_Othello.cpp
		Main file
	@date 2024
	@author Takuto Yamana
	@license GPL-3.0 license
*/

#include <iostream>
#include <unordered_set>
#include <fstream>
#include <string>
#include "engine/board.hpp"
#include "engine/util.hpp"

void init(){
    bit_init();
    mobility_init();
    flip_init();
}

void output_transcript(std::vector<int> &transcript){
    std::cout << "SOLUTION ";
    for (int &move: transcript){
        std::cout << idx_to_coord(move);
    }
    std::cout << std::endl;
    
    for (int &move: transcript){
        std::cerr << idx_to_coord(move);
    }
    std::cerr << std::endl;
    
}

std::string get_unique_transcript(std::string transcript){
    std::string first_move = transcript.substr(0, 2);
    if (first_move == "f5"){
        return transcript;
    }
    std::string res;
    if (first_move == "e6"){
        for (int i = 0; i < transcript.size(); i += 2){
            int x = transcript[i] - 'a';
            int y = transcript[i + 1] - '1';
            res += (char)('a' + y);
            res += (char)('1' + x);
        }
    } else if (first_move == "d3"){
        for (int i = 0; i < transcript.size(); i += 2){
            int x = transcript[i] - 'a';
            int y = transcript[i + 1] - '1';
            res += (char)('a' + 7 - y);
            res += (char)('1' + 7 - x);
        }
    } else if (first_move == "c4"){
        for (int i = 0; i < transcript.size(); i += 2){
            int x = transcript[i] - 'a';
            int y = transcript[i + 1] - '1';
            res += (char)('a' + 7 - x);
            res += (char)('1' + 7 - y);
        }
    }
    return res;
}

inline void first_update_unique_board(Board *res, Board *sym){
    uint64_t vp = vertical_mirror(sym->player);
    uint64_t vo = vertical_mirror(sym->opponent);
    if (res->player > vp || (res->player == vp && res->opponent > vo)){
        res->player = vp;
        res->opponent = vo;
    }
}

inline void update_unique_board(Board *res, Board *sym){
    if (res->player > sym->player || (res->player == sym->player && res->opponent > sym->opponent))
        sym->copy(res);
    uint64_t vp = vertical_mirror(sym->player);
    uint64_t vo = vertical_mirror(sym->opponent);
    if (res->player > vp || (res->player == vp && res->opponent > vo)){
        res->player = vp;
        res->opponent = vo;
    }
}

inline Board get_unique_board(Board b){
    Board res = b;
    first_update_unique_board(&res, &b);
    b.board_black_line_mirror();
    update_unique_board(&res, &b);
    b.board_horizontal_mirror();
    update_unique_board(&res, &b);
    b.board_white_line_mirror();
    update_unique_board(&res, &b);
    return res;
}

Board play_board(std::string transcript){
    Board board;
    board.reset();
    Flip flip;
    for (int i = 0; i < transcript.size(); i += 2){
        int x = transcript[i] - 'a';
        int y = transcript[i + 1] - '1';
        int coord = y * HW + x;
        calc_flip(&flip, &board, coord);
        board.move_board(&flip);
    }
    return board;
}

int main(int argc, char* argv[]){
    init();

    std::ifstream ifs("result_all_pickup.txt");

    int n_ans_in;
    ifs >> n_ans_in;
    std::vector<std::pair<Board, std::unordered_set<std::string>>> data;
    for (int i = 0; i < n_ans_in; ++i){
        std::string transcript;
        ifs >> transcript;
        std::string unique_transcript = get_unique_transcript(transcript);
        Board unique_board = get_unique_board(play_board(unique_transcript));
        bool pushed = false;
        for (std::pair<Board, std::unordered_set<std::string>> &datum: data){
            if (datum.first == unique_board){
                datum.second.emplace(unique_transcript);
                pushed = true;
                break;
            }
        }
        if (!pushed){
            std::unordered_set<std::string> set;
            set.emplace(unique_transcript);
            data.emplace_back(std::make_pair(unique_board, set));
        }
    }
    int n_transcript = 0;
    for (std::pair<Board, std::unordered_set<std::string>> &datum: data){
        n_transcript += datum.second.size();
    }
    std::cout << "unique transcript: " << n_transcript << " unique board: " << data.size() << std::endl;

    for (std::pair<Board, std::unordered_set<std::string>> &datum: data){
        Board board = play_board(*datum.second.begin());
        board.print();
        for (std::string t: datum.second){
            std::cout << t << std::endl;
        }
    }
    
    return 0;
}
