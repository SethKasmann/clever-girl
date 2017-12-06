#ifndef PERFT_H
#define PERFT_H


#include <ctime>
#include <string>
#include <iomanip>
#include "move_generator.h"
#include "state.h"
#include "types.h"
#include "timer.h"
#include "move.h"

void perftTest();
void perftTestDebug();
void printPerft();

#endif