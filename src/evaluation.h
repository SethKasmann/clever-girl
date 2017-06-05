#ifndef EVALUATION_H
#define EVALUATION_H

#include <iostream>
#include "state.h"
#include "pst.h"

static const int PawnWt   = 100;
static const int KnightWt = 320;
static const int BishopWt = 325;
static const int RookWt   = 500;
static const int QueenWt  = 975;

static const int Checkmate = 32767;
static const int Stalemate = 0;
static const int Draw = 0;

static const int Isolated = -15;
static const int Passed   = 15;

static const int SafetyTable[100] = 
{
   0,   0,   0,   1,   1,   2,   3,   4,   5,   6,
   8,  10,  13,  16,  20,  25,  30,  36,  42,  48,
  55,  62,  70,  80,  90, 100, 110, 120, 130, 140,
 150, 160, 170, 180, 190, 200, 210, 220, 230, 240,
 250, 260, 270, 280, 290, 300, 310, 320, 330, 340,
 350, 360, 370, 380, 390, 400, 410, 420, 430, 440,
 450, 460, 470, 480, 490, 500, 510, 520, 530, 540,
 550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
 650, 650, 650, 650, 650, 650, 650, 650, 650, 650,
 650, 650, 650, 650, 650, 650, 650, 650, 650, 650
};

int evaluate(const State & s);

#endif