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
#include "danger.h"

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

		constexpr const static llvm::StringRef TargetFunName = "malloc";
			
		void setupFunction(Module& M){
			
			auto &Context = M.getContext();

			FunctionType* voidTy = M.getFunction(TargetFunName)->getFunctionType();
			//Type* voidTy = Type::getInt8PtrTy(Context);
			Type* intTy = Type::getInt64Ty(Context);
			vector<Type *>  args;
			args.push_back(intTy);
			args.push_back(intTy);
			// Specify the return value, arguments, and if there are variable numbers of arguments.
			FunctionType* funcTy = FunctionType::get(voidTy, args, false);
			Function::Create(funcTy, llvm::GlobalValue::ExternalLinkage)->setName(SECURE_MALLOC);
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
		
		virtual bool runOnModule(Module &M) override{
		
      		//we //Fill map operations
			fill_operation();
      
      		//we initialize new function secure_malloc
			//setupFunction(M);
			
			for (Module::iterator Fit = M.begin(), Fite = M.end(); Fit != Fite; ++Fit) {
				Function *F = &(*Fit);
				errs() << "Function " << F->getName() << "\n";

				//Verify it 
				errs()<< "Function \n" << *F <<"\n";

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
				for (LoopInfo::iterator I = LI.begin(), E = LI.end(); I != E; ++I) {
					errs() << "Loop " << num <<"\n";
					//Loop *L = *I;
					InstructionsInLoop(*I, aliasAnalysis);
					
					num++;	
				}
				//Printing danger list of the current function
				//And Resolving NotDifferentParent Error
				print_danger();
				
			}
				//*****
					
					//begin Transformation

				//*****

				//List  of Malloc calls
				/*vector<llvm::CallInst *> callinst;

				for (auto &F : M) {
					for (auto &BB : F) {
						for (auto &I : BB) {
							if (auto *CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
						
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
				}*/

				/*for (auto CI : callinst){
					std::map <Value*, int>::iterator i = danger.begin();
					std::map <Value*, int>::iterator e = danger.end();
				
					while (i != e) {
						//errs() << i->first << " : " << i->second << "\n";
						Value* v1 = (Value *)i->first;
							
						if(CI->hasArgument(v1)){
							InstrumentNewFunction(CI,M,i->second);
						}
						i++;
					}
				}*/
				
				/*for (auto CI :callinst)
				{
					// At some point write this to delete the old CI
					CI->eraseFromParent();
				}*/
			
			//return true because we modify IR
			return false;
		}
	};
} // namespace

char Danger::ID = 0;
static RegisterPass<Danger> X("Danger", "Get vulnerable buffers per functions");
