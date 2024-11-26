/*
 * Copyright (C) 2007-2023 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
using std::cerr;
using std::endl;
using std::string;

/* ================================================================== */
// Global variables
/* ================================================================== */

std::vector<bool>* branches;
std::vector<uintptr_t>* addrs;
std::vector<uintptr_t>* targs;
uintptr_t limit;

std::ostream* out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB< string > KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "BranchTrace.out", "specify file name for output");
KNOB< uintptr_t > KnobLimit(KNOB_MODE_WRITEONCE, "pintool", "l", "", "specify file name for output");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool prints out the outcomes of conditional branch" << endl
         << "instructions, as well as their addresses during execution" << endl
         << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

/*!
 * Increase counter of the executed basic blocks and instructions.
 * This function is called for every basic block when it is about to be executed.
 * @param[in]   numInstInBbl    number of instructions in the basic block
 * @note use atomic operations for multi-threaded applications
 */
VOID OnBranch(void* pc, void* target, bool taken)
{   
    if(branches->size() <= limit) {
        addrs->push_back((uintptr_t)pc);
        targs->push_back((uintptr_t)target);
        if(taken) {
            branches->push_back(true);
        }else {
            branches->push_back(false);
        }
    }
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

/*!
 * Insert call to the Instruction() analysis routine for every instruction
 * @param[in]   ins      instruction to be instrumented
 * @param[in]   v        value specified by the tool in the TRACE_AddInstrumentFunction
 *                       function call
 */
void Instruction(INS ins, void* v) {

    //check if it is a branch instruction
    if(INS_IsBranch(ins) && INS_HasFallThrough(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)OnBranch, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
    }

}
/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
void Fini(INT32 code, void* v)
{  
    cerr << "Instrumented a total of " << branches->size() << " branches." << endl;
    *out << std::hex;
    *out << branches->size() << endl;
    for(uintptr_t i = 0; i < branches->size(); i++) {
        bool branch = branches->operator[](i);
        if(branch) {
            *out << "1" << endl;
        }else {
            *out << "0" << endl;
        }
        *out << addrs->operator[](i) << endl;
    }
    delete branches;
    delete addrs;
    delete targs;
}
/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char* argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    string fileName = KnobOutputFile.Value();
    limit = KnobLimit.Value();

    if (!fileName.empty())
    {
        out = new std::ofstream(fileName.c_str());
    }
    if(limit == 0) {
        limit = std::numeric_limits<uintptr_t>::max();
    }
    branches = new std::vector<bool>();
    addrs = new std::vector<uintptr_t>();
    targs = new std::vector<uintptr_t>();

    // Register function to be called for every instruction
    INS_AddInstrumentFunction(Instruction, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    cerr << "===============================================" << endl;
    cerr << "This application is instrumented by BranchTrace" << endl;
    if (!KnobOutputFile.Value().empty())
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    cerr << "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
