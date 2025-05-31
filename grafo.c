#include "grafo.h"
#include <sys/queue.h>
#include <stdlib.h>

#define MAX_SIZE_LINE 2047

typedef struct vertice vertice;
typedef struct vizinho vizinho;

struct grafo {
  char nome[MAX_SIZE_LINE];
  unsigned int verticesCont;
  unsigned int arestasCont;
  LIST_HEAD(listaVertices, vertice) vertices;
};

struct vertice {
  LIST_HEAD(listaVizinhos, vizinho) vizinhos;
};

struct vizinho {
  unsigned int idx;
  unsigned int peso;
};

grafo *le_grafo(FILE *f) {

}

unsigned int destroi_grafo(grafo *g) {

}

char *nome(grafo *g) {

}

unsigned int bipartido(grafo *g) {

}

unsigned int n_vertices(grafo *g) {

}

unsigned int n_arestas(grafo *g) {

}

unsigned int n_componentes(grafo *g) {

}

char *diametros(grafo *g) {

}

char *vertices_corte(grafo *g) {

}

char *arestas_corte(grafo *g) {

}