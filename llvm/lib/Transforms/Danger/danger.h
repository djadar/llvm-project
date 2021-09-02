//inspired by Dowser
#include <map>
using namespace llvm;
#include "llvm/IR/Value.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"


#define SECURE_MALLOC "secure_malloc"

//List of pointers and their penalties
static std::map<llvm::Value *, int> danger;
//List of operation and their penalties
static std::map<std::string, int> operations;

//fill the list of operation
static void fill_operation(){
	//basic index aritmetic instruction like addition and subtraction : biai 
    operations["biai"]=5;
    //An instructions scores 1 if it modifies a value which is not passes to the next loop iteration
    operations["biai2"]=1;
    //other index aritmetic instruction like division, shift and xor

    //Different constant values

    //constants used to access fields of structures

    //numerical values determined outside the loop

    //Non-inlined functions returning non-pointer values

    //Data movement instructions

    //load a pointer calculates outside the loop
    operations["load_ext"]=0;
    //GetElemptr instruction
    operations["getelemptr"]=1;

    //pointer cast operations
    operations["cast"]=100;
    
}

llvm::Value* checkAliasPointer(AliasAnalysis &aliasAnalysis,Value * v);

void InstructionsInLoop(Loop *L, AliasAnalysis &aliasAnalysis);

void print_danger();

//void inspectMalloc(llvm::CallBase *CB);

void setupHooks(Module& M);

void InstrumentEnterFunction(llvm::CallBase *CB, Module& M);