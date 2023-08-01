#include "llvm/IR/Function.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "ConstantPropagationInstruction.h"

using namespace llvm;

namespace {

struct ConstantPropagationPass : public FunctionPass
{
  static char ID;
  ConstantPropagationPass() : FunctionPass(ID) {};

  // Vektor promenljivih
  std::vector<Value *> Variables;

  // Vektor instrukcija
  std::vector<ConstantPropagationInstruction *> Instructions;

  void IterateThroughFunction(Function &F)
  {
    ConstantPropagationInstruction *Current, *PreviousCPI;
    Instruction *Previous = nullptr;

    for (BasicBlock &BB : F) {
      for (Instruction &Instr : BB) {
        Current = new ConstantPropagationInstruction(&Instr);
        Instructions.push_back(Current);

        // Trazimo prvu prethodnu instrukciju u basic block-u ako postoji (a da nije debug instrukcija)
        Previous = Instr.getPrevNonDebugInstruction();
        if (!Previous) {
          // Ako takva ne postoji, trazimo poslednju instrukciju iz nekog prethodnog basic bloka, a koja se nalazi u nasem vektoru instrukcija

          for (BasicBlock *Predecessor : predecessors(&BB)) {
            PreviousCPI = *std::find_if(Instructions.begin(), Instructions.end(),
                                        [Predecessor](ConstantPropagationInstruction *CPI)
                                        {
                                          return CPI->GetInstruction() == &Predecessor->back();
                                        });
            Current->AddPredecessor(PreviousCPI);
          }
        } else {
          // Ako postoji, vec se nalazi u vektoru instrukcija pa je dodajemo kao predecessora tekucoj instrukciji
//          PreviousCPI = *std::find_if(Instructions.begin(), Instructions.end(),
//                                      [Previous](ConstantPropagationInstruction *CPI) {
//                                        return CPI->GetInstruction() == Previous;
//                                      });
          Current->AddPredecessor(Instructions[Instructions.size() - 2]);
        }
      }
    }
  }

  void FindVariables(Function &F)
  {
    // Gde god u IR-u imamo Alloca instrukciju, vrsi se alociranje prostora za neku od promenljivih naseg programa
    for (BasicBlock &BB : F) {
      for (Instruction &Instr : BB) {
        if (AllocaInst *AllocaInstruction = dyn_cast<AllocaInst>(&Instr))
          Variables.push_back(AllocaInstruction);
      }
    }
  }

  void SetVariables(Function &F)
  {
    FindVariables(F);

    for (ConstantPropagationInstruction *CPI : Instructions)
      CPI->SetVariables(Variables);
  }

  void SetStatusForStartInstruction()
  {
    // Promenljiva u pocetnoj instrukciji ima iskljucivo jedno stanje, dok sve ostale promenljive imaju oba.
    // Kako ce ta instrukcija biti sigurno izvrsena, promenljiva je dostizna, ali nam njena vrednost ne mora biti poznata.
    for (Value *Variable : Variables)
      Instructions.front()->SetStatusBefore(Variable, Status::Top);
  }

  // ====================================== Optimizacija ======================================
  // Postupak sprovodjenja optimizacije:
  //  1. Proveriti da li su ispunjena neophodna pravila;
  //  2. Ukoliko nisu, vrsiti primenu datih pravila.

  bool CheckRuleOne(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ako je promenljiva dostizna u bar jednom predecessoru, treba da bude dostizna i u tekucoj instrukciji
    for (ConstantPropagationInstruction *Predecessor : CPI->GetPredecessors()) {
      if (Predecessor->GetStatusAfter(Variable) == Status::Top)
        return CPI->GetStatusBefore(Variable) == Status::Top;
    }

    return true;
  }

  void ApplyRuleOne(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    CPI->SetStatusBefore(Variable, Status::Top);
  }

