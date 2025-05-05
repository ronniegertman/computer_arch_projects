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
	bool initialized;
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
	for (int i=0; i<btbSize; i++){
		btb_table->btb_array[i].initialized = false;
	}
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

	//we need a defualt history state
	for (int i=0; i<(!isGlobalHist + (isGlobalHist)* btbSize); i++){
		btb_table->history_array[i].history = 0;
	}

	btb_table->fsm_table = (struct fsm)* malloc(sizeof(struct fsm) * 2^historySize *
	(!isGlobalTable + (!isGlobalTable) * btbSize));
	if(btb_table->fsm_table == NULL){
		free(btb_table->btb_array);
		free(btb_table->history_array);
		free(btb_table);
	}

	//we need a default fsm prediction
	for (int i=0; i<2^historySize * (!isGlobalTable + (!isGlobalTable) * btbSize)>; i++){
		btb_table->fsm_table[i].state = fsmState;
	}

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	*dst = pc + 4; //will be changed if pc tag exists in btb_table

	unsigned int input_btb_line = (pc >> 2) % btb_table->btbSize;
	//checking if the btb_table line has a branch command, if we end up not using ist_p it needs to be changed
	if (btb_table->btb_array[input_btb_line].initialized == false) {
		return false;
	}
	unsigned int input_tag = pc % (2 ^ (btb_table->tagSize));
	if (btb_table->btb_array[input_btb_line].tag != input_tag) {
		//might need to call BP_update, i think not
		return false;
	}
	*dst = btb_table->btb_array[input_btb_line].target;
	// bool is_branch_taken = (btb_table->btb_array[input_btb_line].fsm_p->state >>1);
	bool is_branch_taken;
	char history_state = btb->isGlobalHistory ? 
		btb_table->history_array[0] : btb_table->history_array[input_btb_line];

	if (btb->isGlobalTable) {
		is_branch_taken = btb_table->fsm_table[history_state].state >> 1;
	} else {
		is_branch_taken = btb_table->fsm_table[input_btb_line * 2^btb_table->historySize + history_state].state >> 1;
	}
	
	return is_branch_taken;
}

//TODO: init history and fsm input: input_btb_line, global states
//TODO: function that takes into account of shared or not
//TODO: function that updates both fsm and history

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	//check if pc is already in table
	unsigned int input_btb_line = (pc >> 2) % btb_table->btbSize;
	unsigned int input_tag = pc % (2 ^ (btb_table->tagSize));

	if (btb_table->btb_array[input_btb_line].initialized == false) {
		btb_table->btb_array[input_btb_line].initialized = true;
		btb_table->btb_array[input_btb_line].tag = input_tag;
		btb_table->btb_array[input_btb_line].target = targetPc;
		update();
	}

	else if (btb_table->btb_array[input_btb_line].tag != input_tag) {
		btb_table->btb_array[input_btb_line].tag = input_tag;
		btb_table->btb_array[input_btb_line].target = targetPc;
		init(input_btb_line);
		update();
	}

	else if(btb_table->btb_array[input_btb_line].tag == input_tag) {
		if (targetPc != pred_dst){
			btb_table->btb_array[input_btb_line].target = targetPc;
			init(input_btb_line);
			update();
		}
		//update() withpur initializing


	}
	

	// now we know that the tag of pc is already in the table




	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

struct BTB_ITEM{
	unsigned int tag;
	unsigned int target;
	bool initialized;
};
