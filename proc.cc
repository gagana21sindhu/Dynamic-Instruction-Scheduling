#include "proc.h"

using namespace std;

/*Processor Constructor*/
proc::proc (int a, int b, int c){
    ROBSize = a;
    IQSize = b;
    width = c;
    RMTSize = 67;
    seq_no = 0;
    cycle = 0;
    head = tail = 3;

    
    RMT = new RMTRow[RMTSize];

    pipelineEmpty = false;

    initRMT();
    initROB();
    
} // End : Constructor

void proc::initRMT()
{
    for (unsigned int i = 0; i < 67; i++)
	{
		RMT[i].valid = false;
		RMT[i].ROBTag = -1;
	}
} // END : initRMT

void proc::initROB()
{
    ROBRow r;
    r.clear();
    for (unsigned int i = 0; i < ROBSize; i++)
		ROB.rob.push_back(r);
} // END : initROB

void proc::printSummary(unsigned int i)
{
    cout << RT.preg[i].seq_no << " fu{" << RT.preg[i].op_type << "} src{" << RT.preg[i].src1_init << "," << RT.preg[i].src2_init << "} dst{" << ROB.rob[head].dest << "} ";
    cout << "FE{" << RT.preg[i].FE_begin << "," << RT.preg[i].FE_cycles << "} DE{" << RT.preg[i].DE_begin << "," << RT.preg[i].DE_cycles << "} ";
    cout << "RN{" << RT.preg[i].RN_begin << "," << RT.preg[i].RN_cycles << "} RR{" << RT.preg[i].RR_begin << "," << RT.preg[i].RR_cycles << "} ";
    cout << "DI{" << RT.preg[i].DI_begin << "," << RT.preg[i].DI_cycles << "} IS{" << RT.preg[i].IS_begin << "," << RT.preg[i].IS_cycles << "} ";
    cout << "EX{" << RT.preg[i].EX_begin << "," << RT.preg[i].EX_cycles << "} WB{" << RT.preg[i].WB_begin << "," << RT.preg[i].WB_cycles << "} ";
    cout << "RT{" << RT.preg[i].RT_begin << "," << RT.preg[i].RT_cycles << "} " << endl;

}
bool proc::Advance_Cycle(FILE* f)
{
    if (pipelineEmpty == true && feof(f))
		return false;
	else
		return true;
} // END : Advance_Cycle

void proc::Fetch(FILE* FP)
{
    instr inst;
    uint64_t pc;
    unsigned int bundle_count = 0;
    if(DE.empty == false) // DE Not Empty
        return;
       
    if(DE.empty)
    {
        while(fscanf(FP, "%lx %d %d %d %d", &pc, &inst.op_type, &inst.dest, &inst.src1, &inst.src2) != EOF)
        {
            inst.seq_no = seq_no;
            inst.src1_init = inst.src1;
            inst.src2_init = inst.src2;
            inst.src1_rdy = inst.src2_rdy = inst.src1_ROB = inst.src2_ROB = false;
            if (inst.op_type == 0)
				inst.exe_cycle = 1;
			else if (inst.op_type == 1)
				inst.exe_cycle  = 2;
			else if (inst.op_type == 2)
				inst.exe_cycle  = 5;

            inst.FE_begin = cycle;
            inst.FE_cycles = 1;
            inst.DE_begin = cycle + 1;

            DE.preg.push_back(inst);
            seq_no++;
            bundle_count++;
            if(DE.preg.size() == width) // Read until Width number of instructions
                break;

        }

        if (bundle_count != 0)
			DE.empty = false;
    }
} // END : Fetch

void proc::Decode()
{
    if (!DE.empty) // DE Contains a Decode Bundle
	{
		if (RN.empty) // RN is empty to accept a new RN bundle
		{
			for (unsigned int i = 0; i < DE.preg.size(); i++)
			{
				DE.preg[i].RN_begin = cycle + 1;
				DE.preg[i].DE_cycles = DE.preg[i].RN_begin - DE.preg[i].DE_begin;
			}
			RN.preg = move(DE.preg);
			DE.preg.clear();
			DE.empty = true;
			RN.empty = false;
		}
			
	}
} // END : Decode