  bool CheckRuleTwo(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ako razliciti predecessori istoj promenljivoj dodeljuju razlicite vrednosti, onda je njeno stanje nepoznato u tekucoj instrukciji
    std::unordered_set<int> Values;

    for (ConstantPropagationInstruction *Predecessor : CPI->GetPredecessors()) {
      if (Predecessor->GetStatusAfter(Variable) == Status::Const)
        Values.insert(Predecessor->GetValueAfter(Variable));
    }

    if (Values.size() > 1)
      return CPI->GetStatusBefore(Variable) == Status::Top;

    return true;
  }

  void ApplyRuleTwo(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    CPI->SetStatusBefore(Variable, Status::Top);
  }

  bool CheckRuleThree(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ako razliciti predecessori dodeljuju istu konstantnu vrednost promenljivoj ili vazi da je status nekog od njih nepoznat, onda ce
    // promenljiva u tekucoj instrukciji imati istu tu vrednost

    std::unordered_set<int> Values;

    for (ConstantPropagationInstruction *Predecessor : CPI->GetPredecessors()) {
      if (Predecessor->GetStatusAfter(Variable) == Status::Const) {
        Values.insert(Predecessor->GetValueAfter(Variable));
      } else if (Predecessor->GetStatusAfter(Variable) == Status::Top) {
        return true;
      }
    }

    if (Values.size() == 0)
      return true;

    if (Values.size() == 1)
      return CPI->GetStatusBefore(Variable) == Status::Const && CPI->GetValueBefore(Variable) == *Values.begin();

    return true;
  }

  void ApplyRuleThree(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    int Value;
    for (ConstantPropagationInstruction *Predecessor : CPI->GetPredecessors()) {
      if (Predecessor->GetStatusAfter(Variable) == Status::Const) {
        Value = Predecessor->GetValueAfter(Variable);
        break;
      }
    }

    CPI->SetStatusBefore(Variable, Status::Const);
    CPI->SetValueBefore(Variable, Value);
  }

  bool CheckRuleFour(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ako promenljiva nije dostizna ni u jednom od predecessora, nece biti dostizna ni u tekucoj instrukciji

    unsigned Counter = 0u;
    for (ConstantPropagationInstruction *Predecessor : CPI->GetPredecessors()) {
      if (Predecessor->GetStatusAfter(Variable) == Status::Bottom)
        Counter++;
    }

    if (Counter > 0 && Counter == CPI->GetPredecessors().size())
      return CPI->GetStatusBefore(Variable) == Status::Bottom;

    return true;
  }

  void ApplyRuleFour(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    CPI->SetStatusBefore(Variable, Status::Bottom);
  }

  bool CheckRuleFive(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ako je promenljiva bila nedostizna pre izvrsavanja instrukcije onda ce biti nedostizna i nakon njenog izvrsavanja
    if (CPI->GetStatusBefore(Variable) == Status::Bottom)
      return CPI->GetStatusAfter(Variable) == Status::Bottom;

    return true;
  }

  void ApplyRuleFive(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    CPI->SetStatusAfter(Variable, Status::Bottom);
  }

  bool CheckRuleSix(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ukoliko je naredba naredba dodele, onda ce promenljiva imati istu konstantnu vrednost nakon izvrsavanja naredbe kao i pre izvrsavanja naredbe
    if (StoreInst *StoreInstruction = dyn_cast<StoreInst>(CPI->GetInstruction())) {
      // store i32 42
      if (StoreInstruction->getOperand(1) == Variable) {
        if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(StoreInstruction->getOperand(0))) {
          return CPI->GetStatusAfter(Variable) == Status::Const &&
                 CPI->GetValueAfter(Variable) == ConstInt->getSExtValue();
        }
      }
    }

    return true;
  }

  void ApplyRuleSix(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    ConstantInt *ConstInt = dyn_cast<ConstantInt>(CPI->GetInstruction()->getOperand(0));
//    assert(ConstInt != nullptr);

    CPI->SetStatusAfter(Variable, Status::Const);
    CPI->SetValueAfter(Variable, ConstInt->getSExtValue());
  }

