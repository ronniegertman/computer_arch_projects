/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdlib.h>
#include <math.h>

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

typedef enum{
	STATE_00 = 0; 
	STATE_01 = 1;
	STATE_10 = 2;
	STATE_11 = 3;
} State;

//we need to set this size cousemek
struct history{
	char history;
};

struct fsm{
	State state;
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

	btb_table->history_array = (struct history)* malloc(sizeof(struct history) * (isGlobalHist + (!isGlobalHist)* btbSize));
	if(btb_table->history_array == NULL){
		free(btb_table->btb_array);
		free(btb_table);
	}

	//we need a defualt history state
	for (int i=0; i<(isGlobalHist + (!isGlobalHist)* btbSize); i++){
		btb_table->history_array[i].history = 0;
	}

	btb_table->fsm_table = (struct fsm)* malloc(sizeof(struct fsm) * 2^historySize *
	(isGlobalTable + (!isGlobalTable) * btbSize));
	if(btb_table->fsm_table == NULL){
		free(btb_table->btb_array);
		free(btb_table->history_array);
		free(btb_table);
	}

	//we need a default fsm prediction
	for (int i=0; i<2^historySize * (isGlobalTable + (!isGlobalTable) * btbSize)>; i++){
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
		is_branch_taken = btb_table->fsm_table[history_state].state >> 1; //i think this might be wrong
	} else {
		is_branch_taken = btb_table->fsm_table[input_btb_line * 2^btb_table->historySize + history_state].state >> 1;
	}
	
	return is_branch_taken;
}

//TODO: init history and fsm input: input_btb_line, global states
void init_fsm(unsigned int line){
	for (int i=0; i<pow(2, btb_table->historySize); i++){
		if (btb->isGlobalTable) {
			btb_table->fsm_table[i].state = btb_table->fsmDefaultState;
		} 
		else {
			btb_table->fsm_table[line * pow(2, btb_table->historySize) + i].state = btb_table->fsmDefaultState;				
		}
	}
	
}

void init_history(unsigned int line){
	if(btb_table->isGlobalHistory){
		btb_table->history_array[0] = 0;
	}
	btb_table->history_array[line] = 0;
}

void init(unsigned int line){
	init_history();
	init_fsm();
}
//TODO: function that takes into account of shared or not

//TODO: function that updates both fsm and history
void update(unsigned int line, bool taken){
	char history_index = btb->isGlobalHistory ? 0 : line;
	char history_state = btb_table->history_array[history_index];
	long fsm_index;

	//first we need to update the fsm table of the current history
	if (btb_table->isGlobalTable) {
		fsm_index = history_state;
	} else {
		fsm_index = input_btb_line * pow(2, btb_table->historySize) + history_state;
	}
	next_state(btb_table->fsm_table[fsm_index].state, taken, fsm_index);

	//then, we update the history
	btb_table->history_array[history_index] = (btb_table->history_array[0] << 1) | taken;
}

//Updates the indexed fsm to the next state according to prediction and current state
State next_state(State current, bool input, long index) {
    switch (current) {
		//Strongly not taken
        case STATE_00:
			btb_table->fsm_table[index].state = input ? STATE_01 : STATE_00;
            return input ? STATE_01 : STATE_00;
		//Weakly not taken
        case STATE_01:
			btb_table->fsm_table[index].state = input ? STATE_10 : STATE_00;
            return input ? STATE_10 : STATE_00;
		//Weakly taken
        case STATE_10:
			btb_table->fsm_table[index].state = input ? STATE_11 : STATE_01;
            return input ? STATE_11 : STATE_01;
		//Strongly taken
        case STATE_11:
			btb_table->fsm_table[index].state = input ? STATE_11 : STATE_10;
            return input ? STATE_11 : STATE_10;
        default:
            return STATE_00; // default to safe state
    }
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	//check if pc is already in table
	unsigned int input_btb_line = (pc >> 2) % btb_table->btbSize;
	unsigned int input_tag = pc % (2 ^ (btb_table->tagSize));

	if (btb_table->btb_array[input_btb_line].initialized == false) {
		btb_table->btb_array[input_btb_line].initialized = true;
		btb_table->btb_array[input_btb_line].tag = input_tag;
		btb_table->btb_array[input_btb_line].target = targetPc;
		update(input_btb_line, taken);
	}

	else if (btb_table->btb_array[input_btb_line].tag != input_tag) {
		btb_table->btb_array[input_btb_line].tag = input_tag;
		btb_table->btb_array[input_btb_line].target = targetPc;
		init(input_btb_line);
		update(input_btb_line, taken);
	}

	else if(btb_table->btb_array[input_btb_line].tag == input_tag) {
		//is this considered false prediciton or not
		if (targetPc != pred_dst){
			btb_table->btb_array[input_btb_line].target = targetPc;
			init(input_btb_line);
			update(input_btb_line, taken);
		} else{
			//update() without initializing
			update(line, taken);
		}

	}
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
