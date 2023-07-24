#include "ourcallgraph.h"

void OurCallGraph::CreateCallGraph(Module &M)
{
    ModuleName = M.getName().str();
    Function* Main = M.getFunction("main");

    if (Main == nullptr) {
        errs() << "Main function doesn't exist!\n";
        exit(0);
    }

    DFS(Main);
}

void OurCallGraph::DFS(Function* F)
{
    // Krecemo se prvo kroz svaki basic block u okviru funkcije, a zatim za svaku instrukciju proveravamo
    // da li je u pitanju CALL instrukcija. Ako jeste, to znaci da dolazi do poziva neke druge funkcije,
    // pa je samim tim neophodno da upamtimo to u listi povezanosti i nastavimo pretragu dalje u dubinu.

    for (auto &BB : *F) {
        for (auto &Instr : BB) {
            
            // Koristiti llvm-ov dyn_cast, a ne iz stl-a
            if (auto CallInstruction = dyn_cast<CallInst>(&Instr)) {
                // Dohvatamo funkciju koja se poziva
                // getCalledFunction vraca nullptr ako ne postoji poziv nijedne funkcije

                Function* Callee = CallInstruction->getCalledFunction();
                AdjacencyList[F].insert(Callee);

                // Ako pozvana funkcija ne postoji vec u listi povezanosti, znaci da treba nju dalje
                // da obradjujemo. Inicijalizujemo funkcije koje ona poziva na prazan skup, a zatim
                // nastavljamo DFS.

                if (AdjacencyList.find(Callee) == AdjacencyList.end()) {
                    AdjacencyList[Callee] = {};
                    DFS(Callee);
                }
            }
        }
    }
}

void OurCallGraph::dumpGraphToFile()
{
    std::ofstream File;
    File.open(ModuleName + ".dot");

    File << "digraph  \"Call graph: " + ModuleName << "\" {\n";
    File << "\tlabel=\"Call graph: " << ModuleName << "\";\n";

    // Svaki cvor zapisan je u narednom formatu:
    // Node0x559967de8350 [shape=record, label="{print_hello_world}"];

    // Prolazimo kroz elemente liste povezanosti
    for (auto &kvp : AdjacencyList) {
        File << "\tNode" << kvp.first << " [shape=record, label=\"{" <<
                kvp.first->getName().str() << "}\"];\n";

        // Kada postoji grana izmedju dva cvora u grafu, reprezentuje se na sledeci nacin:
        // Node0x559967de8350 -> Node0x559967de8351, gde su heksadekadne cifre adrese tih cvorova u memoriji

        for (auto F : kvp.second)
            File << "\tNode" << kvp.first << " -> " << "Node" << F << ";\n";
    }

    File << "}";
    File.close();
}