  bool CheckRuleSeven(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ako se promenljivoj x dodeljuje vrednost koja nije konstantna, njena vrednost nakon izvrsavanja dodele nece biti poznata
    if (StoreInst *StoreInstruction = dyn_cast<StoreInst>(CPI->GetInstruction())) {
      if (StoreInstruction->getOperand(1) == Variable) {
        ConstantInt *ConstInt = dyn_cast<ConstantInt>(StoreInstruction->getOperand(0));
        if (ConstInt == nullptr)
          return CPI->GetStatusAfter(Variable) == Status::Top;
      }
    }

    return true;
  }

  void ApplyRuleSeven(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    CPI->SetStatusAfter(Variable, Status::Top);
  }

  bool CheckRuleEight(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    // Ako je naredba naredba dodele i ne menja vrednost promenljive x, onda ce x imati istu vrednost i pre i posle izvrsavanja dodele
    if (StoreInst *StoreInstruction = dyn_cast<StoreInst>(CPI->GetInstruction())) {
      if (StoreInstruction->getOperand(1) == Variable)
        return true;
    }

    return CPI->GetStatusBefore(Variable) == CPI->GetStatusAfter(Variable);
  }

  void ApplyRuleEight(Value *Variable, ConstantPropagationInstruction *CPI)
  {
    CPI->SetStatusAfter(Variable, CPI->GetStatusBefore(Variable));
    CPI->SetValueAfter(Variable, CPI->GetValueBefore(Variable));
  }

  void RunAlgorithm(Value *Variable)
  {
    bool RuleApplied;

    while (true) {
      RuleApplied = false;

      for (ConstantPropagationInstruction *CPI : Instructions) {
        if (!CheckRuleOne(Variable, CPI)) {
          ApplyRuleOne(Variable, CPI);
          RuleApplied = true;
          break;
        }
        if (!CheckRuleTwo(Variable, CPI)) {
          ApplyRuleTwo(Variable, CPI);
          RuleApplied = true;
          break;
        }
        if (!CheckRuleThree(Variable, CPI)) {
          ApplyRuleThree(Variable, CPI);
          RuleApplied = true;
          break;
        }
        if (!CheckRuleFour(Variable, CPI)) {
          ApplyRuleFour(Variable, CPI);
          RuleApplied = true;
          break;
        }
        if (!CheckRuleFive(Variable, CPI)) {
          ApplyRuleFive(Variable, CPI);
          RuleApplied = true;
          break;
        }
        if (!CheckRuleSix(Variable, CPI)) {
          ApplyRuleSix(Variable, CPI);
          RuleApplied = true;
          break;
        }
        if (!CheckRuleSeven(Variable, CPI)) {
          ApplyRuleSeven(Variable, CPI);
          RuleApplied = true;
          break;
        }
        if (!CheckRuleEight(Variable, CPI)) {
          ApplyRuleEight(Variable, CPI);
          RuleApplied = true;
          break;
        }
      }

      if (!RuleApplied)
        break;
    }
  }

