#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <fstream>

class DominatorTree
{
private:
  int num_of_vertices;

  // Lista povezanosti za graf. Ako postoji grana od cvora u ka proizvoljnom cvoru v, cuvamo {u: {v}}
  std::vector<std::vector<int>> adjacency_list;

  // Cuvamo grane obrnutog usmerenja od onog koje postoji u grafu. Ako smo imali granu 2 -> 3, ovde cuvamo 3 -> 2. Potrebno
  // nam je na dalje za proveru svih putanja u nazad.
  std::vector<std::vector<int>> reversed_adjacency_list;

  // Redosled obilaska cvorova u toku DFS pretrage.
  std::vector<int> visited_order;

  // Ulazna numeracija prilikom DFS pretrage. in_numeration[2] = 3 -> cvor numerisan brojem 2 ima ulaznu numeraciju 3.
  std::vector<int> in_numeration;

  // Marker vektor za posecene cvorove.
  std::vector<bool> visited;

  // Vektor koji pamti semidominatore za svaki cvor.
  std::vector<int> sdom;

  // Vektor roditeljskih cvorova.
  std::vector<int> parents;

  // Vektor predaka
  std::vector<int> ancestors;

  // Vektor immediate dominatora.
  std::vector<int> idom;

  void find_semi_dominator_candidates(int start_node, int current_node, std::vector<int>& candidates)
  {
    visited[current_node] = true;

    // Ukoliko je ulazna numeracija cvora koji je potencijalni kandidat manja u odnosu na numeraciju cvora za kog trazimo semidominatora,
    // dodajemo ga u vektor kandidata.
    if (in_numeration[start_node] > in_numeration[current_node]) {
      candidates.push_back(current_node);
      return;
    }

    // Ako prethodni uslov nije zadovoljen, krecemo se obrnutom putanjom od tekuceg cvora i razmatramo sve prethodno neposecene cvorove.
    // Za njih vrsimo istu proveru.
    for (int node : reversed_adjacency_list[current_node]) {
      if (!visited[node])
        find_semi_dominator_candidates(start_node, node, candidates);
    }
  }

  int find_semi_dominator(int node)
  {
    std::vector<int> candidates;
    // Posecene cvorove opet postavljamo na false, kako bismo ponavljali postupak trazenja kandidata za semidominatore.
    std::fill(visited.begin(), visited.end(), false);

    find_semi_dominator_candidates(node, node, candidates);

    int min_numeration = std::numeric_limits<int>::max();
    int semi_dominator;

    // Od svih kandidata, semidominator je onaj cvor koji ima NAJMANJU ULAZNU NUMERACIJU.
    for (int candidate : candidates) {
      if (in_numeration[candidate] < min_numeration) {
        min_numeration = in_numeration[candidate];
        semi_dominator = candidate;
      }
    }

    return semi_dominator;
  }

public:
  DominatorTree(int V)
  {
    num_of_vertices = V;

    adjacency_list.resize(num_of_vertices);
    reversed_adjacency_list.resize(num_of_vertices);
    visited.resize(num_of_vertices, false);
    in_numeration.resize(num_of_vertices, 0);

    sdom.resize(num_of_vertices);
    for (int i = 0; i < num_of_vertices; ++i)
      sdom[i] = i;
  }

  void add_edge(int u, int v)
  {
    adjacency_list[u].push_back(v);
    reversed_adjacency_list[v].push_back(u);
  }

  void DFS(int start_node, int &times)
  {
    visited[start_node] = true;
    visited_order.push_back(start_node);
    in_numeration[start_node] = times++;

    for (int node : adjacency_list[start_node]) {
      if (!visited[node]) {
        parents[node] = start_node;
        DFS(node, times);
      }
    }
  }

  void find_semi_dominators()
  {
    // Semidominatore trazimo za cvorove u OBRNUTOM poretku od onog u kom smo ih obilazili prilikom DFS pretrage.
    std::reverse(visited_order.begin(), visited_order.end());

    for (int node : visited_order) {
      // Pocetni cvor nema semidominatora, pa njega ignorisemo.
      if (node == 0)
        break;

      sdom[node] = find_semi_dominator(node);
//      std::cout << "sdom(" << node << ") = " << sdom[node] << "\n";
    }
  }

  void find_immediate_dominators()
  {
    int parent, min;

    // Za svaki cvor gledamo putanju do njegovog semidominatora
    for (int node : visited_order) {
      parent = parents[node];
      min = sdom[node];

      while (parent != sdom[node]) {
        if (sdom[parent] < min) {
          min = sdom[parent];
          ancestors[parent] = parent;
        }

        parent = parents[parent];
      }
    }

    // Vracamo se u poredak dobijen na pocetku pretrage
    std::reverse(visited_order.begin(), visited_order.end());

    for (int node : visited_order) {
      if (ancestors[node] == -1)           // postoji grana od sdom[node] do node
        idom[node] = sdom[node];
    }

    for (int node : visited_order) {
      if (ancestors[node] != -1)
        idom[node] = idom[ancestors[node]];
    }

    for (int i = 0; i < num_of_vertices; ++i)
      std::cout << "idom(" << i << ") = " << idom[i] << "\n";
  }

  void print_tree(std::string file_name, bool dominator_tree)
  {
    std::ofstream file(file_name + ".dot");

    file << "digraph \"" + file_name << " for 'main' function\" {\n";
    file << "\tlabel=\"" + file_name << " for 'main' function\";\n";

    for (int i = 0; i < num_of_vertices; ++i) {
      file << "\t" << i
           << " [shape=record, color=\"#072757\", style=filled, fillcolor=\"#2462bf\", label=\"{"
           << i << "}\"];\n";
      if (i != 0 && dominator_tree) {
        file << "\t" << idom[i] << " -> " << i << ";\n";
      } else if (i != 0 && !dominator_tree) {
        file << "\t" << sdom[i] << " -> " << i << ";\n";
      }
    }

    file << "}\n";

    file.close();
  }
};

int main ()
{
  DominatorTree dom_tree(8);

  dom_tree.add_edge(0, 1);
  dom_tree.add_edge(1, 2);
  dom_tree.add_edge(1, 3);
  dom_tree.add_edge(2, 3);
  dom_tree.add_edge(2, 6);
  dom_tree.add_edge(3, 4);
  dom_tree.add_edge(4, 5);
  dom_tree.add_edge(5, 7);
  dom_tree.add_edge(7, 5);
  dom_tree.add_edge(7, 6);
  dom_tree.add_edge(6, 7);
  dom_tree.add_edge(6, 2);

  int times = 1;
  dom_tree.DFS(0, times);
  dom_tree.find_semi_dominators();
  dom_tree.print_tree("semidominators", false);

  dom_tree.find_immediate_dominators();
  dom_tree.print_tree("dominator_tree", true);

  return 0;
}