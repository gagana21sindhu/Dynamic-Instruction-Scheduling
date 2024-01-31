#ifndef PROC_H
#define PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include <iomanip>
#include <vector>
#include <algorithm>

using namespace std;

/* instruction structure that travels through the pipeline */
typedef struct inst{
    unsigned int seq_no, op_type, exe_cycle = 0;
    int dest, src1, src2 = 0;
    int src1_init,src2_init = 0;
    bool src1_rdy, src2_rdy, src1_ROB, src2_ROB = false;
    unsigned int FE_begin, FE_cycles, DE_begin, DE_cycles = 0;
    unsigned int RN_begin, RN_cycles, RR_begin, RR_cycles = 0;
    unsigned int DI_begin, DI_cycles, IS_begin, IS_cycles = 0;
    unsigned int EX_begin, EX_cycles, WB_begin, WB_cycles = 0;
    unsigned int RT_begin, RT_cycles = 0;

    bool operator < (const inst &comp) const // For when using sort on the issue queue
	{
		return (seq_no < comp.seq_no);
	}
}instr;

/* Issue Queue structure with the instruction vector */
typedef struct{
    bool valid = false;
    vector <instr> iq;
}IQVect;

/* Pipeline Register structure with a instruction vector to store the instructions */
typedef struct{
    bool empty = true;
    vector <instr> preg;
}pipelineReg;

/* RMT Row structure to build the RMT based on RMT size */
typedef struct {
    bool valid = false;
    int ROBTag = -1;
}RMTRow;

/* ROB Row structure to build the ROB */
typedef struct {
    unsigned int seq_no = 0;
    int dest = -1;
    bool rdy = false;
    void clear()
    {
        dest = -1;
        seq_no = 0;
        rdy = false;
    }
    bool isEmptyEntry()
    {
        return (dest == -1 && seq_no == 0 && rdy == false);
    }
}ROBRow;

/* ROB structure with ROB row vector */
typedef struct{
    vector <ROBRow> rob;
}ROBVect;

/* Execute list structure to store the instructions being executed */
typedef struct{
    vector <instr> exe_list;
    void exe_cycle_dec()
    {
        for (unsigned int i = 0; i < exe_list.size(); i++)
				exe_list[i].exe_cycle--;
    }
}execute_list;

class proc {
    public :
    unsigned int cycle, seq_no, width = 0;
    bool pipelineEmpty = true;
    unsigned int IQSize, ROBSize, RMTSize = 0;
    unsigned int head , tail = 0;

    pipelineReg DE, RN, RR, DI, WB, RT;
    execute_list exec_list;

    RMTRow *RMT;
    ROBVect ROB;
    IQVect IQ;

    proc(int a, int b, int c); // Constructor

    void initRMT();
    void initROB();
    bool Advance_Cycle(FILE* f);
    void Fetch(FILE* FP);
    void Decode();
    void Rename();
    void RegRead();
    void Dispatch();
    void Issue();
    void Execute();
    void Writeback();
    void Retire(); 
    void printSummary(unsigned int i);

};

#endif
