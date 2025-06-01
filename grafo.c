#include "grafo.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <search.h>
#include <sys/queue.h>

#define TAM_LINHA_MAX 2047
#define ESPACO " "
#define COMENTARIO "//"
#define ARESTA "--"
#define HASH_MAX 1 << 20

typedef struct vertice vertice;
typedef struct vizinho vizinho;

struct grafo {
  char nome[TAM_LINHA_MAX];
  LIST_HEAD(listaVertices, vertice) vertices;
};

struct vertice {
  LIST_HEAD(listaVizinhos, vizinho) vizinhos;
  LIST_ENTRY(vertice) entradas;
  long estado; /* variavel auxiliar para algoritmos */
};

struct vizinho {
  vertice *verticeRef; /* cada vizinho eh referente a um vertice */
  LIST_ENTRY(vizinho) entradas;
  long peso;
};

void adicionarVertice(ENTRY *entryP, grafo *grafoP) {
  vertice *novoVertice;
  novoVertice = malloc(sizeof(vertice));
  assert(novoVertice != NULL);
  LIST_INIT(&novoVertice->vizinhos);
  if (LIST_EMPTY(&grafoP->vertices)) {
    LIST_INSERT_HEAD(&grafoP->vertices, novoVertice, entradas);
  } else {
    LIST_INSERT_BEFORE(grafoP->vertices.lh_first, novoVertice, entradas);
  }
  entryP->data = (void*)novoVertice;
  hsearch((*entryP), ENTER);
}

ENTRY* verificaVertice(char *sub, grafo *grafoP) {
  ENTRY entry;
  ENTRY *entryP;
  entry.key = strdup(sub); /* OBS: nao tenho ctz se hdestroy faz free das chaves */
  entryP = hsearch(entry, FIND);
  if (entryP == NULL) { /* entrada nao mapeada pelo hash map */
    adicionarVertice(&entry, grafoP);
    entryP = hsearch(entry, FIND);
    assert(entryP != NULL);
  }
  return entryP;
}

void adicionarVizinho(int peso, vertice *vp1, vertice *vp2) {
  vizinho *vizinho1;
  vizinho *vizinho2;

  vizinho1 = malloc(sizeof(vizinho));
  assert(vizinho1 != NULL);
  vizinho2 = malloc(sizeof(vizinho));
  assert(vizinho2 != NULL);

  /* insere vertice 2 nos vizinhos do vertice 1 */
  vizinho1->peso = peso;
  vizinho1->verticeRef = vp1;
  if (LIST_EMPTY(&vp1->vizinhos)) {
    LIST_INSERT_HEAD(&vp1->vizinhos, vizinho2, entradas);
  } else {
    LIST_INSERT_BEFORE(vp1->vizinhos.lh_first, vizinho2, entradas);
  }

  /* insere vertice 1 nos vizinhos do vertice 2 */
  vizinho2->peso = peso;
  vizinho2->verticeRef = vp2;
  if (LIST_EMPTY(&vp2->vizinhos)) {
    LIST_INSERT_HEAD(&vp2->vizinhos, vizinho1, entradas);
  } else {
    LIST_INSERT_BEFORE(vp2->vizinhos.lh_first, vizinho1, entradas);
  }
}

grafo *le_grafo(FILE *f) {
  grafo *grafoG;
  char line[TAM_LINHA_MAX];
  int peso;

  grafoG = malloc(sizeof(grafo));
  assert(grafoG != NULL);
  LIST_INIT(&grafoG->vertices);

  ENTRY *entryP1, *entryP2;
  hcreate(HASH_MAX);

  fgets(grafoG->nome, TAM_LINHA_MAX, f);
  while (fgets(line, TAM_LINHA_MAX, f)) {
    if (line[0] == '\0') continue; /* ignora linha em branco */
    if (!strncmp(COMENTARIO, line, sizeof(COMENTARIO))) continue; /* ignora comentario */

    char *subtring = strtok(line, ESPACO);
    entryP1 = verificaVertice(subtring, grafoG);

    subtring = strtok(NULL, ESPACO);
    if (subtring != NULL) { /* se ha algo mais, deve ser string ARESTA */
      assert(!strncmp(ARESTA, subtring, sizeof(ARESTA)));
    } else { /* vertice isolado */
      continue;
    }

    subtring = strtok(NULL, ESPACO);
    entryP2 = verificaVertice(subtring, grafoG);

    subtring = strtok(NULL, ESPACO);
    if (subtring == NULL) { /* aresta sem peso */
      adicionarVizinho(-1, (vertice*)entryP1->data, (vertice*)entryP2->data);
    } else { /* aresta com peso */
      sscanf(subtring, "%d", &peso);
      adicionarVizinho(peso, (vertice*)entryP1->data, (vertice*)entryP2->data);
    }
  }

  hdestroy();

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