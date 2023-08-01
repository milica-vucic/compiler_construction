//
// Created by milica on 8/30/23.
//

#ifndef LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H
#define LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace llvm;

enum class Status
{
  Top,
  Bottom,
  Const
};

class ConstantPropagationInstruction
{
private:
  Instruction *Instr;

  std::unordered_map<Value *, std::pair<Status, int>> StatusBefore;
  std::unordered_map<Value *, std::pair<Status, int>> StatusAfter;
  std::vector<ConstantPropagationInstruction *> Predecessors;
public:
  ConstantPropagationInstruction(Instruction *);

  Status GetStatusBefore(Value *);
  Status GetStatusAfter(Value *);
  int GetValueBefore(Value *);
  int GetValueAfter(Value *);
  Instruction *GetInstruction();

  void SetStatusBefore(Value *, Status);
  void SetStatusAfter(Value *, Status);
  void SetValueBefore(Value *, int);
  void SetValueAfter(Value *, int);
  void SetVariables(std::vector<Value *> &);

  std::vector<ConstantPropagationInstruction *> GetPredecessors() const;
  void AddPredecessor(ConstantPropagationInstruction *);
};

#endif // LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H
