#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "DominatorTree.h"

using namespace llvm;

namespace {
// Hello - The first implementation, without getAnalysisUsage.
struct OurDominatorTreePass : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  OurDominatorTreePass() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    DominatorTree* DomTree = new DominatorTree(F);

    DomTree->FindImmediateDominators();
    DomTree->DumpTreeToFile();

    delete DomTree;
    return false;
  }
};
}

char OurDominatorTreePass::ID = 0;
static RegisterPass<OurDominatorTreePass> X("print-our-dominator-tree",
                                            "Our dominator tree pass");