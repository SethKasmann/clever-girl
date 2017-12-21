#ifndef HISTORY_H
#define HISTORY_H

#include <iostream>
#include <vector>
#include <algorithm>
#include "state.h"
#include "move.h"
#include "types.h"
#include "state.h"

static const int maxGameMoves = 1024;

class History
{
public:
	History()
	: mKillers{}, mHistory{}
	{
		mGameHistory.reserve(1024);
		for (std::array<int, Board_size>& arr : mButterfly)
			arr.fill(1);
	}
	History(const State& pState)
	: mKillers{}, mHistory{}
	{
		mGameHistory.reserve(1024);
		for (std::array<int, Board_size>& arr : mButterfly)
			arr.fill(1);
		push(std::make_pair(nullMove, pState.getKey()));
	}
	std::size_t size()
	{
		return mGameHistory.size();
	}
	void init()
	{
		mKillers = {};
		mHistory = {};
		mGameHistory.clear();
		for (std::array<int, Board_size>& arr : mButterfly)
			arr.fill(1);
	}
	void clear()
	{
		mKillers = {};
		mHistory = {};
		for (std::array<int, Board_size>& arr : mButterfly)
			arr.fill(1);
	}
	void push(const std::pair<Move, U64>& mPair)
	{
		mGameHistory.push_back(mPair);
	}
	void pop()
	{
		if (!mGameHistory.empty())
			mGameHistory.pop_back();
	}
	bool isThreefoldRepetition(State& pState) const
	{
		if (mGameHistory.size() < 5)
			return false;
		int candidate = mGameHistory.size() - 5;
		if (mGameHistory[candidate].second == mGameHistory.back().second)
			return true;
		return false;
	}
	void update(Move pBest, int pDepth, int ply, bool causedCutoff)
	{
		if (causedCutoff)
		{
			if (mKillers[ply].first != pBest)
			{
				mKillers[ply].second = mKillers[ply].first;
				mKillers[ply].first = pBest;
			}
			mHistory[getSrc(pBest)][getDst(pBest)] += pDepth * pDepth;
		}
		else
			mButterfly[getSrc(pBest)][getDst(pBest)] += pDepth;
	}
	const std::pair<Move, Move>& getKiller(int ply) const
	{
		return mKillers[ply];
	}
	int getHistoryScore(Move move) const
	{
		assert(mButterfly[getSrc(move)][getDst(move)] > 0);
		return mHistory[getSrc(move)][getDst(move)] / mButterfly[getSrc(move)][getDst(move)];
	}
private:
	std::vector<std::pair<Move, U64>> mGameHistory;
	std::array<std::pair<Move, Move>, Max_ply> mKillers;
	std::array<std::array<int, Board_size>, Board_size> mButterfly;
	std::array<std::array<int, Board_size>, Board_size> mHistory;
};

#endif