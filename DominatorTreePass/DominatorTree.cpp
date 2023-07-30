#include "DominatorTree.h"

DominatorTree::DominatorTree(Function &F)
{
  FunctionName = F.getName().str();
  StartBlock = &F.front();
  Time = 1;

  for (BasicBlock &BB : F) {
    AdjacencyList[&BB] = {};
    for (BasicBlock* Successor : successors(&BB))
      AddEdge(&BB, Successor);
  }
}

void DominatorTree::AddEdge(BasicBlock *From, BasicBlock *To)
{
  AdjacencyList[From].push_back(To);
  ReversedAdjacencyList[To].push_back(From);
}

void DominatorTree::DFS(BasicBlock *BB, int &Time)
{
  Visited.insert(BB);
  VisitedOrder.push_back(BB);
  InNumeration[BB] = Time++;

  for (BasicBlock* CurrentBlock : AdjacencyList[BB]) {
    if (Visited.find(CurrentBlock) == Visited.end()) {
      Parents[CurrentBlock] = BB;
      DFS(CurrentBlock, Time);
    }
  }
}

void DominatorTree::FindSemiDominatorCandidates(BasicBlock *StartBlock, BasicBlock *CurrentBlock, std::vector<BasicBlock*>& Candidates)
{
  Visited.insert(CurrentBlock);

  if (InNumeration[CurrentBlock] < InNumeration[StartBlock]) {
    Candidates.push_back(CurrentBlock);
    return;
  }

  for (BasicBlock* BB : ReversedAdjacencyList[CurrentBlock]) {
    if (Visited.find(BB) == Visited.end())
      FindSemiDominatorCandidates(StartBlock, BB, Candidates);
  }
}

BasicBlock* DominatorTree::FindSemiDominator(BasicBlock *CurrentBlock)
{
  Visited.clear();
  std::vector<BasicBlock*> Candidates;

  FindSemiDominatorCandidates(CurrentBlock, CurrentBlock, Candidates);

  int MinNumeration = std::numeric_limits<int>::max();
  BasicBlock* SemiDominator = nullptr;

  for (BasicBlock* Candidate : Candidates) {
    if (InNumeration[Candidate] < MinNumeration) {
      MinNumeration = InNumeration[Candidate];
      SemiDominator = Candidate;
    }
  }

  return SemiDominator;
}

void DominatorTree::FindSemiDominators()
{
  std::reverse(VisitedOrder.begin(), VisitedOrder.end());

  for (BasicBlock* CurrentBlock : VisitedOrder)
    SDom[CurrentBlock] = FindSemiDominator(CurrentBlock);
}

void DominatorTree::FindImmediateDominators()
{
  DFS(StartBlock, Time);
  FindSemiDominators();

  BasicBlock* Parent = nullptr;
  BasicBlock* Ancestor = nullptr;
  int MinNumeration = std::numeric_limits<int>::max();

  for (BasicBlock* CurrentBlock : VisitedOrder) {
    Parent = Parents[CurrentBlock];
    MinNumeration = InNumeration[SDom[CurrentBlock]];

    while (Parent != SDom[CurrentBlock]) {
      if (InNumeration[SDom[Parent]] < MinNumeration) {
        MinNumeration = InNumeration[SDom[Parent]];
        Ancestor = Parent;
      }

      Parent = Parents[Parent];
    }

    Ancestors[CurrentBlock] = Ancestor;
  }

  std::reverse(VisitedOrder.begin(), VisitedOrder.end());

  for (BasicBlock* CurrentBlock : VisitedOrder) {
    if (Ancestors[CurrentBlock] == nullptr) {
      IDom[CurrentBlock] = SDom[CurrentBlock];
    } else {
      IDom[CurrentBlock] = IDom[Ancestors[CurrentBlock]];
    }
  }
}

void DominatorTree::DumpTreeToFile()
{
  std::string FileName = "ourdom." + FunctionName + ".dot";
  std::error_code Error;

  raw_fd_ostream File(FileName, Error);

  File << "digraph \"Dominator tree for '" << FunctionName << "' function\" {\n";
  File << "\tlabel=\"Dominator tree for '" << FunctionName << "' function\";\n\n";

  for (BasicBlock* Current : VisitedOrder) {
    File << "\tNode" << Current << " [shape=record, color=\"#072757\", style=filled, fillcolor=\"#2462bf\"label=\"{";
    Current->printAsOperand(File, false);
    File << "}\"];\n";

    File << "\tNode" << IDom[Current] << " -> Node" << Current << ";\n";
  }

  File << "}\n";
  File.close();
}