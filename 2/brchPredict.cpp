#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include "pin.H"

using namespace std;

ofstream OutFile;

#define truncate(val, bits) ((val) & ((1 << (bits)) - 1))

static UINT64 takenCorrect = 0;
static UINT64 takenIncorrect = 0;
static UINT64 notTakenCorrect = 0;
static UINT64 notTakenIncorrect = 0;

template <size_t N, UINT64 init = (1 << N) / 2 - 1> // N < 64
class SaturatingCnt
{
    UINT64 val;

public:
    SaturatingCnt() { reset(); }

    void increase()
    {
        if (val < (1 << N) - 1)
            val++;
    }
    void decrease()
    {
        if (val > 0)
            val--;
    }

    void reset() { val = init; }
    UINT64 getVal() { return val; }

    //预测分支是否跳转
    BOOL isTaken() { return (val > (1 << N) / 2 - 1); }
};

template <size_t N> // N < 64
class ShiftReg
{
    UINT64 val;

public:
    ShiftReg() { val = 0; }

    bool shiftIn(bool b)
    {
        bool ret = !!(val & (1 << (N - 1))); //取val最高位并转为bool
        val <<= 1;
        val |= b;            //两条语句连起来即val = (val << 1) | b
        val &= (1 << N) - 1; //取val的低N位
        return ret;
    }

    UINT64 getVal() { return val; }
};

class BranchPredictor
{
public:
    BranchPredictor() {}
    virtual BOOL predict(ADDRINT addr) { return FALSE; };
    virtual void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr){};
};

BranchPredictor *BP;

/* ===================================================================== */
/* 至少需实现2种动态预测方法                                               */
/* ===================================================================== */
// 1. BHT-based branch predictor
template <size_t L>
class BHTPredictor : public BranchPredictor
{
    SaturatingCnt<2> counter[1 << L];

public:
    BHTPredictor() {}

    BOOL predict(ADDRINT addr)
    {
        // TODO:
        return counter[truncate(addr, L)].isTaken();
    }

    void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
    {
        // TODO:
        UINT64 index = truncate(addr, L);
        if (takenActually)
        {
            if (counter[index].getVal() == 0b01)
                counter[index].increase();
            counter[index].increase();
        }
        else
        {
            if (counter[index].getVal() == 0b10)
                counter[index].decrease();
            counter[index].decrease();
        }
    }
};

// 2. Global-history-based branch predictor
template <size_t L, size_t H, UINT64 BITS = 2>
class GlobalHistoryPredictor : public BranchPredictor
{
    SaturatingCnt<BITS> bhist[1 << L]; // PHT中的分支历史字段
    ShiftReg<H> GHR;

    // TODO:
    BOOL predict(ADDRINT addr)
    {
        return bhist[truncate(GHR.getVal() ^ addr, L)].isTaken();
    };

    void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
    {
        UINT64 index = truncate(GHR.getVal() ^ addr, L);
        if (takenActually)
        {

            if (bhist[index].getVal() == 0b01)
                bhist[index].increase();
            bhist[index].increase();
            GHR.shiftIn(true);
        }
        else
        {
            if (bhist[index].getVal() == 0b10)
                bhist[index].decrease();
            bhist[index].decrease();
            GHR.shiftIn(false);
        }
    };
};

// 3. Local-history-based branch predictor
template <size_t L, size_t H, size_t HL = 6, UINT64 BITS = 2>
class LocalHistoryPredictor : public BranchPredictor
{
    SaturatingCnt<BITS> bhist[1 << L]; // PHT中的分支历史字段
    ShiftReg<H> LHT[1 << HL];

    // TODO:
    BOOL predict(ADDRINT addr)
    {
        return bhist[truncate(LHT[truncate(addr, HL)].getVal() ^ addr, L)].isTaken();
    };

    void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
    {
        UINT64 index = truncate(LHT[truncate(addr, HL)].getVal() ^ addr, L);
        if (takenActually)
        {
            if (bhist[index].getVal() == 0b01)
                bhist[index].increase();
            bhist[index].increase();
            LHT[truncate(addr, HL)].shiftIn(true);
        }
        else
        {
            if (bhist[index].getVal() == 0b10)
                bhist[index].decrease();
            bhist[index].decrease();
            LHT[truncate(addr, HL)].shiftIn(false);
        }
    };
};

/* ===================================================================== */
/* 锦标赛预测器的选择机制可用全局法或局部法实现，二选一即可                   */
/* ===================================================================== */
// 1. Tournament predictor: Select output by global selection history
template <UINT64 BITS = 2>
class TournamentPredictor_GSH : public BranchPredictor
{
    SaturatingCnt<BITS> GSHR;
    BranchPredictor *BPs[2];

public:
    TournamentPredictor_GSH(BranchPredictor *BP0, BranchPredictor *BP1)
    {
        BPs[0] = BP0;
        BPs[1] = BP1;
    }

