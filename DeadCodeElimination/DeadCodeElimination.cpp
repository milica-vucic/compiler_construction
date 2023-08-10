#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "CFG.h"

#include <unordered_set>
#include <unordered_map>

using namespace llvm;

namespace {

struct DeadCodeEliminationPass : public FunctionPass {
  static char ID;
  DeadCodeEliminationPass() : FunctionPass(ID) {};

  bool EliminateInstruction;

  void EliminateUnusedVariables(Function &F)
  {
    std::unordered_map<Value *, Value *> VariablesMap = {};
    // instrukcija -> da li se promenljiva u njoj koristi negde ili ne
    std::unordered_map<Value *, bool> Variables = {};
    std::unordered_set<Instruction *> InstructionsToRemove = {};

    errs() << "======= ALL INSTRUCTIONS =======\n";
    for (BasicBlock &BB : F) {
      for (Instruction &Instr : BB)
        errs() << Instr.getOpcodeName() << "\n";
    }
    errs() << "\n";

    for (BasicBlock &BB : F) {
      for (Instruction &Instr : BB) {
        if (Instr.getType() != Type::getVoidTy(Instr.getContext())) {
          // ako instrukcija nije call, onda postavljamo da promenljiva nije koriscena
          if (!isa<CallInst>(&Instr))
            Variables[&Instr] = false;
        }

        if (isa<LoadInst>(&Instr))
          VariablesMap[&Instr] = Instr.getOperand(0);

        if (isa<StoreInst>(&Instr)) {
          if (Variables.find(Instr.getOperand(0)) != Variables.end()) {
            Variables[Instr.getOperand(0)] = true;
            Variables[VariablesMap[Instr.getOperand(0)]] = true;
          }
        } else {
          // %4 = alloca i32, align 4
          int NumOfOperands = (int)Instr.getNumOperands();
          for (int i = 0; i < NumOfOperands; ++i) {
            // ako operand vec postoji u mapi, onda ce biti koriscen
            // gledamo i u sta je bio mapiran, pa i to oznacavamo kao nesto sto se koristi u kodu
            if (Variables.find(Instr.getOperand(i)) != Variables.end()) {
              Variables[Instr.getOperand(i)] = true;
              Variables[VariablesMap[Instr.getOperand(i)]] = true;
            }
          }
        }
      }
    }

    for (BasicBlock &BB : F) {
      for (Instruction &Instr : BB) {
        if (isa<StoreInst>(&Instr)) {
          if (!Variables[Instr.getOperand(1)])
            InstructionsToRemove.insert(&Instr);
        } else if (Variables.find(&Instr) != Variables.end() && !Variables[&Instr]) {
          InstructionsToRemove.insert(&Instr);
        }
      }
    }
    errs() << "======= INSTRUCTIONS TO BE REMOVED =======\n";
    for (Instruction *Instr : InstructionsToRemove)
      errs() << Instr->getOpcodeName() << " " << Instr->getNumOperands() << "\n";
    errs() << "\n";

    if (!InstructionsToRemove.empty())
      EliminateInstruction = true;

    for (Instruction *Instr : InstructionsToRemove)
      Instr->eraseFromParent();
  }

  void EliminateUnreachableInstructions(Function &F)
  {
    CFG *Graph = new CFG(F);
    Graph->TraverseGraph();

    std::vector<BasicBlock *> BasicBlocksToRemove = {};
    for (BasicBlock &BB : F)
      if (!Graph->IsReachable(&BB))
        BasicBlocksToRemove.push_back(&BB);

    if (!BasicBlocksToRemove.empty())
      EliminateInstruction = true;

    for (BasicBlock *BB : BasicBlocksToRemove)
      BB->eraseFromParent();
  }

  void RunAlgorithm(Function &F)
  {
    EliminateInstruction = false;
    EliminateUnusedVariables(F);
    EliminateUnreachableInstructions(F);
  }

  bool runOnFunction(Function &F) override {
    do {
      RunAlgorithm(F);
    } while (EliminateInstruction);
    return true;
  }
};

}

char DeadCodeEliminationPass::ID = 0;
static RegisterPass<DeadCodeEliminationPass> X("dead-code-elimination", "Dead code elimination pass.");