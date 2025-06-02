#include "grafo.h"

int main() {
  grafo* g = le_grafo(stdin);
  printf("Numero vertices = %u\n", n_vertices(g));
  printf("Numero arestas = %u\n", n_arestas(g));
  printf("Numero componentes = %u\n", n_componentes(g));
  destroi_grafo(g);

  return 0;
}