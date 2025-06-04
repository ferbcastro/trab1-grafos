#include "grafo.h"

int main() {
    int bip;
    grafo* g = le_grafo(stdin);
    bip = bipartido(g);
    if (bip) {
        printf("Bipartido\n");
    } else {
        printf("Não Bipartido\n");
    }
    destroi_grafo(g);

    return 0;
}