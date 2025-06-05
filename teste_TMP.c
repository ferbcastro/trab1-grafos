#include "grafo.h"

int main() {
    grafo* g = le_grafo(stdin);
    vertices_corte(g);
    destroi_grafo(g);

    return 0;
}