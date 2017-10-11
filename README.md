# Clever Girl  
Clever Girl is a UCI chess engine written in c++.

## Move Generation  
Uses copy/make move generation vs. make/unmake. Clocked 120,000,000 nodes/second using bulk counting on a I7-4770k processor.

## Search  
- Negamax framework.  
- Alpha-Beta Pruning. 
- Scout search.  
- Q search.  
- Killer Heuristic.  
- Transposition Tables.  

## Evaluation  
- Uses Adam Hair's piece square tables.  
- Evaluates pawn structure including isolated, doubled, connected, or passed pawns.
- Lasker's system for piece weights.
- King safety tables from Stockfish.
- Evlautes bishop and knight outposts.  

TODO: Endgame detection.

## Bugs/Errors  
Please send any bugs to: sakasmann1@cougars.ccis.edu
