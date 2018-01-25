//=============================================================================
// Clever Girl Chess Engine
// Author: Seth Kasmann
//=============================================================================

#include <iostream>
#include <string>
#include <sstream>
#include "search.h"
#include "test.h"
#include "perft.h"
#include "zobrist.h"
#include "evaluation.h"
#include "uci.h"

int main(int argc, char* argv[])
{
    mg_init();
    Zobrist::init();
    bb_init();
    uci();
    return 0;
} 