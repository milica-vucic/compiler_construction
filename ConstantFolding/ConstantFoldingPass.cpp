#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"

#include <vector>

using namespace llvm;

namespace {

struct ConstantFoldingPass : public FunctionPass
{
  static char ID;
  ConstantFoldingPass() : FunctionPass(ID) {};

  std::vector<Instruction *> InstructionsToRemove = {};

  bool IsBinaryOperator(Instruction *Instr)
  {
    return isa<BinaryOperator>(Instr);
  }

  bool IsCompare(Instruction *Instr)
  {
    return isa<ICmpInst>(Instr);
  }

  bool IsBranch(Instruction *Instr)
  {
    return isa<BranchInst>(Instr);
  }

  bool IsConstantInt(Value *Instr)
  {
    return isa<ConstantInt>(Instr);
  }

  int GetConstantInt(Value *Operand)
  {
    return dyn_cast<ConstantInt>(Operand)->getSExtValue();
  }

  void HandleBinaryOperator(Instruction *Instr)
  {
    // Provera da li su oba operanda celobrojne vrednosti
    if (IsConstantInt(Instr->getOperand(0)) && IsConstantInt(Instr->getOperand(1))) {
      int LeftValue = GetConstantInt(Instr->getOperand(0));
      int RightValue = GetConstantInt(Instr->getOperand(1));

      Value *Result = nullptr;

      if (isa<AddOperator>(Instr)) {
        Result = ConstantInt::get(Type::getInt32Ty(Instr->getContext()), LeftValue + RightValue);
      } else if (isa<SubOperator>(Instr)) {
        Result = ConstantInt::get(Type::getInt32Ty(Instr->getContext()), LeftValue - RightValue);
      } else if (isa<MulOperator>(Instr)) {
        Result = ConstantInt::get(Type::getInt32Ty(Instr->getContext()), LeftValue * RightValue);
      } else if (isa<SDivOperator>(Instr)) {
        Result = ConstantInt::get(Type::getInt32Ty(Instr->getContext()), LeftValue / RightValue);
      }

      if (Result)
        Instr->replaceAllUsesWith(Result);
    }
  }

  void HandleCompare(Instruction *Instr)
  {
    if (IsConstantInt(Instr->getOperand(0)) && IsConstantInt(Instr->getOperand(1))) {
      int LeftValue = GetConstantInt(Instr->getOperand(0));
      int RightValue = GetConstantInt(Instr->getOperand(1));

      ICmpInst *Compare = dyn_cast<ICmpInst>(Instr);
      ConstantInt *CompareValue = nullptr;
      auto Predicate = Compare->getPredicate();

      if (Predicate == ICmpInst::ICMP_EQ) {
        CompareValue = ConstantInt::get(Type::getInt1Ty(Instr->getContext()), LeftValue == RightValue);
      } else if (Predicate == ICmpInst::ICMP_NE) {
        CompareValue = ConstantInt::get(Type::getInt1Ty(Instr->getContext()), LeftValue != RightValue);
      } else if (Predicate == ICmpInst::ICMP_SGT) {
        CompareValue = ConstantInt::get(Type::getInt1Ty(Instr->getContext()), LeftValue > RightValue);
      } else if (Predicate == ICmpInst::ICMP_SGE) {
        CompareValue = ConstantInt::get(Type::getInt1Ty(Instr->getContext()), LeftValue >= RightValue);
      } else if (Predicate == ICmpInst::ICMP_SLT) {
        CompareValue = ConstantInt::get(Type::getInt1Ty(Instr->getContext()), LeftValue < RightValue);
      } else if (Predicate == ICmpInst::ICMP_SLE) {
        CompareValue = ConstantInt::get(Type::getInt1Ty(Instr->getContext()), LeftValue <= RightValue);
      }

      if (CompareValue)
        Instr->replaceAllUsesWith(CompareValue);
    }
  }

  void HandleBranch(Instruction *Instr)
  {
    BranchInst *BranchInstruction = dyn_cast<BranchInst>(Instr);

    if (BranchInstruction->isConditional()) {
      if (IsConstantInt(BranchInstruction->getCondition())) {
        ConstantInt *ConditionValue = dyn_cast<ConstantInt>(BranchInstruction->getCondition());

        if (ConditionValue->getZExtValue() == 1) {
          BranchInst::Create(BranchInstruction->getSuccessor(0), Instr->getParent());
        } else {
          BranchInst::Create(BranchInstruction->getSuccessor(1), Instr->getParent());
        }

        InstructionsToRemove.push_back(Instr);
      }
    }
  }

  void IterateThroughFunction(Function &F)
  {
    for (BasicBlock &BB : F) {
      for (Instruction &Instr : BB) {
        if (IsBinaryOperator(&Instr)) {
          HandleBinaryOperator(&Instr);
        } else if (IsCompare(&Instr)) {
          HandleCompare(&Instr);
        } else if (IsBranch(&Instr)) {
          HandleBranch(&Instr);
        }
      }
    }

    for (Instruction *Instr : InstructionsToRemove)
      Instr->removeFromParent();
  }

  bool runOnFunction(Function &F) override {
    IterateThroughFunction(F);
    return true;
  }
};

}

char ConstantFoldingPass::ID = 0;
static RegisterPass<ConstantFoldingPass> X("constant-folding", "Constant folding pass");