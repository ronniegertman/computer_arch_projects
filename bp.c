/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdlib.h>
#include <math.h>


typedef enum{
	STATE_00 = 0,
	STATE_01 = 1,
	STATE_10 = 2,
	STATE_11 = 3
} State;

void init_fsm(unsigned int line);
void init_history(unsigned int line);
void init(unsigned int line);
void update(unsigned int line, bool taken, uint32_t pc);
State next_state(State current, bool input, unsigned int index);
unsigned int calc_fsm_index(unsigned int line, uint32_t pc);

struct BTB{
	struct BTB_ITEM* btb_array;
	unsigned int btbSize;
	unsigned int historySize;
	unsigned int tagSize;
	unsigned int fsmDefaultState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared; //0 not shared, 1 lsb, 2 mid 
	unsigned char* history_array;
	struct fsm* fsm_table;
};

struct BTB_ITEM{
	unsigned int tag;
	unsigned int target;
	bool initialized;
};


struct fsm{
	State state;
};


struct BTB* btb_table;
static int pred_count = 0;
static int flush_count = 0;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	btb_table = (struct BTB*) malloc(sizeof(struct BTB));
	if(!btb_table){
		return -1;
	}
	btb_table->btb_array = (struct BTB_ITEM*) malloc(sizeof(struct BTB_ITEM) * btbSize);
	for (int i=0; i<btbSize; i++){
		btb_table->btb_array[i].initialized = false;
		btb_table->btb_array[i].tag = 0;
		btb_table->btb_array[i].target = 0;
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

	btb_table->history_array = (char*) malloc(isGlobalHist + (!isGlobalHist)* btbSize);
	if(btb_table->history_array == NULL){
		free(btb_table->btb_array);
		free(btb_table);
	}

	//we need a defualt history state
	for (int i=0; i<(isGlobalHist + (!isGlobalHist)* btbSize); i++){
		btb_table->history_array[i] = 0;
	}

	btb_table->fsm_table = (struct fsm*) malloc(sizeof(struct fsm) * pow(2, historySize) *
	(isGlobalTable + (!isGlobalTable) * btbSize));
	if(btb_table->fsm_table == NULL){
		free(btb_table->btb_array);
		free(btb_table->history_array);
		free(btb_table);
	}

	//we need a default fsm prediction
	for (int i=0; i<pow(2, historySize) * (isGlobalTable + (!isGlobalTable) * btbSize); i++){
		btb_table->fsm_table[i].state = fsmState;
	}

	return 0;
}

unsigned int calc_fsm_index(unsigned int line, uint32_t pc){
	unsigned char history_state = (btb_table->isGlobalHist) ?
			btb_table->history_array[0] : btb_table->history_array[line];
	//history might be smaller than 8 bits
	unsigned char mask = (1 << btb_table->historySize) - 1;
	history_state = history_state & mask;
	unsigned int fsm_index;
	unsigned char toxor;
	unsigned char new_index;
	switch(btb_table->Shared){
		case 0:
		//not using shared
			fsm_index = btb_table->isGlobalTable ? history_state :
				line * (1 << btb_table->historySize) + history_state;
			break;
		case 1:
		//shared lsb
			toxor = (pc >> 2) & mask;
			new_index = history_state ^ toxor;
			fsm_index =  btb_table->isGlobalTable ? new_index :
							line * (1 << btb_table->historySize) + history_state;
			break;
		case 2:
		//shared mid
			toxor = (pc >> 16) & mask;
			new_index = history_state ^ toxor;
			fsm_index = btb_table->isGlobalTable ? new_index :
					line * (1<<btb_table->historySize) + history_state;
			break;
		default:
			fsm_index = history_state;
	}
	return fsm_index;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	pred_count++; //for the stats func
	*dst = pc + 4; // will be changed if pc tag exists in btb_table

	unsigned int input_btb_line = (pc >> 2) % btb_table->btbSize;
	//checking if the btb_table line has a branch command, if we end up not using ist_p it needs to be changed
	if (btb_table->btb_array[input_btb_line].initialized == false) {
		return false;
	}
	//this takes the pc and shifts it to only have the tag bits
	//for exaple : ZZZZZZZZZZZZXXXXXYYYY00 this takes the X bytes.
	unsigned int input_tag = (pc >> (2 + btb_table->btbSize)) % (1 << btb_table->tagSize);
	if (btb_table->btb_array[input_btb_line].tag != input_tag) {
		return false;
	}

	bool is_branch_taken;
	unsigned int fsm_index = calc_fsm_index(input_btb_line, pc);
//	printf("\nfsm index is %u\n", fsm_index);
//	printf("pc is 0x%X line is %u \n", pc, input_btb_line);

	is_branch_taken = btb_table->fsm_table[fsm_index].state >> 1;

	if (is_branch_taken) {
		*dst = btb_table->btb_array[input_btb_line].target;
	}
	return is_branch_taken;
}

//TODO: init history and fsm input: input_btb_line, global states
void init_fsm(unsigned int line){
	for (int i=0; i<pow(2, btb_table->historySize); i++){
		if (!btb_table->isGlobalTable) {
			btb_table->fsm_table[line * (1 << btb_table->historySize) + i].state = btb_table->fsmDefaultState;
		}
	}
	
}

