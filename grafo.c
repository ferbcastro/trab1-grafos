#include "grafo.h"

#include <assert.h>
#include <search.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#ifndef DEBUG
#define DEBUG_PRINT(...) \
{ while (0); }
#else
#define DEBUG_PRINT(...)              \
{                                 \
  fprintf(stderr, "[DEBUG] ");  \
  fprintf(stderr, __VA_ARGS__); \
}
#endif

#define TAM_LINHA_MAX 2047
#define STRINGS_MAX 1 << 18
#define TRUE 1
#define FALSE 0
#define ARESTAS_CORTE 2
#define VERTICES_CORTE 3

#define ESPACO " "
#define COMENTARIO "//"
#define ARESTA "--"

#define VERTICE_EM_V0 1
#define VERTICE_EM_V1 2
#define VERTICE_EM_V2 3
#define VERTICE_BRANCO VERTICE_EM_V0
#define VERTICE_AZUL VERTICE_EM_V1
#define VERTICE_VERMELHO VERTICE_EM_V2

#define ELEMENTOS_TIPO_LONG 1
#define ELEMENTOS_TIPO_STRING 2

typedef struct vertice vertice;
typedef struct vizinho vizinho;
typedef struct componente componente;

struct grafo {
  char nome[TAM_LINHA_MAX];
  LIST_HEAD(listaVertices, vertice) vertices;
  LIST_HEAD(listaComp, componente) componentes;
  unsigned int numV;
  unsigned int numA;
  unsigned int numVcorte;
  unsigned int numAcorte;
  unsigned int numComponentes;
  char ehPonderado;
};

struct componente {
  struct listaVertices vertices;
  LIST_ENTRY(componente) entradas;
};

struct vertice {
  LIST_HEAD(listaVizinhos, vizinho) vizinhos;
  LIST_ENTRY(vertice) entradasVertices;
  LIST_ENTRY(vertice) entradasComponentes;
  LIST_ENTRY(vertice) entradasTmp;
  vertice *pai;     /* Pai do vértice | para algoritmos */
  char estado;      /* variavel auxiliar para algoritmos */
  char L, lowPoint; /* L(v) | l(v) */
  char corte;       /* Indica se o vértice é de corte */
  char *nome;
  long custo;
};

struct vizinho {
  vertice *verticeRef; /* cada vizinho eh referente a um vertice */
  LIST_ENTRY(vizinho) entradas;
  long peso;
  char *nome;
};

long totalBytes;
char *verticesCorte = NULL;
char *arestasCorte = NULL;
char *diametrosString = NULL;

void mergeStrings(char **v, int a, int m, int b);
void mergeLongs(long *v, int a, int m, int b);
void mergeSort(void *v, int a, int b, int tipoDosElementos);

void adicionarVertice(ENTRY *entryP, grafo *grafoP) {
  vertice *novoVertice;
  int len = strlen(entryP->key);
  novoVertice = malloc(sizeof(vertice));
  assert(novoVertice != NULL);
  LIST_INIT(&novoVertice->vizinhos);

  novoVertice->nome = entryP->key;
  novoVertice->L = novoVertice->lowPoint = 0;
  novoVertice->corte = FALSE;
  if (LIST_EMPTY(&grafoP->vertices)) {
    LIST_INSERT_HEAD(&grafoP->vertices, novoVertice, entradasVertices);
  } else {
    LIST_INSERT_AFTER(grafoP->vertices.lh_first, novoVertice, entradasVertices);
  }
  entryP->data = (void *)novoVertice;
  hsearch((*entryP), ENTER);
}

ENTRY *verificaVertice(char *sub, grafo *grafoP) {
  ENTRY entry;
  ENTRY *entryP;
  entry.key = sub;
  entryP = hsearch(entry, FIND);
  if (entryP == NULL) { /* entrada nao mapeada pelo hash map */
    DEBUG_PRINT("Entrada [%s] nao encontrada\n", entry.key);
    entry.key = strdup(sub); /* sub eh temporario, precisa ser duplicado */

    adicionarVertice(&entry, grafoP);
    entryP = hsearch(entry, FIND);
    assert(entryP != NULL);

    grafoP->numV++;
  }
  return entryP;
}

