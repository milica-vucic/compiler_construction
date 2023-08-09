//
// Created by milica on 1.8.23..
//

#ifndef LLVM_PROJECT_CFG_H
#define LLVM_PROJECT_CFG_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace llvm;

class CFG {
private:
  std::unordered_map<BasicBlock *, std::vector<BasicBlock *>> AdjacencyList;
  std::unordered_set<BasicBlock *> Visited;
  BasicBlock *StartBlock;

  void CreateCFG(Function &F);
  void DFS(BasicBlock *Current);
  void AddEdge(BasicBlock *, BasicBlock *);
public:
  CFG(Function &F);
  void TraverseGraph();
  bool IsReachable(BasicBlock *);
};

#endif // LLVM_PROJECT_CFG_H