  void ChangeIR()
  {
    std::unordered_map<Value *, Value *> VariablesMap;

    for (ConstantPropagationInstruction *CPI : Instructions) {
      if (LoadInst *LoadInstruction = dyn_cast<LoadInst>(CPI->GetInstruction())) {
        VariablesMap[LoadInstruction] = LoadInstruction->getOperand(0);
      }
    }

    Value *NewVariable;
    ConstantInt *ConstInt;
    for (ConstantPropagationInstruction *CPI : Instructions) {
      if (StoreInst *StoreInstruction = dyn_cast<StoreInst>(CPI->GetInstruction())) {
        NewVariable = VariablesMap[StoreInstruction->getOperand(0)];

        if (CPI->GetStatusAfter(NewVariable) == Status::Const) {
          // kreiramo novu celobrojnu vrednost ukoliko znamo da promenljiva ima konstantnu vrednost nakon izvrsavanja naredbe
          ConstInt = ConstantInt::get(
                Type::getInt32Ty(CPI->GetInstruction()->getContext()),
                CPI->GetValueAfter(NewVariable),
                true
              );

          NewVariable->replaceAllUsesWith(ConstInt);
        }
      } else if (BinaryOperator *BinaryOp = dyn_cast<BinaryOperator>(CPI->GetInstruction())) {
        Value *Lhs = VariablesMap[BinaryOp->getOperand(0)];
        Value *Rhs = VariablesMap[BinaryOp->getOperand(1)];

        IRBuilder Builder(CPI->GetInstruction());

        Value *NewOperation = nullptr;

        if (CPI->GetStatusAfter(Lhs) == Status::Const) {
          Lhs = ConstantInt::get(
                Type::getInt32Ty(CPI->GetInstruction()->getContext()),
                CPI->GetValueAfter(Lhs),
                true
              );
        } else {
          Lhs = BinaryOp->getOperand(0);
        }

        if (CPI->GetStatusAfter(Rhs) == Status::Const) {
          Rhs = ConstantInt::get(
                Type::getInt32Ty(CPI->GetInstruction()->getContext()),
                CPI->GetValueAfter(Rhs),
                true
              );
        } else {
          Rhs = BinaryOp->getOperand(1);
        }

        if (AddOperator *Add = dyn_cast<AddOperator>(BinaryOp)) {
          (void) Add;
          NewOperation = Builder.CreateAdd(Lhs, Rhs);
        } else if (SubOperator *Sub = dyn_cast<SubOperator>(BinaryOp)) {
          (void) Sub;
          NewOperation = Builder.CreateSub(Lhs, Rhs);
        } else if (MulOperator *Mul = dyn_cast<MulOperator>(BinaryOp)) {
          (void) Mul;
          NewOperation = Builder.CreateMul(Lhs, Rhs);
        } else if (SDivOperator *Div = dyn_cast<SDivOperator>(BinaryOp)) {
          (void) Div;
          NewOperation = Builder.CreateSDiv(Lhs, Rhs);
        }

        if (NewOperation) {
          BinaryOp->replaceAllUsesWith(NewOperation);
          BinaryOp->eraseFromParent();
        }
      } else if (ICmpInst *ICMP = dyn_cast<ICmpInst>(CPI->GetInstruction())) {
        Value *Lhs = VariablesMap[ICMP->getOperand(0)];
        Value *Rhs = VariablesMap[ICMP->getOperand(1)];

        if (CPI->GetStatusAfter(Lhs) == Status::Const) {
          Lhs = ConstantInt::get(
              Type::getInt32Ty(CPI->GetInstruction()->getContext()),
              CPI->GetValueAfter(Lhs), true);
        }
        else {
          Lhs = ICMP->getOperand(0);
        }

        if (CPI->GetStatusAfter(Rhs) == Status::Const) {
          Rhs = ConstantInt::get(
              Type::getInt32Ty(CPI->GetInstruction()->getContext()),
              CPI->GetValueAfter(Rhs), true);
        }
        else {
          Rhs = ICMP->getOperand(1);
        }

        IRBuilder Builder(CPI->GetInstruction());
        Value *NewCmp = Builder.CreateICmp(ICMP->getSignedPredicate(), Lhs, Rhs);
        ICMP->replaceAllUsesWith(NewCmp);
        ICMP->eraseFromParent();
      }
    }
  }

  bool runOnFunction(Function &F) override {
    IterateThroughFunction(F);
    SetVariables(F);
    SetStatusForStartInstruction();

    for (Value *Variable : Variables)
      RunAlgorithm(Variable);

    ChangeIR();
    return true;
  }
};

}

char ConstantPropagationPass::ID = 0;
static RegisterPass<ConstantPropagationPass> X("constant-propagation", "Constant propagation pass");
