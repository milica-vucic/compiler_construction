//
// Created by milica on 26.7.23..
//

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

#include "OurCFG.h"

using namespace llvm;

namespace {

struct OurCFGPass : public FunctionPass {
  static char ID;
  OurCFGPass() : FunctionPass(ID) {};

  bool runOnFunction(Function &F) override {
    OurCFG* CFG = new OurCFG();
    CFG->CreateCFG(F);
    CFG->DumpToFile();

    delete CFG;
    return false;
  }
};

}

char OurCFGPass::ID = 0;
static RegisterPass<OurCFGPass> X("cfg-pass", "Simple CFG pass");