//
// Created by milica on 8/30/23.
//

#include "ConstantPropagationInstruction.h"

ConstantPropagationInstruction::ConstantPropagationInstruction(Instruction *Inst)
{
  Instr = Inst;
}

Status ConstantPropagationInstruction::GetStatusBefore(Value *Variable)
{
  return StatusBefore[Variable].first;
}

Status ConstantPropagationInstruction::GetStatusAfter(Value *Variable)
{
  return StatusAfter[Variable].first;
}

int ConstantPropagationInstruction::GetValueBefore(Value *Variable)
{
  return StatusBefore[Variable].second;
}

int ConstantPropagationInstruction::GetValueAfter(Value *Variable)
{
  return StatusAfter[Variable].second;
}

Instruction *ConstantPropagationInstruction::GetInstruction()
{
  return Instr;
}

std::vector<ConstantPropagationInstruction *> ConstantPropagationInstruction::GetPredecessors() const
{
  return Predecessors;
}

void ConstantPropagationInstruction::SetStatusBefore(Value *Variable, Status S)
{
  StatusBefore[Variable].first = S;
}

void ConstantPropagationInstruction::SetStatusAfter(Value *Variable, Status S)
{
  StatusAfter[Variable].first = S;
}

void ConstantPropagationInstruction::SetValueBefore(Value *Variable, int Value)
{
  StatusBefore[Variable].second = Value;
}

void ConstantPropagationInstruction::SetValueAfter(Value *Variable, int Value)
{
  StatusAfter[Variable].second = Value;
}

void ConstantPropagationInstruction::AddPredecessor(ConstantPropagationInstruction *Predecessor)
{
  Predecessors.push_back(Predecessor);
}

void ConstantPropagationInstruction::SetVariables(std::vector<Value *> &Variables)
{
  for (Value *Variable : Variables) {
    SetStatusBefore(Variable, Status::Bottom);
    SetStatusAfter(Variable, Status::Bottom);
  }
}
