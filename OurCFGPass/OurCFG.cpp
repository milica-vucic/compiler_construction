//
// Created by milica on 26.7.23..
//

#include "OurCFG.h"

void OurCFG::AddEdge(BasicBlock *From, BasicBlock *To)
{
  AdjacencyList[From].push_back(To);
}

void OurCFG::CreateCFG(Function &F)
{
  FunctionName = F.getName().str();

  for (BasicBlock &BB : F) {
    for (auto* Successor : successors(&BB))
      AddEdge(&BB, Successor);
  }
}

void OurCFG::DumpToFile()
{
  std::error_code error;
  raw_fd_stream File(FunctionName + ".dot", error);

  File << "digraph \"CFG for '" << FunctionName << "' function\" {\n"
          "\tlabel=\"CFG for '" << FunctionName << "' function\";\n";

  for (auto &kvp : AdjacencyList)
    DumpBasicBlock(kvp.first, File);

  File << "}\n";
  File.close();
}

void OurCFG::DumpBasicBlock(BasicBlock *BB, raw_fd_stream &File, bool only)
{
  bool MultipleSuccessors = false;

  File << "\tNode" << BB << " [shape=record, color=\"#73167d\", style=filled, fillcolor=\"#811b8c\", label=\"{";

  BB->printAsOperand(File, false);
  if (only) {
    File << "\\l ";

    for (Instruction &Instr : *BB) {
      if (auto* BranchInstruction = dyn_cast<BranchInst>(&Instr)) {
        if (BranchInstruction->isConditional()) {
          MultipleSuccessors = true;
          File << "|{<s0>T|<s1>F}";
        } else {
          File << Instr << "\\l ";
        }
      }

      if (auto* SwitchInstruction = dyn_cast<SwitchInst>(&Instr)) {
        MultipleSuccessors = true;

        File << "|{<s0>def";

        for (auto Case : SwitchInstruction->cases()) {
          unsigned CaseIndex = Case.getCaseIndex();
          File << "|<s" << (CaseIndex + 1) << ">";

          auto* CaseOperand = SwitchInstruction->getOperand(2 * CaseIndex + 2);
          CaseOperand->printAsOperand(File, false);
        }
        File << "}";
      } else {
        File << Instr << "\\l ";
      }
    }
  }

  File << "}\"];\n";
  unsigned Index = 0;

  for (auto Successor : AdjacencyList[BB]) {
    if (MultipleSuccessors) {
      File << "\tNode" << BB << ":s" << Index << " -> Node" << Successor << ";\n";
      Index++;
    } else {
      File << "\tNode" << BB << " -> Node" << Successor << ";\n";
    }
  }
}
