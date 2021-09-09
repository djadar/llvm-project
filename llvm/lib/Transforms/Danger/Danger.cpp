#include "llvm-c/Core.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Dominators.h"
#include <cstdint>
#define DEBUG_TYPE "danger"

//#include "callee.cpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
using namespace std;
using namespace llvm;
#include "llvm/ADT/StringRef.h"
//Manipulate loops
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/DerivedTypes.h"

#include <vector>
#include <iostream>
#include "llvm-c/Core.h"

#define SECURE_MALLOC "secure_malloc"

const static llvm::StringRef TargetFunName = "malloc";

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
//List of pointers and their penalties
static std::map<llvm::Value *, int> danger;
static std::map<llvm::Value *, int> copyDanger;

namespace {

	struct Danger : public ModulePass{
		//list pointer-penalty : string, tableau[2] : pointer penalty size
		
		static char ID;
		Danger() : llvm::ModulePass(ID) {}

		

		//An LLVM pass must declare which other passes it requires to execute properly
		void getAnalysisUsage(AnalysisUsage &AU)const override{
			
			AU.addRequired<AAResultsWrapperPass>();
			AU.addRequired<LoopInfoWrapperPass>();
			//AU.setPreservesAll();
			//for transformations that change instructions in the program but do not modify the CFG or terminator instructions
			AU.setPreservesCFG();
		}

			//verify if the pointer doesn't alias another already in the map and increment penalty
		Value* checkAliasPointer(AliasAnalysis &aliasAnalysis, Value * v){
			std::map <Value*, int>::iterator i = danger.begin();
			std::map <Value*, int>::iterator e = danger.end();
			
			//errs()<<"RECHERCHE D'ALIAS"<<'\n';
			while (i != e) {
				//errs() << i->first << " : " << i->second << "\n";
				Value* v1 = (Value *)i->first;
				//errs()<<"test "<<aliasAnalysis.alias(v,v1)<<"\n";
					
				switch(aliasAnalysis.alias(v,v1)){
					case AliasResult::MayAlias:{
						//errs()<<"okay1\n";
						return i->first;
					}	
					case AliasResult::MustAlias:{
						//errs()<<"okay2\n";
						return i->first;
					}
					case AliasResult::PartialAlias:{
						//errs()<<"okay3\n";
						return i->first;
					}
					case AliasResult::NoAlias:{
						//errs()<<"No alias \n";
					}
					default: ;
						//errs()<<"Pas d'alias de pointeur \n";

				}

				i++;
				
			}
			return NULL;
		}

		//Construct the final map pointer-penalty
		void fill_list(AliasAnalysis &aliasAnalysis, Value *pointer, int penality){
			//if the pointer is already in the list
			if (danger.find(pointer) != danger.end()){
				danger[pointer] = (danger[pointer] > penality) ? danger[pointer]: penality;
			}
			//if not
			else {
				//We verify if another pointer alias him
				Value* V = checkAliasPointer(aliasAnalysis,pointer);
				//if so we choose the max penalty
				if (V!=NULL)
					danger[V] = (danger[V] > penality) ? danger[V]: penality;
				else
					danger[pointer] = penality;
			}

				
		}

		//check if the uses of an instruction are contains in the users of other instructions

