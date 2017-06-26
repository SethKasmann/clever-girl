#ifndef PERFT_H
#define PERFT_H

#include <ctime>
#include <string>
#include "move_generator.h"
#include "state.h"
#include "types.h"

extern int nodes;
extern int this_node;
void perft_test();
void perft_tree(State & s, int depth);
void perft(std::string & fen, int depth);

#endif