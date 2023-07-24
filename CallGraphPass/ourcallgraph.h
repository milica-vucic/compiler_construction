#ifndef OURCALLGRAPH_H
#define OURCALLGRAPH_H

#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"

#include <unordered_map>
#include <unordered_set>
#include <fstream>

using namespace llvm;

class OurCallGraph
{
private:
    std::unordered_map<Function* , std::unordered_set<Function*>> AdjacencyList;
    std::string ModuleName;
public:
    void CreateCallGraph(Module &M);
    void DFS(Function* F);
    void dumpGraphToFile();
};

#endif // OURCALLGRAPH_H
