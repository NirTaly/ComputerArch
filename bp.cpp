/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include <stdexcept>
#include "bp_api.h"

/*********************************************************************************************/
/*									Classes													 */
/*********************************************************************************************/

enum using_share_enum {no_share,lsb_share,mid_share};

static unsigned log2(unsigned n)
{
	int count = 0;
	for (; n != 0; count++, n>>=1) { }
	return count;
}

class History
{
public:
	History(unsigned btbSize, unsigned historySize, unsigned tagSize, bool isGlobalHist, int Shared)
		: isGlobalHist(isGlobalHist), historySize(historySize), tagSize(tagSize), 
			btbSize(btbSize), shared(using_share_enum(Shared)), histo_mask(UINT32_MAX >> (32-historySize))
	{
		if (historySize < 1 || historySize > 8 || tagSize > 30-log2(btbSize))
		{
			throw std::runtime_error("Bad Args");
		}
		
		if (isGlobalHist)
		{
			histo = new uint32_t(0);
		}
		else
		{
			histo = new uint32_t[btbSize]();
			tags = new uint32_t[btbSize]();
		}
	}

	~History()
	{
		delete[] histo;
		if (!isGlobalHist)
			delete[] tags;
	}
	
	// if new branch - init history, return 1
	// else - update relevant history, return 0
	uint32_t update(uint32_t pc, bool isTaken)
	{
		uint32_t tag_mask = UINT32_MAX >> (32-tagSize);
		uint32_t tag = tag_mask & (pc>>(log2(btbSize)+2));
		
		uint32_t btb_mask = UINT32_MAX >> (32-btbSize);
		uint32_t btb = btb_mask & (pc>>2);

		uint32_t retval = 0;
		uint32_t* curr_histo;
		
		getTableIndex(pc, &curr_histo);

		*curr_histo <<=1;
		*curr_histo &= histo_mask;				// to stay in range [0..2^histoSize]
		*curr_histo |= isTaken;

		if (!isGlobalHist && tags[btb] != tag) 	// if new branch or collision
		{
			*curr_histo = 0;
			tags[btb] = tag;
			
			retval = 1;
		}
		
		return retval;
	}

	uint32_t getTableIndex(uint32_t pc, uint32_t** curr_histo)
	{
		uint32_t btb_mask = UINT32_MAX >> (32-btbSize);
		uint32_t btb = btb_mask & (pc>>2);

		uint32_t retval = 0;
	
		if (isGlobalHist)
		{
			*curr_histo = histo;
		}
		else
		{
			*curr_histo = histo+btb;
		}

		switch (shared)
		{
		case no_share:
			retval = **curr_histo;
			break;
		case lsb_share:
			retval = (**curr_histo ^ (pc>>2)) & histo_mask;
			break;
		case mid_share:
			retval = (**curr_histo ^ (pc>>16)) & histo_mask;
			break;
		default:
			throw(std::runtime_error("Bad Shared Arg"));
			break;
		}

		return retval;
	}

	

private:
	bool isGlobalHist;
	unsigned historySize;
	unsigned tagSize;
	unsigned btbSize;
	using_share_enum shared;

	uint32_t histo_mask;	// to keep histo size correct to histoSize
	uint32_t* histo;
	uint32_t* tags;
};

/*********************************************************************************************/
// class Tables
// {
// public:
// 	Tables(/* args */);
// 	~Tables();
// private:
// };
/*********************************************************************************************/

class BP
{
public:
	BP();
	~BP();
private:

	History history;
	// Tables tables;
};


/*********************************************************************************************/
/*********************************************************************************************/

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

