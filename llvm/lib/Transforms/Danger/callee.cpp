#include "llvm-c/Core.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
using namespace std;
using namespace llvm;
#include "llvm/ADT/StringRef.h"
//Manipulate loops
#include "llvm/Analysis/LoopInfo.h"
#include "danger.h"
#include <vector>
#include <iostream>


	//verify if the pointer doesn't alias another already in the map and increment penalty
	Value* checkAliasPointer(AliasAnalysis &aliasAnalysis, Value * v){
		std::map <Value*, int>::iterator i = danger.begin();
		std::map <Value*, int>::iterator e = danger.end();
		
		//errs()<<"RECHERCHE D'ALIAS"<<'\n';
		while (i != e) {
			//errs() << i->first << " : " << i->second << "\n";
			Value* v1 = (Value *)i->first;
			errs()<<"test "<<aliasAnalysis.alias(v,v1)<<"\n";
				
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
					errs()<<"No alias \n";
				}
				default:
					errs()<<"Pas d'alias de pointeur \n";

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
	//void checkUse(llvm::Instruction){

	//}

	//An instruction
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


	//Affiche les résultats
	void print_danger(){
		
		std::map <Value*, int>::iterator i = danger.begin();
		std::map <Value*, int>::iterator e = danger.end();
		
		errs()<<"OPERATION"<<'\n';
		while (i != e) {
			errs() << i->first << " : " << i->second << "\n";
			i++;
		}
		errs() << "\n";
		danger.clear();
	}

	//Inspect all Malloc call
	/*void inspectMalloc(llvm::CallBase *CB,llvm::FunctionCallee secureMalloc){

		const static llvm::StringRef TargetFunName = "malloc";
		// Only find direct function calls.
		//errs()<<CB->getCalledFunction()->getName().str()<<" ok \n";
		if (!CB->isIndirectCall() && CB->getCalledFunction() &&
			CB->getCalledFunction()->getName().str() ==
				TargetFunName) {
		errs() << "found a direct call to '" << TargetFunName
					<< "'!\n";
		
		std::map <Value*, int>::iterator i = danger.begin();
		std::map <Value*, int>::iterator e = danger.end();
		
		
		while (i != e) {
			//errs() << i->first << " : " << i->second << "\n";
			Value* v1 = (Value *)i->first;
				
			if(CB->hasArgument(v1)){
				errs()<<"******************"<<'\n';
				errs()<<"Argument IDENTIFIE"<<'\n';
				//remplacons les malloc(taille) par des secure_malloc(taille,penalty)

				//1429 1472 1503
				Function *fn = 
				CB->setCalledFunction(fn);
				errs()<<"--Okay--"<<'\n';
				//Attribute pn = 
				vector<Value *>  args;
				args.push_back(CB->getOperand(0));
				Type *voidTy = Type::getInt32Ty(LLVMContext);
				args.push_back(i->second);
				//CB->addParamAttr(2,args);
				CallInst::Create(secureMalloc,args,"",CB);
				errs()<<"******************"<<'\n';
			}
			i++;
		} 
		
		}

	}
	*/