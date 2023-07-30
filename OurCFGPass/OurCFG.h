//
// Created by milica on 26.7.23..
//

#ifndef LLVM_PROJECT_OURCFG_H
#define LLVM_PROJECT_OURCFG_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_map>
#include <vector>

using namespace llvm;

class OurCFG {
private:
  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> AdjacencyList;
  std::string FunctionName;

  void DumpBasicBlock(BasicBlock*, raw_fd_stream &File, bool only = true);
public:
  void AddEdge(BasicBlock*, BasicBlock*);
  void CreateCFG(Function &F);
  void DumpToFile();
};

#endif // LLVM_PROJECT_OURCFG_H
