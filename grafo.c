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
#define TAM_VET_INIT 16

typedef struct vertice vertice;
typedef struct vizinho vizinho;

struct grafo {
  char nome[TAM_LINHA_MAX];
  vertice **vetListas;
  long numEntradas;
};

struct vertice {
  LIST_HEAD(listaVizinhos, vizinho) vizinhos;
  long estado; /* variavel auxiliar para algoritmos */
};

struct vizinho {
  vertice *verticeRef; /* cada vizinho eh referente a um vertice */
  LIST_ENTRY(vizinho) entradas;
  long peso;
};

char* adicionarVertice(ENTRY *entryP, grafo *grafoP, long *idx, long *tam) {
  vertice *novoVertice;
  novoVertice = malloc(sizeof(vertice));
  assert(novoVertice != NULL);
  LIST_INIT(&novoVertice->vizinhos);

  grafoP->vetListas[(*idx)++] = novoVertice;
  if ((*idx) >= (*tam)) {
    (*tam) *= 2;
    grafoP->vetListas = realloc(grafoP->vetListas, sizeof(vertice*) * (*tam));
    assert(grafoP->vetListas != NULL);
  }

  entryP->data = (void*)novoVertice;
  hsearch((*entryP), ENTER);
}

ENTRY* verificaVertice(char *sub, grafo *grafoP, long *idx, long *tam) {
  ENTRY entry;
  ENTRY *entryP;
  entry.key = strdup(sub); /* OBS: nao tenho ctz se hdestroy da free nas chaves */
  entryP = hsearch(entry, FIND);
  if (entryP == NULL) { /* entrada nao mapeada pelo hash map */
    adicionarVertice(&entry, grafoP, idx, tam);
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
  long idxNovoVertice = 0;
  long tamVetListas = TAM_VET_INIT;
  char line[TAM_LINHA_MAX];
  int peso;

  grafoG = malloc(sizeof(grafo));
  assert(grafoG != NULL);
  grafoG->vetListas = malloc(sizeof(vertice *) * tamVetListas);
  assert(grafoG->vetListas != NULL);

  ENTRY *entryP1, *entryP2;
  hcreate(HASH_MAX);

  fgets(grafoG->nome, TAM_LINHA_MAX, f);
  while (fgets(line, TAM_LINHA_MAX, f)) {
    if (line[0] == '\0') continue; /* ignora linha em branco */
    if (!strncmp(COMENTARIO, line, sizeof(COMENTARIO))) continue; /* ignora comentario */

    char *subtring = strtok(line, ESPACO);
    entryP1 = verificaVertice(subtring, grafoG, &idxNovoVertice, &tamVetListas);

    subtring = strtok(line, ESPACO);
    if (subtring != NULL) /* se ha algo mais, deve ser string ARESTA */
      assert(!strncmp(ARESTA, subtring, sizeof(ARESTA)));

    subtring = strtok(NULL, ESPACO);
    if (subtring == NULL) continue; /* vertice isolado */
    entryP2 = verificaVertice(subtring, grafoG, &idxNovoVertice, &tamVetListas);

    subtring = strtok(NULL, ESPACO);
    if (subtring == NULL) { /* aresta sem peso */
      adicionarVizinho(-1, (vertice*)entryP1->data, (vertice*)entryP2->data);
    } else { /* aresta com peso */
      sscanf(subtring, "%d", &peso);
      adicionarVizinho(peso, (vertice*)entryP1->data, (vertice*)entryP2->data);
    }
  }

  grafoG->numEntradas = idxNovoVertice;
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