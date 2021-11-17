/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include <vector>
#include <stdexcept>

#include "bp_api.h"

const int NUM_OF_GLOBAL_TABLE = 1;
const int GLOBAL_TABLE = 0;	//index in table
const int TARGET_SIZE = 30;
// const int EXE_FLUSH = 3;
const int VALID_BIT_SIZE = 1;

static uint32_t log2(uint32_t n)
{
	int count = 0;
	for (; n != 0; count++, n>>=1) { }
	return count-1;
}

/*********************************************************************************************/
/*									Classes													 */
/*********************************************************************************************/

enum using_share_enum {no_share,lsb_share,mid_share};

class BTB
{
public:
	BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, bool isGlobalHist, int Shared);
	~BTB();
	
	/**
	 * @brief in Exec - update BTB after actual instruction
	 * @param pc 
	 * @param isTaken actual desicion of op.
	 * @param target_pc actual target pc from op
	 */
	void update(uint32_t pc, bool isTaken, uint32_t target_pc);

	/**
	 * @brief Get the Tables Index - dont check whether <pc> is currently in btb
	 * 			so need to call Function predict first
	 * @param pc 
	 * @return uint32_t 
	 */
	uint32_t getTableIndex(uint32_t pc);

	/**
	 * @brief check if pc is branch, if no/new - <target_pc> = pc+4, <histo> = 0
	 * 				else, <target_pc> = next pc, <histo> = curr histo
	 * @param pc 
	 * @param target_pc 
	 * @param histo 
	 * @return true if is known branch, false if no/new branch
	 */
	uint32_t predictTarget(uint32_t pc);

	/**
	 * @brief check if pc is known branch using tag
	 * @param pc 
	 * @return true if known, false if unknown
	 */
	bool isKnownBranch(uint32_t pc);
	
	/**
	 * @brief find histo fo pc - if unknown branch - return 0 (for safety check isKnownBranch() first)
	 * for global history - return curr histo, for loacl history - return histo[btb_i]
	 * @param pc 
	 * @return uint32_t history
	 */
	uint32_t* findCurrHisto(uint32_t pc);
	
	/**
	 * @brief get btb index out of pc, using bit manipulation
	 * @param pc 
	 * @return uint32_t btb_index
	 */
	uint32_t getBTBIndex(uint32_t pc);
	uint32_t getTag(uint32_t pc);

private:
	bool isGlobalHist;
	unsigned historySize;
	unsigned tagSize;
	unsigned btbSize;
	using_share_enum shared;

	uint32_t histo_mask;	// to keep histo size correct to histoSize
	
	uint32_t* histo;
	uint32_t* tags;
	uint32_t* targets;
	bool* valid_bits;
};
/*****************************************************************************************************************/
BTB::BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, bool isGlobalHist, int Shared) :
		isGlobalHist(isGlobalHist), historySize(historySize), tagSize(tagSize), 
			btbSize(btbSize), shared(using_share_enum(Shared)), histo_mask(UINT32_MAX >> (32-historySize)),
				histo(nullptr), tags(nullptr), targets(nullptr), valid_bits(nullptr)
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
	}

	valid_bits = new bool[btbSize]();
	tags = new uint32_t[btbSize]();
	targets = new uint32_t[btbSize]();
}
/*****************************************************************************************************************/
BTB::~BTB()
{
	if (!isGlobalHist)
	{
		delete[] histo;
	}
	else
	{
		delete histo;
	}

	delete[] tags;
	delete[] targets;
	delete[] valid_bits;
}
/*****************************************************************************************************************/
void BTB::update(uint32_t pc, bool isTaken, uint32_t target_pc)
{
	uint32_t tag = getTag(pc);
	uint32_t btb_i = getBTBIndex(pc);
	uint32_t* curr_histo = findCurrHisto(pc);

	if (isKnownBranch(pc) || isGlobalHist)
	{
		*curr_histo <<=1;
		*curr_histo &= histo_mask;				// to stay in range [0..2^histoSize]
		*curr_histo |= isTaken;
	}
	else
	{
		*curr_histo = (uint32_t)isTaken; // (isTaken ? 1 : 0)
	}

	tags[btb_i] = tag;
	valid_bits[btb_i] = true;
	targets[btb_i] = target_pc;
}
/*****************************************************************************************************************/
uint32_t BTB::getTableIndex(uint32_t pc)
{
	uint32_t retval = 0;
	uint32_t* curr_histo = findCurrHisto(pc);

	if (!isKnownBranch(pc) && !isGlobalHist)
	{
		*curr_histo = 0;
	}

	switch (shared)
	{
	case no_share:
		retval = *curr_histo;
		break;
	case lsb_share:
		retval = (*curr_histo ^ (pc>>2)) & histo_mask;
		break;
	case mid_share:
		retval = (*curr_histo ^ (pc>>16)) & histo_mask;
		break;
	default:
		throw(std::runtime_error("Bad Shared Arg"));
		break;
	}

	return retval;
}

