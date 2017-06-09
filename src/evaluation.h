#ifndef EVALUATION_H
#define EVALUATION_H

#include <iostream>
#include "state.h"
#include "pst.h"

static const int Pawn_wt   = 100;
static const int Knight_wt = 300;
static const int Bishop_wt = 300;
static const int Rook_wt   = 500;
static const int Queen_wt  = 950;

static const int Knight_th = 2;
static const int Bishop_th = 2;
static const int Rook_th   = 3;
static const int Queen_th  = 5;

static const int Checkmate = 32767;
static const int Stalemate = 0;
static const int Draw = 0;

static const int Passed    = 15;
static const int Connected = 15;
static const int Isolated  = -15;
static const int Doubled   = -10;

static const int Safety_table[100] = 
{
	  0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
	 18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
	 68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
	140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
	260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
	377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
	494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

int evaluate(const State & s);

#endif