void adicionarVizinho(int peso, vertice *vp1, vertice *vp2) {
  vizinho *vizinho1;
  vizinho *vizinho2;
  char *first;
  char *second;

  vizinho1 = malloc(sizeof(vizinho));
  assert(vizinho1 != NULL);
  vizinho2 = malloc(sizeof(vizinho));
  assert(vizinho2 != NULL);

  vizinho1->peso = vizinho2->peso = peso;
  vizinho1->verticeRef = vp1;
  vizinho2->verticeRef = vp2;

  /* insere vertice 2 nos vizinhos do vertice 1 */
  if (LIST_EMPTY(&vp1->vizinhos)) {
    LIST_INSERT_HEAD(&vp1->vizinhos, vizinho2, entradas);
  } else {
    LIST_INSERT_AFTER(vp1->vizinhos.lh_first, vizinho2, entradas);
  }

  /* insere vertice 1 nos vizinhos do vertice 2 */
  if (LIST_EMPTY(&vp2->vizinhos)) {
    LIST_INSERT_HEAD(&vp2->vizinhos, vizinho1, entradas);
  } else {
    LIST_INSERT_AFTER(vp2->vizinhos.lh_first, vizinho1, entradas);
  }

  /* sizeof('\0') + sizeof(' ') = 2 */
  long tam = strlen(vp1->nome) + strlen(vp2->nome) + 2;
  vizinho1->nome = malloc(tam);
  assert(vizinho1->nome != NULL);
  vizinho2->nome = vizinho1->nome;

  vizinho1->nome[0] = '\0';
  if (strcmp(vp1->nome, vp2->nome) < 0) { /* s1 < s2 */
    first = vp1->nome;
    second = vp2->nome;
  } else {
    first = vp2->nome;
    second = vp1->nome;
  }
  strcat(vizinho1->nome, first);
  strcat(vizinho1->nome, " ");
  strcat(vizinho1->nome, second);

  DEBUG_PRINT("Nome vizinhanca [%s], tam [%ld]\n", vizinho2->nome, tam);
}

grafo *le_grafo(FILE *f) {
  grafo *grafoG;
  char line[TAM_LINHA_MAX];
  char *auxP;
  int peso;

  grafoG = malloc(sizeof(grafo));
  assert(grafoG != NULL);
  LIST_INIT(&grafoG->vertices);
  LIST_INIT(&grafoG->componentes);
  grafoG->numV = 0;
  grafoG->numA = 0;
  grafoG->numVcorte = 0;
  grafoG->ehPonderado = FALSE;
  grafoG->nome[0] = '\0';

  ENTRY *entryP1, *entryP2;
  hcreate(STRINGS_MAX);

  while (fgets(line, TAM_LINHA_MAX, f)) {
    auxP = strchr(line, '\n');
    if (auxP != NULL) *auxP = '\0'; /* remover '\n' */
    if (line[0] == '\0') continue;  /* ignora linha em branco */
    if (!strncmp(COMENTARIO, line, sizeof(COMENTARIO) - 1)) continue;

    if (grafoG->nome[0] == '\0') {
      strcpy(grafoG->nome, line);
      continue;
    }

    char *substring = strtok(line, ESPACO);
    entryP1 = verificaVertice(substring, grafoG);
    DEBUG_PRINT("Primeiro vertice [%s]\n", substring);

    substring = strtok(NULL, ESPACO);
    if (substring != NULL) { /* se ha algo mais, deve ser string ARESTA */
      assert(!strncmp(ARESTA, substring, sizeof(ARESTA) - 1));
      grafoG->numA++;
    } else { /* vertice isolado */
      continue;
    }

    substring = strtok(NULL, ESPACO);
    entryP2 = verificaVertice(substring, grafoG);
    DEBUG_PRINT("Segundo vertice [%s]\n", substring);

    substring = strtok(NULL, ESPACO);
    if (substring == NULL) { /* aresta sem peso */
      adicionarVizinho(-1, (vertice *)entryP1->data,
      (vertice *)entryP2->data);
    } else { /* aresta com peso */
      sscanf(substring, "%d", &peso);
      adicionarVizinho(peso, (vertice *)entryP1->data,
      (vertice *)entryP2->data);
      grafoG->ehPonderado = TRUE;
      DEBUG_PRINT("Peso [%d]\n", peso);
    }
  }

  hdestroy();

  return grafoG;
}

unsigned int destroi_grafo(grafo *g) {
  vertice *verticeIt;
  vizinho *vizinhoIt;
  componente *componenteIt;
  while (!LIST_EMPTY(&g->vertices)) {
    while (!LIST_EMPTY(&g->vertices.lh_first->vizinhos)) {
      vizinhoIt = g->vertices.lh_first->vizinhos.lh_first;
      LIST_REMOVE(vizinhoIt, entradas);
      /* nome do vizinho eh marcado com '\0' para evitar double free */
      if (vizinhoIt->nome[0] == '\0') {
        free(vizinhoIt->nome);
      } else {
        vizinhoIt->nome[0] = '\0';
      }
      free(vizinhoIt);
    }
    verticeIt = g->vertices.lh_first;
    LIST_REMOVE(verticeIt, entradasVertices);
    free(verticeIt->nome);
    free(verticeIt);
  }

  while (!LIST_EMPTY(&g->componentes)) {
    componenteIt = g->componentes.lh_first;
    LIST_REMOVE(componenteIt, entradas);
    free(componenteIt);
  }

  if (verticesCorte) free(verticesCorte);
  if (arestasCorte) free(arestasCorte);
  if (diametrosString) free(diametrosString);
  free(g);

  return 1;
}