void init_history(unsigned int line){
	if(!btb_table->isGlobalHist){
		btb_table->history_array[line] = 0;
	}
}

void init(unsigned int line){
	init_history(line);
	init_fsm(line);
}


//TODO: function that updates both fsm and history
void update(unsigned int line, bool taken, uint32_t pc){
	unsigned char history_index = btb_table->isGlobalHist ? 0 : line;
	// char history_state = btb_table->history_array[history_index] % (1 << btb_table->historySize);
	int mask = (1 << btb_table->historySize) - 1;
	//first we need to update the fsm table of the current history
	unsigned int fsm_index = calc_fsm_index(line, pc);
	next_state(btb_table->fsm_table[fsm_index].state, taken, fsm_index);
	//the history array is accessed with line, history state is LSB in history size.
//	printf("history before change %u\n", btb_table->history_array[history_index]);
	btb_table->history_array[history_index] = ((btb_table->history_array[history_index] << 1) | taken) & mask;
//	printf("history state is %u\n", btb_table->history_array[history_index]);

}

//Updates the indexed fsm to the next state according to prediction and current state
State next_state(State current, bool input, unsigned int index) {
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
	// unsigned int input_tag = ((pc >> (30 - btb_table->tagSize)));
	unsigned int input_tag = (pc >> (2 + btb_table->btbSize)) % (1 << btb_table->tagSize);
	//check if pred was correct
	unsigned int fsm_index = calc_fsm_index(input_btb_line, pc);
	bool pred_taken = btb_table->fsm_table[fsm_index].state >> 1;
	if ( !btb_table->btb_array[input_btb_line].initialized || btb_table->btb_array[input_btb_line].tag != input_tag){
		pred_taken = false;
	}
	if ((taken != pred_taken) || ((targetPc != pred_dst) && (pred_taken == 1))) {
		flush_count++;
	}

	if (btb_table->btb_array[input_btb_line].initialized == false) {
		btb_table->btb_array[input_btb_line].initialized = true;
		btb_table->btb_array[input_btb_line].tag = input_tag;
		btb_table->btb_array[input_btb_line].target = targetPc;
		update(input_btb_line, taken, pc);
	}

	else if (btb_table->btb_array[input_btb_line].tag != input_tag) {
		btb_table->btb_array[input_btb_line].tag = input_tag;
		btb_table->btb_array[input_btb_line].target = targetPc;
		init(input_btb_line);
		update(input_btb_line, taken, pc);
	}

	else if(btb_table->btb_array[input_btb_line].tag == input_tag) {
		//is this considered false prediciton or not
		update(input_btb_line, taken, pc);
		if (targetPc != pred_dst){
			btb_table->btb_array[input_btb_line].target = targetPc;
		}
	}
	return;
}

void BP_GetStats(SIM_stats *curStats){
	curStats->br_num  = pred_count;
	curStats->flush_num  = flush_count;
	int btb_size = btb_table->btbSize;
	int tag_size = btb_table->tagSize;
	bool isGlobalHist = btb_table->isGlobalHist;
	bool isGlobalTable = btb_table->isGlobalTable;
	int historySize = btb_table->historySize;
	curStats->size = btb_size*(1 + tag_size + 32) +
		(isGlobalHist ? 1 : btb_size) * (historySize) +
			(isGlobalTable ? 1 : btb_size) * 2 * pow(2, historySize) ;
	free(btb_table->fsm_table);
	free(btb_table->history_array);
	free(btb_table->btb_array);
	free(btb_table);
	return;
}


void print_table(){
		printf("BTB Table:\n");
		printf("Line\tInitialized\tTag\t\tTarget\n");
		for (unsigned int i = 0; i < btb_table->btbSize; i++) {
			printf("%u\t%s\t\t0x%x\t\t0x%x\n", 
				   i, 
				   btb_table->btb_array[i].initialized ? "Yes" : "No", 
				   btb_table->btb_array[i].tag, 
				   btb_table->btb_array[i].target);
			if (!btb_table->isGlobalHist) {
				printf("\t\tHistory: %d\n", btb_table->history_array[i]);
			}
		}
		if (btb_table->isGlobalHist) {
			printf("Global History: %d\n", btb_table->history_array[0]);
		}
}

//this works only for regular share and not xor
void print_fsm(unsigned int btb_line) {
	printf("FSM Table for BTB Line %u:\n", btb_line);
	unsigned int fsm_table_size = pow(2, btb_table->historySize);
	unsigned int start_index = btb_table->isGlobalTable ? 0 : btb_line * fsm_table_size;

	for (unsigned int i = 0; i < fsm_table_size; i++) {
		unsigned int index = start_index + i;
		printf("Index: %u, State: %d\n", index, btb_table->fsm_table[index].state);
	}

	if (btb_table->isGlobalHist) {
		printf("Corresponding Global History: %d\n", btb_table->history_array[0]);
	} else {
		printf("Corresponding History for BTB Line %u: %d\n", btb_line, btb_table->history_array[btb_line]);
	}
}

