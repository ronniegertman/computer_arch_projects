/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdlib.h>

struct BTB{
	struct BTB_ITEM* btb_array;
	unsigned int btbSize;
	unsigned int historySize;
	unsigned int tagSize;
	unsigned int fsmDefaultState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;
	struct history* history_array;
	struct fsm* fsm_table;
};

struct BTB_ITEM{
	unsigned int tag;
	unsigned int target;
	struct history* history_p;
	struct fsm* fsm_p;
};

struct history{
	char history;
};

struct fsm{
	char state;
};


struct BTB* btb_table;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	btb_table = (struct BTB)* malloc(sizeof(struct BTB));
	if(!btb_table){
		return -1;
	}
	btb_table->btb_array = (struct BTB_ITEM)* malloc(sizeof(struct BTB_ITEM) * btbSize);
	if(!btb_table->btb_array){
		free(btb_table);
		return -1;
	}
	btb_table->btbSize = btbSize;
	btb_table->historySize = historySize;
	btb_table->tagSize = tagSize;
	btb_table->fsmDefaultState = fsmState;
	btb_table->isGlobalHist = isGlobalHist;
	btb_table->isGlobalTable = isGlobalTable;
	btb_table->Shared = Shared;

	btb_table->history_array = (struct history)* malloc(sizeof(struct history) * (!isGlobalHist + (isGlobalHist)* btbSize));
	if(btb_table->history_array == NULL){
		free(btb_table->btb_array);
		free(btb_table);
	}

	btb_table->fsm_table = (struct fsm)* malloc(sizeof(struct fsm) * 2^historySize * 
	(!isGlobalTable + (!isGlobalTable) * btbSize));
	if(btb_table->fsm_table == NULL){
		free(btb_table->btb_array);
		free(btb_table->history_array);
		free(btb_table);
	}


	return 0;
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