void zerarEstadosVertices(grafo *grafoP) {
  vertice *verticeIt;
  LIST_FOREACH(verticeIt, &grafoP->vertices, entradasVertices) {
    verticeIt->estado = VERTICE_EM_V0;
    verticeIt->pai = NULL;
    verticeIt->corte = FALSE;
    verticeIt->custo = 0;
  }
}

char *nome(grafo *g) { return g->nome; }

int buscaBipartido(grafo *g, vertice *raiz) {
  vertice *v, *w, *ultimo;
  vizinho *vizinhoIt;

  LIST_HEAD(lista, vertice) lista;
  LIST_INIT(&lista);
  LIST_INSERT_HEAD(&lista, raiz, entradasTmp);
  raiz->estado = VERTICE_VERMELHO;
  ultimo = raiz;

  while (!LIST_EMPTY(&lista)) {
    v = lista.lh_first;
    LIST_FOREACH(vizinhoIt, &v->vizinhos, entradas) {
      w = vizinhoIt->verticeRef;
      if (w->estado == v->estado) {
        return 0; /* vizinhos com mesma cor */
      } else if (w->estado == VERTICE_BRANCO) {
        w->estado = (v->estado == VERTICE_VERMELHO) ? VERTICE_AZUL
        : VERTICE_VERMELHO;
        LIST_INSERT_AFTER(ultimo, w, entradasTmp);
        ultimo = w;
      }
    }
    LIST_REMOVE(v, entradasTmp);
  }

  return 1;
}

unsigned int bipartido(grafo *g) {
  vertice *verticeIt;

  zerarEstadosVertices(g);
  LIST_FOREACH(verticeIt, &g->vertices, entradasVertices) {
    if (verticeIt->estado == VERTICE_BRANCO)
      if (!buscaBipartido(g, verticeIt)) return 0;
  }

  return 1;
}

unsigned int n_vertices(grafo *g) { return g->numV; }

unsigned int n_arestas(grafo *g) { return g->numA; }

void componenteF(vertice *v, componente *c) {
  LIST_HEAD(lista, vertice) lista;
  LIST_INIT(&lista);
  v->estado = VERTICE_EM_V1;
  LIST_INSERT_HEAD(&lista, v, entradasTmp);
  vertice *verticeIt;
  vizinho *vizinhoIt;
  while (!LIST_EMPTY(&lista)) {
    verticeIt = lista.lh_first;
    LIST_FOREACH(vizinhoIt, &verticeIt->vizinhos, entradas) {
      if (vizinhoIt->verticeRef->estado == VERTICE_EM_V0) {
        vizinhoIt->verticeRef->estado = VERTICE_EM_V1;
        LIST_INSERT_AFTER(verticeIt, vizinhoIt->verticeRef, entradasTmp);
        LIST_INSERT_AFTER(verticeIt, c->vertices.lh_first, entradasComponentes);
      }
    }
    LIST_REMOVE(verticeIt, entradasTmp);
    verticeIt->estado = VERTICE_EM_V2;
  }
}

unsigned int n_componentes(grafo *g) {
  componente *novoComponente;
  g->numComponentes = 0;

  zerarEstadosVertices(g);
  vertice *verticeIt;
  LIST_FOREACH(verticeIt, &g->vertices, entradasVertices) {
    if (verticeIt->estado == VERTICE_EM_V0) {
      novoComponente = malloc(sizeof(componente));
      assert(novoComponente != NULL);
      LIST_INIT(&novoComponente->vertices);
      LIST_INSERT_HEAD(&novoComponente->vertices, verticeIt, entradasComponentes);

      componenteF(verticeIt, novoComponente);
      g->numComponentes++;

      if (LIST_EMPTY(&g->componentes)) {
        LIST_INSERT_HEAD(&g->componentes, novoComponente, entradas);
      } else {
        LIST_INSERT_AFTER(g->componentes.lh_first, novoComponente, entradas);
      }
    }
  }

  return g->numComponentes;
}

