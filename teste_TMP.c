#include "grafo.h"

/* temporario para testes */
#define DEBUG

int main() {
  grafo* g = le_grafo(stdin);
  destroi_grafo(g);

  return 0;
}