		void InstructionsInLoop(Loop *L, AliasAnalysis &aliasAnalysis) {
				
			Loop::block_iterator bb;

			//Fill map operations
			fill_operation();

			//Identification of operation schemes
			for(bb = L->block_begin(); bb != L->block_end();++bb){
				for (BasicBlock::iterator i = (*bb)->begin(), e = (*bb)->end(); i != e; ++i) {
					
					//errs()<<"L'instruction est : "<<i->getOpcodeName() <<" et a "<< i->getNumOperands()<< "opérandes"<<"\n";
					
					//getElementptr suivi d'un load ou d'un store - dereferencement en écriture et lecture
					/*llvm::Instruction inst= *i;
					checkUse(*i);*/
					if(GetElementPtrInst *inst = dyn_cast<GetElementPtrInst>(i)){
						
						if(StoreInst *storeInst = dyn_cast<llvm::StoreInst>(i->getNextNode())){
							
							//errs() << "dereferencement en écriture"<< '\n';

							//if(PointerType *pointerType=dyn_cast<PointerType>(i->getOperand(1)->getType()->getPointerElementType() ))
							fill_list(aliasAnalysis,storeInst->getOperand(1), operations["biai"]);
						}
						else if(LoadInst *loadInst = dyn_cast<llvm::LoadInst>(i->getNextNode())){
							
							//errs() << "dereferencement en lecture"<< '\n';
							fill_list(aliasAnalysis,loadInst->getOperand(0), operations["biai"]);
						}
					}
					//2 load suivi d'un store
					if(llvm::isa<llvm::LoadInst>(i) && llvm::isa<llvm::LoadInst>(i->getNextNode())){
						
						if(i->getValueName() == (i->getNextNode())->getOperand(0)->getValueName()){
							
							if((i->getNextNode())->getValueName() == ((i->getNextNode())->getNextNode())->getOperand(0)->getValueName()){
								//errs() << "double load suivi d'un store"<< '\n';
								fill_list(aliasAnalysis,i->getNextNode()->getOperand(0), operations["biai"]);
							}
						}
					}
					//cast de pointeur
					if(CastInst *inst = dyn_cast<CastInst>(i)){
						//errs() << "Cast de pointeur"<< '\n';
						fill_list(aliasAnalysis,inst->getOperand(0), operations["cast"]);
					}
					//errs() << "L'opération sur pointeur est :" << s << "\n";
					//Check the case where we load a pointer calcultaed outside the loop
					Instruction *inst = dyn_cast<Instruction>(i);
					if (L->hasLoopInvariantOperands(inst)){
						//errs() << "pointeur calculé à l'extérieur de la boucle mais utilisé"<< '\n';
						fill_list(aliasAnalysis,inst->getOperand(0), 0);
					}
				}
			}
		}

		//print result
	
		void print_danger(){
			
			std::map <Value*, int>::iterator i = danger.begin();
			std::map <Value*, int>::iterator e = danger.end();
			
			errs()<<"/*************\nOPERATION"<<"\n*****************/\n";
			while (i != e) {
				errs() << i->first << " : " << i->second << "\n";
				//copyDanger[i->first] = i->second;
				i++;
			}
			errs() << "\n";
			//Clean the map 
			errs() << "taille DANGER = "<< danger.size() <<" \n";
			
			copyDanger.insert(danger.begin(), danger.end());
			//errs() << "taille copyDanger = "<< copyDanger.size() <<" \n";
			danger.clear();

		}
			
		void setupFunction(Module& M){
			
			auto &Context = M.getContext();

			//FunctionType* voidTy = M.getFunction(TargetFunName)->getFunctionType();
			Type* voidTy = Type::getInt8PtrTy(Context);
			Type* intTy = Type::getInt64Ty(Context);
			vector<Type *>  args;
			args.push_back(intTy);
			args.push_back(intTy);
			// Specify the return value, arguments, and if there are variable numbers of arguments.
			FunctionType* funcTy = FunctionType::get(voidTy, args, false);
			//Function::Create(funcTy, llvm::GlobalValue::ExternalLinkage)->setName(SECURE_MALLOC);
			Function::Create(funcTy, Function::ExternalLinkage, SECURE_MALLOC, M);
			errs()<< "Also Function \n" << *funcTy <<"\n";
		}

