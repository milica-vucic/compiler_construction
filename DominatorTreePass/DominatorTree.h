//
// Created by milica on 28.7.23..
//

#ifndef LLVM_PROJECT_DOMINATORTREE_H
#define LLVM_PROJECT_DOMINATORTREE_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"

#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <vector>

using namespace llvm;

class DominatorTree {
private:
  // U ovom slucaju cvorovi u grafu su nam BasicBlock-ovi
  int Time;

  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> AdjacencyList;
  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> ReversedAdjacencyList;
  std::unordered_map<BasicBlock*, BasicBlock*> Parents;
  std::unordered_map<BasicBlock*, llvm::BasicBlock*> Ancestors;

  std::vector<BasicBlock*> VisitedOrder;
  std::unordered_set<BasicBlock*> Visited;
  std::unordered_map<BasicBlock*, int> InNumeration;

  std::unordered_map<BasicBlock*, BasicBlock*> SDom;
  std::unordered_map<BasicBlock*, BasicBlock*> IDom;
public:
  std::string FunctionName;
  BasicBlock* StartBlock;

  DominatorTree(Function&);

  void AddEdge(BasicBlock*, BasicBlock*);
  void DFS(BasicBlock*, int&);
  void FindSemiDominators();
  BasicBlock* FindSemiDominator(BasicBlock*);
  void FindSemiDominatorCandidates(BasicBlock*, BasicBlock*, std::vector<BasicBlock*>&);
  void FindImmediateDominators();
  void DumpTreeToFile();
};

#endif // LLVM_PROJECT_DOMINATORTREE_H