void proc::Rename()
{
    if(!RN.empty) // RN Contains a Rename Bundle
    {
        if(RR.empty) // RR is empty to accept a new register-read bundle
        {
            unsigned int ROBavail;
            if (tail < head) {
                ROBavail = head - tail;
            } else if (head < tail) {
                ROBavail = ROBSize - (tail - head);
            } else {
                if (tail < ROBSize - 1) {
                    ROBavail = ROB.rob[tail+1].isEmptyEntry() ? ROBSize : 0;
                } else {
                    ROBavail = ROB.rob[tail-1].isEmptyEntry() ? ROBSize : 0;
                }
            }

            if (ROBavail < RN.preg.size()) // ROB does not have enough free entries to accept the entire RN bundle
                return;
            else  // Process the RN Bundle
            {
                for (unsigned int i = 0; i<RN.preg.size(); i++)
                {
                    // (1) Allocate an entry in the ROB for the instruction
                    ROB.rob[tail].dest = RN.preg[i].dest;
                    ROB.rob[tail].seq_no = RN.preg[i].seq_no;
                    ROB.rob[tail].rdy = false ;

                    // (2) Rename Source Registers 
                    if(RN.preg[i].src1 != -1)
                    {
                        if(RMT[RN.preg[i].src1].valid == true)
                        {
                                RN.preg[i].src1 = RMT[RN.preg[i].src1].ROBTag;
                                RN.preg[i].src1_ROB = true;
                        }

                    }
                    if(RN.preg[i].src2 != -1)
                    {
                        if(RMT[RN.preg[i].src2].valid == true)
                        {
                                RN.preg[i].src2 = RMT[RN.preg[i].src2].ROBTag;
                                RN.preg[i].src2_ROB = true;
                        }

                    }

                    // (3) Rename Destination Register and updating RMT
                    if(RN.preg[i].dest != -1)
                    {
                        RMT[RN.preg[i].dest].ROBTag = tail;
                        RMT[RN.preg[i].dest].valid = true;
                    }
                    RN.preg[i].dest = tail;

                    if(tail != (ROBSize -1)) // Increment the tail
                        tail++;
                    else
                        tail = 0;

                    RN.preg[i].RR_begin = cycle+1;
                    RN.preg[i].RN_cycles = RN.preg[i].RR_begin - RN.preg[i].RN_begin;

                }

                RR.preg = move(RN.preg);
                RN.preg.clear();
                RN.empty = true;
			    RR.empty = false;

            }
        }
    }
} // END : Rename

void proc::RegRead()
{
    if(!RR.empty) // RB Contains a Register Read Bundle
    {
        if(DI.empty) // DI is empty to accept a new dispatch bundle
        {
            for(unsigned int i = 0; i < RR.preg.size(); i++)
            {
                // Ascertain the readiness of the renamed source operands
                if (RR.preg[i].src1_ROB) // if src1 renamed to ROB entry 
				{
					if (ROB.rob[RR.preg[i].src1].rdy == true) // check the ROB entry for ready 
						RR.preg[i].src1_rdy = true;
				}
				else
					RR.preg[i].src1_rdy = true;  // No ROB entry value form ARF 

				if (RR.preg[i].src2_ROB) // if src1 renamed to ROB entry
				{
					if (ROB.rob[RR.preg[i].src2].rdy == true) // check the ROB entry for ready 
						RR.preg[i].src2_rdy = true;
				}
				else
					RR.preg[i].src2_rdy = true; // No ROB entry value form ARF 

				RR.preg[i].DI_begin = cycle + 1;
				RR.preg[i].RR_cycles = RR.preg[i].DI_begin - RR.preg[i].RR_begin;
            }
            DI.preg = move(RR.preg);
            RR.preg.clear();
            RR.empty = true;
            DI.empty = false;
        }
    }
} //END : RegRead

void proc::Dispatch()
{
    if(!DI.empty) // // DI Contains a Dispatch Bundle
    {
        // Check if no.of free entries in IQ is greater than or equal to the size of the dispatch bundle 
        if((IQSize - IQ.iq.size()) >= DI.preg.size())
        {
            for(unsigned int i = 0; i < DI.preg.size(); i++)
            {
                DI.preg[i].IS_begin = cycle + 1;
				DI.preg[i].DI_cycles = DI.preg[i].IS_begin - DI.preg[i].DI_begin;
				IQ.iq.push_back(DI.preg[i]); // Dispatching to the issue queue
            }
            DI.preg.clear();
            DI.empty = true;
        }
    }
} // END : Dispatch

