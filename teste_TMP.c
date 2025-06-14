#include "grafo.h"

int main() {
    grafo* g = le_grafo(stdin);
    puts(nome(g));
    if (bipartido(g)) {puts("Eh bipartido");}
    printf("Vértices de corte: %s\n", vertices_corte(g));
    destroi_grafo(g);

    return 0;
}