/* algoritmo de low point modificado para encontrar vertices
 * ou arestas de corte; objetivo eh indicado no ultimo parametro */
void lowPoint(grafo *g, vertice *raiz, char **strings, int obj) {
  vertice *w;
  vizinho *vizinhoIt;
  static char primeiraChamada = TRUE;
  char raizEspecial;
  unsigned int raizFilhos = 0;

  raizEspecial = primeiraChamada;
  primeiraChamada = FALSE;
  raiz->estado = VERTICE_EM_V1;

  LIST_FOREACH(vizinhoIt, &raiz->vizinhos, entradas) {
    w = vizinhoIt->verticeRef;
    if ((w->estado == VERTICE_EM_V1) && (w->L < raiz->lowPoint) &&
    (w != raiz->pai)) {
      raiz->lowPoint = w->L;
    }

    if (w->estado != VERTICE_EM_V0) continue;

    w->pai = raiz;
    w->L = w->lowPoint = raiz->L + 1;
    lowPoint(g, w, strings, obj);
    if (w->lowPoint < raiz->lowPoint) {
      raiz->lowPoint = w->lowPoint;
    }
    if (w->lowPoint >= raiz->L) {
      if (obj == VERTICES_CORTE) {
        if (raiz->corte == TRUE) continue;

        if (raizEspecial == TRUE) {
          raizFilhos++;
          if (raizFilhos < 2) continue;
        }

        raiz->corte = TRUE;
        totalBytes += strlen(raiz->nome) + 1;
        strings[g->numVcorte++] = raiz->nome;

      } else if (obj == ARESTAS_CORTE) {
        strings[g->numAcorte++] = vizinhoIt->nome;
        totalBytes += strlen(vizinhoIt->nome) + 1;
      }
    }
  }

  raiz->estado = VERTICE_EM_V2;
}

char *lowPointComponentes(grafo *grafoG, int objetivo) {
  char *string;
  unsigned int *numCorte;
  char **strings;
  size_t tamStrings;
  vertice *verticeIt;

  if (objetivo == VERTICES_CORTE) {
    string = verticesCorte;
    numCorte = &grafoG->numVcorte;
    tamStrings = grafoG->numV;
  } else {
    string = arestasCorte;
    numCorte = &grafoG->numAcorte;
    tamStrings = grafoG->numA;
  }

  if (string != NULL) return string;

  strings = malloc(sizeof(char *) * tamStrings);
  assert(strings != NULL);

  *numCorte = 0;
  totalBytes = 0;
  zerarEstadosVertices(grafoG);
  LIST_FOREACH(verticeIt, &grafoG->vertices, entradasVertices) {
    if (verticeIt->estado == VERTICE_EM_V0) {
      verticeIt->L = verticeIt->lowPoint = 0;
      lowPoint(grafoG, verticeIt, strings, objetivo);
    }
  }

  mergeSort(strings, 0, *numCorte - 1, ELEMENTOS_TIPO_STRING);
  string = malloc(totalBytes);
  assert(string != NULL);
  string[0] = '\0';
  for (int i = 0; i < *numCorte; ++i) {
    strcat(string, strings[i]);
    if (i < *numCorte - 1) strcat(string, " ");
    DEBUG_PRINT("%s\n", strings[i]);
  }
  free(strings);

  return string;
}

char *vertices_corte(grafo *g) {
  verticesCorte = lowPointComponentes(g, VERTICES_CORTE);
  return verticesCorte;
}

char *arestas_corte(grafo *g) {
  arestasCorte = lowPointComponentes(g, ARESTAS_CORTE);
  return arestasCorte;
}

void mergeSort(void *v, int a, int b, int tipoDosElementos) {
  if (a < b) {
    int m = (a + b) / 2;
    mergeSort(v, a, m, tipoDosElementos);
    mergeSort(v, m + 1, b, tipoDosElementos);
    if (tipoDosElementos == ELEMENTOS_TIPO_LONG) {
      char **strings = v;
      mergeStrings(strings, a, m, b);
    } else if (tipoDosElementos == ELEMENTOS_TIPO_STRING) {
      long *longs = v;
      mergeLongs(longs, a, m, b);
    }
  }
}

long buscaLargura(grafo *grafoG, vertice *raiz);
long buscaDijkstra(grafo *grafoG, vertice *raiz);

