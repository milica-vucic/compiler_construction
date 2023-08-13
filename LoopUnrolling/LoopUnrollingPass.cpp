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

#include <vector>
#include <unordered_map>

using namespace llvm;

namespace {

struct LoopUnrollingPass : public LoopPass {
  static char ID;
  LoopUnrollingPass() : LoopPass(ID) {};

  std::vector<BasicBlock *> LoopBasicBlocks = {};
  std::unordered_map<Value *, Value *> VariablesMap = {};
  Value *LoopCounter = nullptr;
  int LoopBound = -1;
  bool IsLoopBoundConstant;

  void MapVariables(Loop *L) {
    Function *F = L->getHeader()->getParent();

    for (BasicBlock &BB : *F) {
      for (Instruction &Instr : BB) {
        if (isa<LoadInst>(&Instr))
          // %8 = load i32, ptr %6, align 4
          VariablesMap[&Instr] = Instr.getOperand(0);
      }
    }
  }

  void FindLoopBoundAndCounter()
  {
    IsLoopBoundConstant = false;
    for (Instruction &Instr : *LoopBasicBlocks[0]) {
      // %9 = icmp slt i32 %8, 10
      if (isa<ICmpInst>(&Instr)) {
        LoopCounter = VariablesMap[Instr.getOperand(0)];
        if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr.getOperand(1))) {
          IsLoopBoundConstant = true;
          LoopBound = ConstInt->getSExtValue();
        }
      }
    }
  }

  void DuplicateLoop(std::vector<BasicBlock *> LoopBasicBlocks, int NumOfTimes, BasicBlock *MoveBefore)
  {
    BasicBlock *LastFromPrevious = LoopBasicBlocks.back();
    std::vector<BasicBlock *> LoopBasicBlocksCopy = {};
    std::unordered_map<Value *, Value *> Mapping = {};
    std::unordered_map<Value *, Value *> LoadMapping = {};
    std::unordered_map<BasicBlock *, BasicBlock *> BasicBlocksMapping = {};

    BasicBlock *NewBasicBlock = nullptr;
    Instruction *InstrCopy = nullptr;
    
    errs() << "Num of BB: " << LoopBasicBlocks.size() << "\n";

    // Kreiranje buildera sa nekim BB koji je validan
    IRBuilder<> Builder(MoveBefore);
    // Onoliki broj puta koliki bi se regularno petlja izvrsila
    for (int k = 0; k < NumOfTimes; ++k) {
      LoopBasicBlocksCopy.clear();
      
      errs() << "MoveBefore: ";
      MoveBefore->printAsOperand(errs(), false);
      errs() << ", LastFromPrevious: ";
      LastFromPrevious->printAsOperand(errs(), false);
      errs() << " -> ";

      // Prolazimo kroz sve BB koji su ulazili u sastav petlje
      // MoveBefore je inicijalno postavljen na ExitBlock, pa sve sto dodajemo dodajemo pre njega
      for (size_t i = 0; i < LoopBasicBlocks.size(); ++i) {
        NewBasicBlock = BasicBlock::Create(MoveBefore->getContext());
        // insertInto(Function *Parent, BasicBlock *InsertBefore)
        NewBasicBlock->insertInto(MoveBefore->getParent(), MoveBefore);
        LoopBasicBlocksCopy.push_back(NewBasicBlock);
        // Originalni BB mapiramo u BB koji kreiramo
        BasicBlocksMapping[LoopBasicBlocks[i]] = NewBasicBlock;
      }

      for (size_t i = 0; i < LoopBasicBlocks.size(); ++i) {
        Builder.SetInsertPoint(LoopBasicBlocksCopy[i]);

        for (Instruction &Instr : *LoopBasicBlocks[i]) {
          InstrCopy = Instr.clone();
          Builder.Insert(InstrCopy);
          Mapping[&Instr] = InstrCopy;

          if (isa<LoadInst>(InstrCopy) && InstrCopy->getOperand(0) == LoopCounter) {
            // %13 = load i32, ptr %6, align 4
            // %14 = add nsw i32 %13, 1
            Instruction *Add = (Instruction *)BinaryOperator::CreateAdd(InstrCopy, ConstantInt::get(Type::getInt32Ty(InstrCopy->getContext()), k + 1));
            Add->insertAfter(InstrCopy);
            LoadMapping[InstrCopy] = Add;
          }

          for (size_t j = 0; j < Instr.getNumOperands(); ++j) {
            if (Mapping.find(Instr.getOperand(j)) != Mapping.end()) {
              InstrCopy->setOperand(j, Mapping[Instr.getOperand(j)]);
            }
            if (LoadMapping.find(InstrCopy->getOperand(j)) != LoadMapping.end()) {
              InstrCopy->setOperand(j, LoadMapping[InstrCopy->getOperand(j)]);
            }
          }
        }
      }

      for (size_t i = 0; i < LoopBasicBlocks.size(); ++i) {
        for (size_t j = 0; j < LoopBasicBlocks[i]->getTerminator()->getNumSuccessors(); ++j) {
          if (BasicBlocksMapping.find(LoopBasicBlocks[i]->getTerminator()->getSuccessor(j)) != BasicBlocksMapping.end()) {
            LoopBasicBlocksCopy[i]->getTerminator()->setSuccessor(j, BasicBlocksMapping[LoopBasicBlocks[i]->getTerminator()->getSuccessor(j)]);
          }
        }
      }

      // %7 -> %9
      // %9 -> %11 ...
      // LoopBasicBlocksCopy sadrzi samo jedan BB u primeru pa je svejedno da li se uzima .front ili .back

      LastFromPrevious->getTerminator()->setSuccessor(0, LoopBasicBlocksCopy.front());
      LoopBasicBlocksCopy.front()->printAsOperand(errs(), false);
      errs() << "\n";
      LastFromPrevious = LoopBasicBlocksCopy.back();
    }

    // prvi BB koji se nalazi pre exit bloka
    LastFromPrevious->getTerminator()->setSuccessor(0, MoveBefore);
  }

  // U potpunosti uklanjamo petlju i dobijamo ekvivalentan program tako sto naredbe iz njenog tela ponavljamo onoliko puta
  // kolika je bila granica za iteraciju
  void FullyUnrollLoop(Loop *L)
  {
    std::vector<BasicBlock *> LoopBasicBlocksCopy(LoopBasicBlocks.size() - 2);
    // Ne uzimamo prvi i poslednji BB koji ulazi u sastav petlje iz originalnog IR-a, od znacaja nam je samo onaj koji
    // sadrzi logiku koju razmotavamo i koju cemo ponoviti vise puta
    std::copy(LoopBasicBlocks.begin() + 1, LoopBasicBlocks.end() - 1, LoopBasicBlocksCopy.begin());

    L->getLoopPreheader()->getTerminator()->setSuccessor(0, LoopBasicBlocks[1]);
    // za pretposlednji BB postavljamo kao successora izlazni BB
    LoopBasicBlocks[LoopBasicBlocks.size() - 2]->getTerminator()->setSuccessor(0, L->getExitBlock());

    // Prvi i poslednji BB uklanjamo u potpunosti
    LoopBasicBlocks.front()->eraseFromParent();
    LoopBasicBlocks.back()->eraseFromParent();

    DuplicateLoop(LoopBasicBlocksCopy, LoopBound - 1, L->getExitBlock());
  }

  void CopyLoop(Loop *L)
  {
    std::vector<BasicBlock *> LoopBasicBlocksCopy = {};
    std::unordered_map<Value *, Value *> Mapping = {};
    std::unordered_map<Value *, Value *> LoadMapping = {};
    std::unordered_map<BasicBlock *, BasicBlock *> BasicBlocksMapping = {};

    BasicBlock *NewBasicBlock = nullptr;
    Instruction *InstrCopy = nullptr;
    BasicBlock *ExitBlock = L->getExitBlock();
    IRBuilder<> Builder(ExitBlock);

    for (size_t i = 0; i < LoopBasicBlocks.size(); ++i) {
      NewBasicBlock = BasicBlock::Create(LoopBasicBlocks.front()->getContext());
      NewBasicBlock->insertInto(LoopBasicBlocks.front()->getParent(), ExitBlock);
      LoopBasicBlocksCopy.push_back(NewBasicBlock);
      BasicBlocksMapping[LoopBasicBlocks[i]] = NewBasicBlock;
    }

    for (size_t i = 0; i < LoopBasicBlocks.size(); ++i) {
      Builder.SetInsertPoint(LoopBasicBlocksCopy[i]);

      for (Instruction &Instr : *LoopBasicBlocks[i]) {
        InstrCopy = Instr.clone();
        Builder.Insert(InstrCopy);
        Mapping[&Instr] = InstrCopy;

        for (size_t j = 0; j < Instr.getNumOperands(); ++j) {
          if (Mapping.find(Instr.getOperand(j)) != Mapping.end()) {
            InstrCopy->setOperand(j, Mapping[Instr.getOperand(j)]);
          }

          if (LoadMapping.find(InstrCopy->getOperand(j)) != LoadMapping.end()) {
            InstrCopy->setOperand(j, LoadMapping[InstrCopy->getOperand(j)]);
          }
        }
      }
    }

    for (size_t i = 0; i < LoopBasicBlocks.size(); ++i) {
      for (size_t j = 0; j < LoopBasicBlocks[i]->getTerminator()->getNumSuccessors(); ++j) {
        if (BasicBlocksMapping.find(LoopBasicBlocks[i]->getTerminator()->getSuccessor(j)) != BasicBlocksMapping.end())
          LoopBasicBlocksCopy[i]->getTerminator()->setSuccessor(j, BasicBlocksMapping[LoopBasicBlocks[i]->getTerminator()->getSuccessor(j)]);
      }
    }

    LoopBasicBlocks.front()->getTerminator()->setSuccessor(1, LoopBasicBlocksCopy.front());
  }

  void PartiallyUnrollLoop(Loop *L)
  {
    int UnrollingFactor = 4;

    CopyLoop(L);

    std::vector<BasicBlock *> LoopBasicBlocksCopy(LoopBasicBlocks.size() - 2);
    std::copy(LoopBasicBlocks.begin() + 1, LoopBasicBlocks.end() - 1, LoopBasicBlocksCopy.begin()); //[)
    DuplicateLoop(LoopBasicBlocksCopy, UnrollingFactor - 1, LoopBasicBlocks.back());

    for (Instruction &Instr : *LoopBasicBlocks.front()) {
      if (isa<LoadInst>(&Instr) && Instr.getOperand(0) == LoopCounter) {
        Instruction *AddInstr = (Instruction *) BinaryOperator::CreateAdd(&Instr,
                           ConstantInt::get(Type::getInt32Ty(Instr.getContext()), UnrollingFactor - 1));
        AddInstr->insertAfter(&Instr);
        Instr.replaceAllUsesWith(AddInstr);
        AddInstr->setOperand(0, &Instr);
      }
    }

    for (Instruction &Instr : *LoopBasicBlocks.back()) {
      if (isa<AddOperator>(&Instr) && VariablesMap[Instr.getOperand(0)] == LoopCounter) {
        Instr.setOperand(1, ConstantInt::get(Type::getInt32Ty(Instr.getContext()), UnrollingFactor));
      }
    }
  }

  void UnrollLoop(Loop *L)
  {
    if (IsLoopBoundConstant) {
      FullyUnrollLoop(L);
    } else {
      PartiallyUnrollLoop(L);
    }
  }

  bool runOnLoop(Loop *L, LPPassManager &) override {
    LoopBasicBlocks = L->getBlocksVector();
    MapVariables(L);
    FindLoopBoundAndCounter();
    UnrollLoop(L);

    return true;
  }
};

}

char LoopUnrollingPass::ID = 0;
static RegisterPass<LoopUnrollingPass> X("loop-unrolling", "Loop unrolling pass");