uint32_t BTB::getTag(uint32_t pc)
{
	uint32_t tag_mask = UINT32_MAX >> (32-tagSize);
	uint32_t tag = tag_mask & (pc>>(log2(btbSize)+2));

	if (tagSize == 0)
		tag = 0;
	
	return tag;
}
/*****************************************************************************************************************/
uint32_t BTB::predictTarget(uint32_t pc) 
{	
	uint32_t predict_target = pc+4;
	if (isKnownBranch(pc))
	{
		predict_target = targets[getBTBIndex(pc)];
	}

	return predict_target;
}
/*****************************************************************************************************************/
bool BTB::isKnownBranch(uint32_t pc)
{
	uint32_t tag = getTag(pc);
	uint32_t btb_i = getBTBIndex(pc);

	return (valid_bits[btb_i] && tags[btb_i] == tag);
}
/*****************************************************************************************************************/
uint32_t* BTB::findCurrHisto(uint32_t pc)
{
	return (isGlobalHist ? histo : histo + getBTBIndex(pc));
}

/*****************************************************************************************************************/
uint32_t BTB::getBTBIndex(uint32_t pc)
{
	uint32_t log_btb = log2(btbSize);
	
	uint32_t btb_mask = UINT32_MAX >> (32-log_btb);
	uint32_t btb_i = btb_mask & (pc>>2);

	if (0 == log_btb)
		btb_i = 0;
	
	return btb_i;
}

/*********************************************************************************************/
/*********************************************************************************************/

typedef enum {SNT,WNT,WT,ST} fsm_state;

class FSM
{
private:
	fsm_state state;
public:
	FSM(fsm_state def_state): state(def_state){};
	fsm_state operator*() const { return state;}
	FSM&  operator++() {state = (state==ST) ? ST: fsm_state(state + 1); return *this;}
	FSM& operator--(){state = (state==SNT) ? SNT: fsm_state(state - 1); return *this;}
	~FSM() = default;
};
/*********************************************************************************************/
/*********************************************************************************************/

class Table
{
private:
	std::vector<FSM> fsm_array;
public:
	Table(int fsmSize,fsm_state fsmState): fsm_array(fsmSize,fsmState){}

	/**
	 * @brief operator for getting the state of FSM in index "index"
	 * 
	 * @param index - the index of the FSM in the Table
	 * @return fsm_state the current state
	 */
	fsm_state operator[](int index) const {return *fsm_array[index];}

	Table& operator= (const Table& other)
	{
		fsm_array = other.fsm_array;

		return *this;
	}
	/**
	 * @brief updating an FSM in the given index
	 * 
	 * @param index  the index of the FSM
	 * @param taken  the update. TRUE if Taken, FALSE if NotTaken
	 */
	void update(int index, bool taken);

	~Table() = default;
};

void Table::update(int index, bool taken)
{
	if (taken)
		++fsm_array[index];
	else
		--fsm_array[index];
}
/*********************************************************************************************/
/*********************************************************************************************/

class Tables
{
private:
	unsigned num_of_tables;
	unsigned btbSize;
	fsm_state fsmState;
	bool isGlobalTable;
	using_share_enum shared;
	std::vector<Table> tables;
	
public:
	Tables(unsigned historySize, unsigned btbSize, fsm_state fsmState, bool isGlobalTable, int Shared);

	/**
	 * @brief Get the Prediction of the commend. True if taken, otherwise False
	 * 
	 * @param btb_index - the index of the table
	 * @param fsm_index - the index of the state machine of the specific prediction
	 * @return true if the state is ST or WT
	 * @return false if the statr is SNT or WNT
	 */
	bool getPrediction(uint32_t btb_index, uint32_t fsm_index);

	/**
	 * @brief update the given fsm with the wtakenw value
	 * 
	 * @param btb_index - the index of the table
	 * @param fsm_index - the index of the state machine of the specific prediction
	 * @param taken if real branch resolve
	 */
	void updateFSM(uint32_t btb_index, uint32_t fsm_index, bool taken);

	/**
	 * @brief reset a whole table for the new entry in the BTB
	 * 
	 * @param btb_index - the index of the table
	 */
	void clearTable(uint32_t btb_index);

