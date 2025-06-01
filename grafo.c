#include "grafo.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAM_LINHA_MAX 2047
#define ESPACO " "
#define COMENTARIO "//"
#define ARESTA "--"

typedef struct vertice vertice;
typedef struct vizinho vizinho;

struct grafo {
  char nome[TAM_LINHA_MAX];
  unsigned int verticesCont;
  unsigned int arestasCont;
  vertice **listasVet;
};

struct vertice {
  LIST_HEAD(listaVizinhos, vizinho) vizinhos;
};

struct vizinho {
  unsigned int idx;
  unsigned int peso;
  LIST_ENTRY(vizinho) entradas;
};

void adicionarVertice(int idx, vertice **vptr) {
  *vptr = malloc(sizeof(vertice));
  LIST_INIT(&(*vptr)->vizinhos);
}

void adicionarVizinho(int peso, int idx, vertice *vptr) {
  vizinho *novoVizinho;

  novoVizinho = malloc(sizeof(vizinho));
  if (novoVizinho == NULL) {
    fprintf(stderr, "Erro ao alocar vizinho. Saindo.\n");
    exit(1);
  }
  novoVizinho->idx = idx;
  if (peso >= 0) {
    novoVizinho->peso = peso; /* pesos devem ser não negativos */
  }
  if (vptr->vizinhos.lh_first == NULL) {
    LIST_INSERT_HEAD(&vptr->vizinhos, novoVizinho, entradas);
  } else {
    LIST_INSERT_BEFORE(vptr->vizinhos.lh_first, novoVizinho, entradas);
  }
}

void tratarLinha(char *line, grafo *grafoG) {
  int primVertIdx;
  int segVertIdx;
  int pesoAresta;
  char *subString;

  if (!strcmp(COMENTARIO, line)) return;

  subString = strtok(line, ESPACO);
  sscanf(subString, "%d", &primVertIdx);
  if (grafoG->listasVet[primVertIdx] == NULL) {
    /* vertice nao conhecido */
    adicionarVertice(primVertIdx, &grafoG->listasVet[primVertIdx]);
    grafoG->verticesCont++;
  }

  subString = strtok(NULL, ESPACO);
  if (subString == NULL) return; /* linha sem aresta */
  if (strcmp(ARESTA, subString)) {
    fprintf(stderr, "Esperava por aresta. Saindo.\n");
    exit(1);
  }
  grafoG->arestasCont++;

  subString = strtok(NULL, ESPACO);
  sscanf(subString, "%d", &segVertIdx);
  if (grafoG->listasVet[segVertIdx] == NULL) {
    /* vertice nao conhecido */
    adicionarVertice(segVertIdx, &grafoG->listasVet[segVertIdx]);
    grafoG->verticesCont++;
  }

  subString = strtok(NULL, ESPACO);
  if (subString == NULL) { /* linha sem peso */
    pesoAresta = -1;
  } else {
    sscanf(subString, "%d", &pesoAresta);
  }
  adicionarVizinho(pesoAresta, segVertIdx, grafoG->listasVet[primVertIdx]);
  adicionarVizinho(pesoAresta, primVertIdx, grafoG->listasVet[segVertIdx]);
}

grafo *le_grafo(FILE *f) {
  grafo *grafoG;
  char line[TAM_LINHA_MAX];

  grafoG = malloc(sizeof(grafo));
  if (grafoG == NULL) {
    fprintf(stderr, "Erro ao alocar grafo. Saindo.\n");
    exit(1);
  }
  grafoG->listasVet = malloc(sizeof(vertice*) * 16384);
  if (grafoG->listasVet == NULL) {
    fprintf(stderr, "Erro ao alocar vetor de listas. Saindo.\n");
    exit(1);
  }
  memset(grafoG->listasVet, 0, sizeof(vertice*) * 16384);

  grafoG->arestasCont = 0;
  grafoG->verticesCont = 0;
  fgets(grafoG->nome, TAM_LINHA_MAX, f);
  while (fgets(line, TAM_LINHA_MAX, f)) {
    if (line[0] == '\0') continue;
    tratarLinha(line, grafoG);
  }

  return grafoG;
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