    // TODO:
    BOOL predict(ADDRINT addr)
    {
        return BPs[!!(GSHR.getVal() & (1 << (BITS - 1)))]->predict(addr);
    };

    void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
    {
        BOOL pre0_res = (BPs[0]->predict(addr) == takenActually);
        BOOL pre1_res = (BPs[1]->predict(addr) == takenActually);

        if (!pre0_res && pre1_res)
        {
            if (GSHR.getVal() == 0b01)
                GSHR.increase();
            GSHR.increase();
        }
        else if (pre0_res && !pre1_res)
        {
            if (GSHR.getVal() == 0b10)
                GSHR.decrease();
            GSHR.decrease();
        }

        BPs[0]->update(takenActually, takenPredicted, addr);
        BPs[1]->update(takenActually, takenPredicted, addr);
    };
};

// 2. Tournament predictor: Select output by local selection history
template <size_t L, UINT64 BITS = 2>
class TournamentPredictor_LSH : public BranchPredictor
{
    SaturatingCnt<BITS> LSHT[1 << L];
    BranchPredictor *BPs[2];

public:
    TournamentPredictor_LSH(BranchPredictor *BP0, BranchPredictor *BP1)
    {
        BPs[0] = BP0;
        BPs[1] = BP1;
    }

    // TODO:
    BOOL predict(ADDRINT addr)
    {
        return BPs[!!(LSHT[truncate(addr, L)].getVal() & (1 << (BITS - 1)))]->predict(addr);
    };

    void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
    {
        BOOL pre0_res = (BPs[0]->predict(addr) == takenActually);
        BOOL pre1_res = (BPs[1]->predict(addr) == takenActually);

        if (!pre0_res && pre1_res)
        {
            if (LSHT[truncate(addr, L)].getVal() == 0b01)
                LSHT[truncate(addr, L)].increase();
            LSHT[truncate(addr, L)].increase();
        }
        else if (pre0_res && !pre1_res)
        {
            if (LSHT[truncate(addr, L)].getVal() == 0b10)
                LSHT[truncate(addr, L)].decrease();
            LSHT[truncate(addr, L)].decrease();
        }

        BPs[0]->update(takenActually, takenPredicted, addr);
        BPs[1]->update(takenActually, takenPredicted, addr);
    };
};

// This function is called every time a control-flow instruction is encountered
void predictBranch(ADDRINT pc, BOOL direction)
{
    BOOL prediction = BP->predict(pc);
    BP->update(direction, prediction, pc);
    if (prediction)
    {
        if (direction)
            takenCorrect++;
        else
            takenIncorrect++;
    }
    else
    {
        if (direction)
            notTakenIncorrect++;
        else
            notTakenCorrect++;
    }
}

// Pin calls this function every time a new instruction is encountered
void Instruction(INS ins, void *v)
{
    if (INS_IsControlFlow(ins) && INS_HasFallThrough(ins))
    {
        INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)predictBranch,
                       IARG_INST_PTR, IARG_BOOL, TRUE, IARG_END);

        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)predictBranch,
                       IARG_INST_PTR, IARG_BOOL, FALSE, IARG_END);
    }
}

// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "brchPredict.txt", "specify the output file name");

// This function is called when the application exits
VOID Fini(int, VOID *v)
{
    double precision = 100 * double(takenCorrect + notTakenCorrect) / (takenCorrect + notTakenCorrect + takenIncorrect + notTakenIncorrect);

    cout << "takenCorrect: " << takenCorrect << endl
         << "takenIncorrect: " << takenIncorrect << endl
         << "notTakenCorrect: " << notTakenCorrect << endl
         << "nnotTakenIncorrect: " << notTakenIncorrect << endl
         << "Precision: " << precision << endl;

    OutFile.setf(ios::showbase);
    OutFile << "takenCorrect: " << takenCorrect << endl
            << "takenIncorrect: " << takenIncorrect << endl
            << "notTakenCorrect: " << notTakenCorrect << endl
            << "nnotTakenIncorrect: " << notTakenIncorrect << endl
            << "Precision: " << precision << endl;

    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl
         << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    // TODO: New your Predictor below.
    // BP = new BHTPredictor<16>();
    // BP = new GlobalHistoryPredictor<16, 16>();
    // BP = new LocalHistoryPredictor<16, 16>();
    BP = new TournamentPredictor_GSH<>(new BHTPredictor<16, 16>(),
                                       new GlobalHistoryPredictor<16, 16>());
    // BP = new TournamentPredictor_LSH<12>(new BHTPredictor<16, 16>(),
    //                                      new LocalHistoryPredictor<16, 16>());

    // Initialize pin
    if (PIN_Init(argc, argv))
        return Usage();

    OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
