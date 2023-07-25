#include "OurCFG.h"

void OurCFG::AddEdge(BasicBlock* From, BasicBlock* To)
{
    AdjacencyList[From].push_back(To);
}

// Analogno successorima, postoje i predecessori
void OurCFG::CreateCFG(Function &F)
{
    FunctionName = F.getName().str();

    // Krecemo se kroz sve BasicBlock-ove prosledjene funkcije
    for (BasicBlock &BB : F) {
        AdjacencyList[&BB] = {};
        // successors - funkcija koja nam vraca sve BasicBlock-ove u koje mozemo skociti iz trenutnog BasicBlock-a
        // uz koriscenje br instrukcije
        for (BasicBlock* Successor : successors(&BB))
            AddEdge(&BB, Successor);
    }
}

void OurCFG::DumpToFile() 
{
    std::error_code error;
    std::string FileName = FunctionName + ".dot";
    raw_fd_stream File(FileName, error);

    File << "digraph \"CFG for '" << FunctionName << "' function\" {\n"
        "\tlabel=\"CFG for '" << FunctionName << "' function\";\n";
    
    for (auto &kvp : AdjacencyList)
        DumpBasicBlock(kvp.first, File);
    
    File << "}\n";
    File.close();
}

void OurCFG::DumpBasicBlock(BasicBlock* BB, raw_fd_stream &File, bool only)
{
    bool MultipleSuccessors = false;

    File << "\tNode" << BB << " [shape=record, color=\"#b70d28ff\", style=filled, fillcolor=\"#b70d2870\", label=\"{";
    BB->printAsOperand(File, false);

    if (only) {
        File << "\\l ";
        // Vrsimo proveru da li kontrola toka moze da nas dovede iz tekuceg BasicBlock-a u vise njih ili samo u jedan.
        // Da bismo to utvrdili, neophodno je da proverimo da li BasicBlock kao instrukciju sadrzi br instrukciju koja se odnosi na 
        // IF ili SWITCH naredbu. Ukoliko sadrzi, znamo da postoji vise successora i neophodno je adekvatno ih zapisati u .dot fajlu, 
        // a ukoliko ne, zapisujemo instrukciju standardno.

        for (Instruction &Instr : *BB) {
            // Provera da li je u pitanju br instrukcija
            if (BranchInst *BranchInstruction = dyn_cast<BranchInst>(&Instr)) {
                // Provera da li je ova instrukcija IF
                if (BranchInstruction->isConditional()) {
                    MultipleSuccessors = true;

                    // Grananje u .dot fajlu zapisujemo na ovaj nacin
                    File << "|{<s0>T|<s1>F}";
                } else {
                    File << Instr << "\\l ";
                }
            }

            if (SwitchInst *SwitchInstruction = dyn_cast<SwitchInst>(&Instr)) {
                MultipleSuccessors = true;

                // Zapisujemo prvo default slucaj
                File << "|{<s0>def";

                // Dalje prolazimo kroz svaki case i ispisujemo redni broj, a zatim i vrednost
                for (auto Case : SwitchInstruction->cases()) {
                    File << "|<s" << (Case.getCaseIndex() + 1) << ">";

                    // Ispisujemo kao operand jer zelimo da izvucemo samo vrednost, a ne i tip podataka uz nju
                    // U IR-u, switch ima narednu formu
                    // switch i32 %num, label %num_l1 [
                    //      i32 value_1, label %num_l2
                    //      i32 value_2, label %num_l3
                    //      ...   
                    // ]
                    // Napomena: u implementaciji SwitchInst, vrednosti koje su nama neophodne nalaze se na PARNIM indeksima.
                    // Dodatno, prva dva indeksa treba preskociti jer se oni odnose na vrednost po kojoj granamo, kao i 
                    // basic block na koji se skace u podrazumevanom slucaju. Nepadni indeksi se odnose na basic block-ove 
                    // u koje dolazimo prilikom poklapanja.

                    auto* SwitchOperand = SwitchInstruction->getOperand(2 * Case.getCaseIndex() + 2);
                    SwitchOperand->printAsOperand(File, false);
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