#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"
#include "proc.h"

using namespace std;

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    //int op_type, dest, src1, src2;  // Variables are read from trace file
    //uint64_t pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    
    
    proc p1(params.rob_size,params.iq_size,params.width);

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //    printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly
  
    do{
        p1.Retire();
        p1.Writeback();
        p1.Execute();
        p1.Issue();
        p1.Dispatch();
        p1.RegRead();
        p1.Rename();
        p1.Decode();
        p1.Fetch(FP);
        if (p1.DE.empty && p1.RN.empty && p1.RR.empty && p1.DI.empty && p1.IQ.iq.size() == 0 && p1.exec_list.exe_list.size() == 0 && p1.WB.preg.size() == 0 && p1.RT.preg.size() == 0)
			if (p1.head == p1.tail)
				if (p1.ROB.rob[p1.tail].seq_no == 0)
					p1.pipelineEmpty = true;

		p1.cycle++;
    } while (p1.Advance_Cycle(FP));

    fclose(FP);
    cout << "# === Simulator Command =========" << endl;
    cout << "# ";
	for (int i = 0; i < argc; i++)
		cout << argv[i] << " ";
    cout << endl;
	cout << "# === Processor Configuration ===" << endl;
	cout << "# ROB_SIZE = " << p1.ROBSize << endl;
	cout << "# IQ_SIZE  = " << p1.IQSize << endl;
	cout << "# WIDTH    = " << p1.width << endl;
	cout << "# === Simulation Results ========" << endl;
	cout << "# Dynamic Instruction Count    = " << p1.seq_no << endl;
	cout << "# Cycles                       = " << p1.cycle << endl;
	cout << "# Instructions Per Cycle (IPC) = " << fixed << setprecision(2) << ((float)p1.seq_no / (float)p1.cycle) << endl;
    return 0;
}
