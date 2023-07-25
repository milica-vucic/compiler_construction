#ifndef OURCFG_H
#define OURCFG_H

#include "llvm/IR/Instructions.h"

#include <unordered_map>
#include <vector>

using namespace llvm;

// Kreiranje CFG-a
// Lista povezanosti cuva prelaze iz jednog BasicBlock-a u njegove successore

class OurCFG {
private:
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> AdjacencyList;
    std::string FunctionName;

    void DumpBasicBlock(BasicBlock*, raw_fd_stream &File, bool only = true);
public:
    void AddEdge(BasicBlock*, BasicBlock*);
    void CreateCFG(Function&);
    void DumpToFile();
};

#endif // OURCFG_H