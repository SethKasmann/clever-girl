## Clever Girl  
Clever Girl is a UCI chess engine written in c++.

# Move Generation  
Uses copy/make move generation vs. make/unmake. Clocked 120,000,000 nodes/second using bulk counting on a I7-4770k processor.

# Search  
- Negamax framework.  
- Alpha-Beta Pruning. 
- Scout search.  
- Q search.  
- Killer Heuristic.  
- Transposition Tables.  

# Evaluation  
- Uses Adam Hair's piece square tables.  
- Weights pawn structure including isolated, doubled, connected, or passed pawns.
- Weights piece count using standard weights for pieces.
- Weights king safety.
- Weights bishop and knight outposts.  
- Weights 

TODO: Endgame detection.

# Bugs/Errors  
Please send any bugs to: sakasmann1@cougars.ccis.edu