void proc::Issue()
{
    
    if(IQ.iq.size() !=0) //  If IQ not empty
    {
        sort(IQ.iq.begin(),IQ.iq.end());
        unsigned int count = 0;
        for (unsigned int i = 0; i < IQ.iq.size(); i++)
        {
            if (IQ.iq[i].src1_rdy && IQ.iq[i].src2_rdy)
            {
                IQ.iq[i].EX_begin = cycle + 1;
                IQ.iq[i].IS_cycles = IQ.iq[i].EX_begin - IQ.iq[i].IS_begin;
                exec_list.exe_list.push_back(IQ.iq[i]);
                IQ.iq.erase(IQ.iq.begin() + i);
                count++;
                i--;
            }
            if(count == width)
                break;
        }
    }
}

void proc::Execute()
{
    if(exec_list.exe_list.size() !=0)
    {
        exec_list.exe_cycle_dec(); // counter to model execution latency
        for (unsigned int i = 0; i < exec_list.exe_list.size();i++) // Check the execute list for all the completed instructions 
        {
            if(exec_list.exe_list[i].exe_cycle == 0) 
            {
                exec_list.exe_list[i].WB_begin = cycle + 1;
                exec_list.exe_list[i].EX_cycles = exec_list.exe_list[i].WB_begin - exec_list.exe_list[i].EX_begin;

                WB.preg.push_back(exec_list.exe_list[i]); // Add the inst to WB

                // Wakeup dependent instructions in IQ
                for (unsigned int j = 0; j < IQ.iq.size(); j++)
                {
                    if (IQ.iq[j].src1 == exec_list.exe_list[i].dest)
                        IQ.iq[j].src1_rdy = true;
                    if (IQ.iq[j].src2 == exec_list.exe_list[i].dest)
                        IQ.iq[j].src2_rdy = true;
				}

                // Wakeup dependent instructions in DI
                for (unsigned int j = 0; j < DI.preg.size(); j++)
                {
                    if (DI.preg[j].src1 == exec_list.exe_list[i].dest)
						DI.preg[j].src1_rdy = true;
                    if (DI.preg[j].src2 == exec_list.exe_list[i].dest)
                        DI.preg[j].src2_rdy = true;
                }

                // Wakeup dependent instructions in RR
                for (unsigned int j = 0; j < RR.preg.size(); j++)
                {
                    if (RR.preg[j].src1 == exec_list.exe_list[i].dest)
						RR.preg[j].src1_rdy = true;
                    if (RR.preg[j].src2 == exec_list.exe_list[i].dest)
                        RR.preg[j].src2_rdy = true;
                }

                exec_list.exe_list.erase(exec_list.exe_list.begin() + i); // Remove the instruction from the execute list
                i--;

            }
        }
    }
} // END : Execute

void proc::Writeback()
{
    if (WB.preg.size() != 0) // If WB has writeback bundle 
	{
		for (unsigned int i = 0; i < WB.preg.size(); i++)
		{
			ROB.rob[WB.preg[i].dest].rdy = true; // set the ready tag in the ROB table 
			WB.preg[i].RT_begin = cycle + 1;
			WB.preg[i].WB_cycles = WB.preg[i].RT_begin - WB.preg[i].WB_begin; 
            RT.preg.push_back(WB.preg[i]);
		}
		WB.preg.clear();
	}
} // END : Writeback


void proc::Retire()
{
    for(unsigned int i =0; i< width ; i++)
    {
        if (ROB.rob[head].rdy) // check if the head is ready 
		{
            for (unsigned int j = 0; j < RT.preg.size(); j++)
			{
				if (RT.preg[j].seq_no == ROB.rob[head].seq_no)
				{
					RT.preg[j].RT_cycles = (cycle + 1) - RT.preg[j].RT_begin;
                    printSummary(j);                          // print the summary of the retiring instruction 
					RT.preg.erase(RT.preg.begin() + j);       // remove the instruction from the retire 
					break;
                }
			}
            for (unsigned int k = 0; k < RR.preg.size(); k++) // Waking up RR with the retired source
			{
				if (RR.preg[k].src1 == head)
					RR.preg[k].src1_rdy = true;

				if (RR.preg[k].src2 == head)
					RR.preg[k].src2_rdy = true;
			}
            for (unsigned int l = 0; l < RMTSize; l++) // Updating the RMT
			{
				if (RMT[l].ROBTag == head)
				{
					RMT[l].ROBTag = 0;
					RMT[l].valid = false;
				}
			}
			ROB.rob[head].clear();

			head = (head + 1) % ROBSize;

        }
        else 
            break;
            
    }
}


