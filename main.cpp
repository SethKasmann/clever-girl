//=============================================================================
// Clever Girl Chess Engine
// Author: Seth Kasmann
//=============================================================================

#include <iostream>
#include <string>
#include <sstream>
#include "search.h"
//#include "src/book.c"
#include "test.h"
#include "perft.h"
#include "zobrist.h"
#include "evaluation.h"
#include "uci.h"

void init()
{
    mg_init();
    Zobrist::init();
    bb_init();
    search_init();

    // initialize opening book database

    /*
    strcpy(TOERFILE,"db/tourbook.bin");   
    strcpy(BOOKFILE,"db/mainbook.bin");    
    INITIALIZE();
    if (ERROR)
    {
        printf("Something went wrong, error-code %d",ERROR);
        exit(1);
    }
    */
}

// Currently unused function to test opening book
/*
void choose(State & s)
{
    strcpy(EPD, s.get_EPD());
    std::cout << s.get_EPD() << '\n';
    std::cout << s;
    FIND_OPENING();
    if (ERROR)
    {
        printf("Something went wrong, error-code %d",ERROR);
    }
    else
    {
        printf("Move: %s-%s\nList: ",FROM,TO);
        for (int x=0; x<AZ; x++) printf("%c%c-%c%c ",FROM1[x],FROM2[x],TO1[x],TO2[x]);
        printf("\n\n");
    }
}
*/

int main(int argc, char* argv[])
{
    init();
    perftTestDebug();
    //perftTest();
    //ccrTest();
    uci();
    return 0;
}