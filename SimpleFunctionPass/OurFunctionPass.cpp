#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#include <unordered_map>

// Prolazenje kroz funkcije i ispisivanje koliko puta se koja instrukcija javlja
// OpCode - kod masinske instrukcije na odgovarajucoj arhitekturi

namespace {
  struct OurFunctionPass : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    OurFunctionPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      std::unordered_map<std::string, int> OpCodesMap;

      for (BasicBlock &Block : F) {
        for (Instruction &Instr : Block) {
          OpCodesMap[std::string(Instr.getOpcodeName())]++;
        }
      }

      errs() << F.getName() << "\n===========================\n";
      for (const auto &pair : OpCodesMap) {
        errs() << pair.first << " " << pair.second << "\n";
      }
      return false;
    }
  };
}

char OurFunctionPass::ID = 0;
static RegisterPass<OurFunctionPass> X("ourfunctionpass", "Our function pass", false, false);
