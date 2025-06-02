#include "grafo.h"

int main() {
  grafo* g = le_grafo(stdin);
  printf("Numero vertices = %u\n", n_vertices(g));
  destroi_grafo(g);

  return 0;
}