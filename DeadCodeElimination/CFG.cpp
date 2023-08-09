//
// Created by milica on 1.8.23..
//

#include "CFG.h"

CFG::CFG(Function &F)
{
  CreateCFG(F);
}

void CFG::AddEdge(BasicBlock *From, BasicBlock *To)
{
  AdjacencyList[From].push_back(To);
}

void CFG::CreateCFG(Function &F)
{
  StartBlock = &F.front();

  for (BasicBlock &BB : F) {
    AdjacencyList[&BB] = {};
    for (BasicBlock *Successor : successors(&BB))
      AddEdge(&BB, Successor);
  }
}

void CFG::TraverseGraph()
{
  DFS(StartBlock);
}

void CFG::DFS(BasicBlock *Current)
{
  Visited.insert(Current);

  for (BasicBlock *Successor : AdjacencyList[Current])
    if (Visited.find(Successor) == Visited.end())
      DFS(Successor);
}

bool CFG::IsReachable(BasicBlock *BB)
{
  return Visited.find(BB) != Visited.end();
}