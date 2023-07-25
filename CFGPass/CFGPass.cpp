#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "OurCFG.h"

using namespace llvm;

namespace {

struct OurCFGPass : public FunctionPass {
    static char ID; // identifikacija za pass
    OurCFGPass() : FunctionPass(ID) {};

    bool runOnFunction(Function &F) override {
        OurCFG *CFG = new OurCFG();

        CFG->CreateCFG(F);
        CFG->DumpToFile();

        delete CFG;
        return false;       // vracamo false zato sto je Analysis pass, ne menja se IR
    }
};
}

char OurCFGPass::ID = 0;
static RegisterPass<OurCFGPass> X("print-our-cfg", "Simple pass that prints CFG.");