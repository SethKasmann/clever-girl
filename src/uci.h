#ifndef UCI_H
#define UCI_H

#include <iostream>
#include <string>
#include <sstream>
#include "state.h"
#include "transpositiontable.h"
#include "types.h"
#include "move_generator.h"
#include "search.h"
#include "timer.h"

const std::string Start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq";

void uci();

#endif