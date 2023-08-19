#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/LoopPeel.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SizeOpts.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopUnrollAnalyzer.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

#include <unordered_map>
#include <vector>

using namespace llvm;

namespace {

struct LoopInversionPass : public LoopPass {
  static char ID;
  LoopInversionPass() : LoopPass(ID) {};

  int CounterValue = -1;
  int BoundValue = -1;
  Value *LoopCounter = nullptr;
  Value *LoopBound = nullptr;

  std::vector<BasicBlock *> LoopBasicBlocks = {};
  std::unordered_map<Value *, Value *> VariablesMap = {};

  void MapVariables()
  {
    Function *F = LoopBasicBlocks[0]->getParent();

    for (BasicBlock &BB : *F) {
      for (Instruction &Instr : BB) {
        if (isa<LoadInst>(&Instr))
          VariablesMap[&Instr] = Instr.getOperand(0);
      }
    }

    errs() << "--- Variables mapping ---\n";
    for (auto kvp : VariablesMap) {
      kvp.first->print(errs(), false);
      errs() << " -> ";
      kvp.second->print(errs(), false);
      errs() << "\n";
    }
  }

  void FindLoopBoundAndCounter(Loop *L)
  {
    // %5 = icmp slt i32 %4, 10
    for (Instruction &Instr : *LoopBasicBlocks[0]) {
      if (isa<ICmpInst>(&Instr)) {
        LoopCounter = VariablesMap[Instr.getOperand(0)];

        // Potrebno je proveriti da li je granica konstanta ili je sacuvana u nekoj pomocnoj promenljivoj
        if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr.getOperand(1))) {
          LoopBound = Instr.getOperand(1);
        } else {
          LoopBound = VariablesMap[Instr.getOperand(1)];
        }
      }
    }

    // Vrednost brojaca:
    // store i32 0, ptr %2, align 4
    for (Instruction &Instr : *L->getLoopPreheader()) {
      if (isa<StoreInst>(&Instr) && Instr.getOperand(1) == LoopCounter) {
        ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr.getOperand(0));
        CounterValue = ConstInt->getSExtValue();
      }
      if (isa<StoreInst>(&Instr) && Instr.getOperand(1) == LoopBound) {
        ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr.getOperand(0));
        BoundValue = ConstInt->getSExtValue();
      }
    }

    errs() << "--- Loop parameters ---\nLoop bound: ";
    LoopBound->printAsOperand(errs(), false);
    errs() << "\nLoop counter: ";
    LoopCounter->printAsOperand(errs(), false);
    errs() << "\nCounter value: " << CounterValue << "\n";
  }

  void CopyInstructions(BasicBlock *From, BasicBlock *To)
  {
    Instruction *InstrCopy = nullptr;
    std::unordered_map<Value *, Value *> Mapping = {};

    for (Instruction &Instr : *From) {
      if (!Instr.isTerminator()) {
        InstrCopy = Instr.clone();
        InstrCopy->insertBefore(To->getTerminator());
        Mapping[&Instr] = InstrCopy;

        for (size_t i = 0; i < InstrCopy->getNumOperands(); ++i) {
          if (Mapping.find(InstrCopy->getOperand(i)) != Mapping.end())
            InstrCopy->setOperand(i, Mapping[Instr.getOperand(i)]);
        }
      }
    }

    To->getTerminator()->setSuccessor(0, LoopBasicBlocks.front());
  }

//  BasicBlock *CreateBasicBlock()
//  {
//    BasicBlock *NewBasicBlock = BasicBlock::Create(LoopBasicBlocks.front()->getContext());
//    NewBasicBlock->insertInto(LoopBasicBlocks.front()->getParent(), LoopBasicBlocks.front());
//
//    Instruction *InstrCopy = nullptr;
//    std::unordered_map<Value *, Value *> Mapping = {};
//
//    IRBuilder<> Builder(NewBasicBlock);
//    for (Instruction &Instr : *LoopBasicBlocks.front()) {
//      InstrCopy = Instr.clone();
//      Builder.Insert(InstrCopy);
//      Mapping[&Instr] = InstrCopy;
//
//      for (size_t i = 0; i < InstrCopy->getNumOperands(); ++i) {
//        if (Mapping.find(InstrCopy->getOperand(i)) != Mapping.end())
//          InstrCopy->setOperand(i, Mapping[Instr.getOperand(i)]);
//      }
//    }
//
//    errs() << "--- Copied basic block ---\n";
//    for (Instruction &Instr : *NewBasicBlock) {
//      Instr.print(errs(), false);
//      errs() << "\n";
//    }
//
//    return NewBasicBlock;
//  }

  void LoopInversion(Loop *L)
  {
    // BasicBlock *NewBasicBlock = CreateBasicBlock();
    // Alternativa za kopiranje BB jeste da se sve instrukcije kreiraju rucno
    BasicBlock *NewBasicBlock = BasicBlock::Create(LoopBasicBlocks.front()->getContext());
    NewBasicBlock->insertInto(LoopBasicBlocks.front()->getParent(), LoopBasicBlocks.front());

    IRBuilder<> Builder(NewBasicBlock);
    // %4 = load i32, ptr %2, align 4
    // %5 = icmp slt i32 %4, 10
    // br i1 %5, label %6, label %10
    Value *Load = Builder.CreateLoad(Type::getInt32Ty(NewBasicBlock->getContext()), LoopCounter);
    Value *CMP = Builder.CreateICmp(CmpInst::ICMP_SLT, Load, LoopBound);
    Builder.CreateCondBr(CMP,
                         LoopBasicBlocks.front()->getTerminator()->getSuccessor(0),
                         LoopBasicBlocks.front()->getTerminator()->getSuccessor(1));

    errs() << "--- Instructions ---\n";
    for (Instruction &Instr : *NewBasicBlock) {
      Instr.print(errs(), false);
      errs() << "\n";
    }

    L->getLoopPreheader()->getTerminator()->setSuccessor(0, NewBasicBlock);
    LoopBasicBlocks.front()->moveBefore(LoopBasicBlocks.back());
    CopyInstructions(LoopBasicBlocks.back(), LoopBasicBlocks[LoopBasicBlocks.size() - 2]);
//    LoopBasicBlocks.back()->eraseFromParent(); <- ovde puca
  }

  bool runOnLoop(Loop *L, LPPassManager &) override {
    LoopBasicBlocks = L->getBlocksVector();

    errs() << "--- Loop basic blocks ---\n";
    for (BasicBlock *BB : LoopBasicBlocks) {
      BB->printAsOperand(errs(), false);
      errs() << "\n";
    }

    MapVariables();
    FindLoopBoundAndCounter(L);
    LoopInversion(L);

    return true;
  }
};

}

char LoopInversionPass::ID = 0;
static RegisterPass<LoopInversionPass> X("loop-inversion", "Loop inversion pass");