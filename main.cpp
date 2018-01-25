//=============================================================================
// Clever Girl Chess Engine
// Author: Seth Kasmann
//=============================================================================

#include <iostream>
#include <string>
#include <sstream>
#include "src/search.h"
#include "src/test.h"
#include "src/perft.h"
#include "src/zobrist.h"
#include "src/evaluation.h"
#include "src/uci.h"

int main(int argc, char* argv[])
{
    mg_init();
    Zobrist::init();
    bb_init();
    uci();
    return 0;
} 