		void InstrumentNewFunction(llvm::CallInst *CI, Module& M, int penalty){ 
			
			//the function takes the malloc CallInst as argument
			// change malloc to secureMalloc

			// you create secure malloc here
			Function* secureMalloc = M.getFunction(SECURE_MALLOC);

			//Verify it 
			errs()<< "Function \n" << *secureMalloc <<"\n";

			//
			LLVMContext &context = M.getContext();
			SmallVector<Value*, 16> new_args;
			new_args.push_back(CI->getArgOperand(0));
		    // example où penalité est 42
			new_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), penalty));

			CallInst* new_CI = CallInst::Create(secureMalloc, new_args, "", CI);
			new_CI->setCallingConv(CI->getCallingConv());
			new_CI->setAttributes(CI->getAttributes());
			new_CI->setDebugLoc(CI->getDebugLoc());
			// Replace old call with new one.
			CI->replaceAllUsesWith(new_CI);
			// take the old ones name
			new_CI->takeName(CI);
				
		}
		
		//BEGIN
		virtual bool runOnModule(Module &M) override{
		
      		//Fill the map of dangerous operations
			fill_operation();
      
      		//we initialize new function secure_malloc
			setupFunction(M);

			for (Module::iterator Fit = M.begin(), Fite = M.end(); Fit != Fite; ++Fit) {
				Function *F = &(*Fit);
				errs() << "Function " << F->getName() << "\n";

				//Verify it 
				//errs()<< "Function \n" << *F <<"\n";

				//we do not consider function with keyword declare 
				if (F->isDeclaration()) continue;

				//To recover the datastructures created by the pass, we can use the getAnalysis method
				//for manipulating loops
				LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();	
				
				//For checking alias analysis
				AliasAnalysis &aliasAnalysis = getAnalysis<AAResultsWrapperPass>(*F).getAAResults();

          
				//We visit loops searching for instructions dereferencing pointers
				//we assign and increment penalties of operations on those pointers

				unsigned num = 0;
				errs()<<"/*************\n CHECK"<<"\n*****************/\n";
				for (LoopInfo::iterator I = LI.begin(), E = LI.end(); I != E; ++I) {
					errs() << "Loop " << num <<"\n";
					//Loop *L = *I;
					InstructionsInLoop(*I, aliasAnalysis);
					
					num++;	
				}
				//Printing danger list of the current function
				//And Resolving NotDifferentParent Error
				//copy danger;
				
				
				print_danger();
				
				
				
				errs() << " COPY= "<< copyDanger.size() <<" \n";
				errs() << " DANGER= "<< danger.size() <<" \n";
			}
			
				//*****
					
					//begin Transformation

				//*****

				//List  of Malloc calls
				vector<llvm::CallInst *> callinst;

				for (auto &F : M) {
					for (auto &BB : F) {
						for (auto &I : BB) {
							if (auto *CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
								//
								//if call to malloc function
								if (!CI->isIndirectCall() && CI->getCalledFunction() &&
									CI->getCalledFunction()->getName().str() ==
											TargetFunName) {
									
									errs() << "found a direct call to '" << TargetFunName
												<< "'!\n";
									
									callinst.push_back(CI);
								}
							}
						}
					}
				}
				

				//Replacement of funtion calls
				errs()<<"/*************\n REPLACEMENT"<<"\n*****************/\n";
				errs() << "taille CALLInST = "<< callinst.size() <<" \n";
				errs() << "taille COPY= "<< copyDanger.size() <<" \n";

				for (auto CI : callinst){
					//we read the values in the map
					std::map <Value*, int>::iterator i = copyDanger.begin();
					std::map <Value*, int>::iterator e = copyDanger.end();
					//errs() << "Test 0 \n";
					while (i != e) {
						//errs() << "Test 1 \n";
						errs() << i->first << " : " << i->second << "\n";
						Value* v1 = (Value *)i->first;
							
						if(CI->hasArgument(v1)){
							errs() << "Test 2 \n";
							InstrumentNewFunction(CI,M,i->second);
						}
						i++;
					}
				}
				
				errs()<<"/*************\n DELETE"<<"\n*****************/\n";
				
				for (auto CI :callinst)
				{
					// At some point write this to delete the old CI
					//CI->removeFromParent();
					//errs()<<"ok\n";
					CI->eraseFromParent();
					errs()<<"Done\n";
				}
			
			
			//return true because we modify IR
			return true;
		}
	};
} // namespace

char Danger::ID = 0;
static RegisterPass<Danger> X("Danger", "Get vulnerable buffers per functions");
