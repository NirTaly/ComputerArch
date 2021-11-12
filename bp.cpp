/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include <bitset>
#include <vector>
#include "bp_api.h"

using std::bitset;


typedef enum {SNT = 0,WNT,WT,ST} fsm_state;

class FSM
{
private:
	fsm_state state;
public:
	FSM(fsm_state def_state): state(def_state){};
	fsm_state operator*() const { return state;}
	FSM&  operator++() {state = (state==ST) ? ST: fsm_state(state + 1);}
	FSM& operator--(){state = (state==SNT) ? SNT: fsm_state(state - 1);}
	~FSM() = default;
};

class BP
{
public:
	BP();
	~BP();
private:
	History history;
	Tables tables;
};

class History
{
public:
	History(/* args */);
	~History();
private:
};

/**
 * @brief 
 * 
 */
class Table
{
private:
	std::vector<FSM> fsm_array;
public:
	Table(int fsmSize,int fsmState): fsm_array(fsmSize,fsm_state(fsmState)){}
	fsm_state operator[](int index) const {return *fsm_array[index];}
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

class GlobalTables
{
public:
	GlobalTables(/* args */);
	~GlobalTables();
private:
};

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