char *diametros(grafo *g) {
  long *diametros;
  long diametrosIt = 0;
  long ret;

  if (diametrosString != NULL) return diametrosString;

  if (g->componentes.lh_first == NULL) {
    n_componentes(g);
  }
  diametros = malloc(sizeof(long) * g->numComponentes);
  assert(diametros != NULL);
  memset(diametros, 0, sizeof(long) * g->numComponentes);
  totalBytes = 0;

  componente *componenteIt;
  vertice *verticeIt;
  LIST_FOREACH(componenteIt, &g->componentes, entradas) {
    LIST_FOREACH(verticeIt, &componenteIt->vertices, entradasComponentes) {
      zerarEstadosVertices(g);
      if (g->ehPonderado == TRUE) {
        ret = buscaDijkstra(g, verticeIt);
      } else {
        ret = buscaLargura(g, verticeIt);
      }
      if (ret > diametros[diametrosIt]) {
        diametros[diametrosIt] = ret;
      }
    }

    diametrosIt++;
  }

  diametrosString = malloc(totalBytes);
  assert(diametrosString != NULL);

  return diametrosString;
}

long buscaLargura(grafo *grafoG, vertice *raiz) {
  long maiorDist = 0;

  return maiorDist;
}

long buscaDijkstra(grafo *grafoG, vertice *raiz) {
  long maiorCusto = 0;

  LIST_HEAD(lista, vertice) lista;
  LIST_INIT(&lista);
  LIST_INSERT_HEAD(&lista, raiz, entradasTmp);

  raiz->custo = 0;
  raiz->estado = VERTICE_EM_V1;
  vertice *verticeIt;
  vizinho *vizinhoIt;
  while (!LIST_EMPTY(&lista)) {
    verticeIt = lista.lh_first;
    if (maiorCusto < verticeIt->custo) {
      maiorCusto = verticeIt->custo;
    }
    LIST_FOREACH(vizinhoIt, &verticeIt->vizinhos, entradas) {
      if (vizinhoIt->verticeRef->estado == VERTICE_EM_V0) {
        vizinhoIt->verticeRef->custo = verticeIt->custo + vizinhoIt->peso;
        vizinhoIt->verticeRef->estado = VERTICE_EM_V1;

        // inserir ordenado
        vertice* iteradorLista;
        LIST_FOREACH(iteradorLista, &lista, entradasTmp) {
          if (vizinhoIt->verticeRef->custo < iteradorLista->custo) {
            break;
          }
        }
        LIST_INSERT_BEFORE(iteradorLista, vizinhoIt->verticeRef, entradasTmp);
      } else if (vizinhoIt->verticeRef->estado == VERTICE_EM_V1) {
        if (verticeIt->custo + vizinhoIt->peso < vizinhoIt->verticeRef->custo) {
          vizinhoIt->verticeRef->custo = verticeIt->custo + vizinhoIt->peso;
        }
      }
    }
    verticeIt->estado = VERTICE_EM_V2;
    LIST_REMOVE(verticeIt, entradasTmp);
  }

  return maiorCusto;
}

void mergeStrings(char **v, int a, int m, int b) {
  int i, j, k, n1, n2;

  n1 = m - a + 1;
  n2 = b - m;

  char **L = malloc(n1 * sizeof(char *));
  char **R = malloc(n2 * sizeof(char *));
  assert(L != NULL);
  assert(R != NULL);

  for (i = 0; i < n1; ++i) L[i] = v[a + i];
  for (j = 0; j < n2; ++j) R[j] = v[m + 1 + j];

  i = 0;
  j = 0;
  k = a;

  while ((i < n1) && (j < n2)) {
    if (strcmp(L[i], R[j]) < 0) {
      v[k] = L[i];
      i++;
    } else {
      v[k] = R[j];
      j++;
    }
    k++;
  }

  while (i < n1) {
    v[k] = L[i];
    i++;
    k++;
  }

  while (j < n2) {
    v[k] = R[j];
    j++;
    k++;
  }

  free(L);
  free(R);
}

void mergeLongs(long *v, int a, int m, int b) {
  int i, j, k, n1, n2;

  n1 = m - a + 1;
  n2 = b - m;

  long *L = malloc(n1 * sizeof(long));
  long *R = malloc(n2 * sizeof(long));
  assert(L != NULL);
  assert(R != NULL);

  for (i = 0; i < n1; ++i) L[i] = v[a + i];
  for (j = 0; j < n2; ++j) R[j] = v[m + 1 + j];

  i = 0;
  j = 0;
  k = a;

  while ((i < n1) && (j < n2)) {
    if (L[i] < R[j]) {
      v[k] = L[i];
      i++;
    } else {
      v[k] = R[j];
      j++;
    }
    k++;
  }

  while (i < n1) {
    v[k] = L[i];
    i++;
    k++;
  }

  while (j < n2) {
    v[k] = R[j];
    j++;
    k++;
  }

  free(L);
  free(R);
}