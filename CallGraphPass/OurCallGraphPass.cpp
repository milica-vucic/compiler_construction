#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "ourcallgraph.h"

using namespace llvm;

namespace {
  struct OurCallGraphPass : public ModulePass {
    static char ID;
    OurCallGraphPass() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
      OurCallGraph* CallGraph = new OurCallGraph();

      CallGraph->CreateCallGraph(M);
      CallGraph->dumpGraphToFile();

      return false;
    }
  };
}

char OurCallGraphPass::ID = 0;
static RegisterPass<OurCallGraphPass> X
("print-our-call-graph", "Our pass to print call graph of a module.");