	/**
	 * @brief Get the Default Pred of the tables
	 * 
	 * @return true if taken
	 * @return false if nottaken
	 */
	bool getDefaultPred(){return (fsmState == ST || fsmState == WT);}

	~Tables() = default;
};

Tables::Tables(unsigned historySize,unsigned btbSize, fsm_state fsmState, bool isGlobalTable, int Shared) :
	num_of_tables(1<<historySize), btbSize(btbSize), fsmState(fsm_state(fsmState)), isGlobalTable(isGlobalTable), shared(using_share_enum(Shared))
{
	if(isGlobalTable)
	{
		tables = std::vector<Table>(NUM_OF_GLOBAL_TABLE,Table(num_of_tables,fsmState));
	}
	else
	{
		tables = std::vector<Table>(btbSize,Table(num_of_tables,fsmState));
	}	
}

bool Tables::getPrediction(uint32_t btb_index, uint32_t fsm_index)
{
	fsm_state pred;
	if(isGlobalTable)
		pred = tables[GLOBAL_TABLE][fsm_index];
	else
		pred = tables[btb_index][fsm_index];
	return (pred == ST || pred == WT);
}

void Tables::updateFSM(uint32_t btb_index, uint32_t fsm_index, bool taken)
{
	if(isGlobalTable)
		tables[GLOBAL_TABLE].update(fsm_index, taken);
	else
		tables[btb_index].update(fsm_index, taken);
}

void Tables::clearTable(uint32_t btb_index)
{
	if(!isGlobalTable)
		tables[btb_index] = Table(num_of_tables,fsmState);
}
/*********************************************************************************************/
/*********************************************************************************************/

class BP
{
public:
	BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	~BP() = default;

	/**
	 * @brief turns the predictor's prediction (taken / not taken) and predicted target address
	 * 
	 * @param pc the branch instruction address
	 * @param dst the target address (when prediction is not taken, dst = pc + 4)
	 * @return true when prediction is taken
	 * @return false otherwise (prediction is not taken)
	 * 
	 */
	bool predict(uint32_t pc, uint32_t *dst);

	/**
	 * @brief update history, FSM based on actual op. 
	 * insert new op. to BTB even if NOT_TAKEN
	 * 
	 * @param pc 
	 * @param targetPc actual op. target pc
	 * @param taken actual branch desicion
	 * @param pred_dst predicted target pc from BTB
	 */
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);

	/**
	 * @brief Get the SIM Stats object
	 * 
	 * @param curStats The returned current simulator state 
	 */
	void getStats(SIM_stats *curStats) {*curStats=stats; }
private:
	BTB btb;
	Tables tables;
	SIM_stats stats;
};

BP::BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared): 
				btb(btbSize,historySize,tagSize,isGlobalHist,Shared),
				tables(historySize,btbSize,fsm_state(fsmState),isGlobalTable,Shared)
{ 
	stats.br_num = 0; stats.flush_num = 0;

	stats.size = btbSize*(tagSize + TARGET_SIZE + VALID_BIT_SIZE);
	stats.size += isGlobalHist ? historySize : btbSize*historySize;
	stats.size += isGlobalTable ? 1<<(historySize+1): btbSize*(1<<(historySize+1));
}

bool BP::predict(uint32_t pc, uint32_t *dst)
{
	*dst = pc +4;
	if(!btb.isKnownBranch(pc))
		return false;
	uint32_t btb_index = btb.getBTBIndex(pc);
	uint32_t fsm_index = btb.getTableIndex(pc);
	bool prediction = tables.getPrediction(btb_index,fsm_index);
	if(prediction)
		*dst = btb.predictTarget(pc);
	return prediction;
}

void BP::update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	uint32_t btb_i = btb.getBTBIndex(pc);
	uint32_t fsm_i = btb.getTableIndex(pc);
	
	if (!btb.isKnownBranch(pc))
	{
		tables.clearTable(btb_i);
	}
	
	btb.update(pc,taken,targetPc);
	tables.updateFSM(btb_i,fsm_i,taken);

	stats.br_num++;
	if ((taken && targetPc != pred_dst) || (!taken && pred_dst != pc+4))
	{
		stats.flush_num++;
	}
}

/*********************************************************************************************/
/*********************************************************************************************/

BP* bp;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){

	int retval = 0;
	try
	{
		bp = new BP(btbSize, historySize,tagSize,fsmState,isGlobalHist,isGlobalTable,Shared);
	}	catch(const std::exception& e)
	{
		retval = -1;
	}
	
	return retval;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return bp->predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	bp->update(pc,targetPc,taken,pred_dst);
}

void BP_GetStats(SIM_stats *curStats){
	bp->getStats(curStats);
	delete bp;
}