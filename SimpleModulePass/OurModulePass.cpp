#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_map>

using namespace llvm;

// Pass koji prolazi kroz citav modul i belezi ukupan broj pojavljivanja svake od instrukcija
// koje se u tom modulu javljaju

// OurFunctionPass radio je isto, samo sto je nivo pretrage bila funkcija

namespace {
    struct OurModulePass : public ModulePass {
        static char ID;
        OurModulePass() : ModulePass(ID) {};

        bool runOnModule(Module &M) override {
            std::unordered_map<std::string, int> OpCodesMap;

            for (auto &F : M) {
                for (auto &BB : F) {
                    for (auto &Instr : BB) {
                        OpCodesMap[Instr.getOpcodeName()]++;
                    }
                }
            }

            for (const auto &kvp : OpCodesMap)
                errs() << kvp.first << " occurs " << kvp.second << " times.\n";

            return false;
        }
    };
}

char OurModulePass::ID = 0;
static RegisterPass<OurModulePass> X("ourmodulepass", "Our module pass, doing nothing smart.");
