#include "grafo.h"

int main() {
    grafo* g = le_grafo(stdin);
    printf("Vértices de corte: %s\n", vertices_corte(g));
    destroi_grafo(g);

    return 0;
}