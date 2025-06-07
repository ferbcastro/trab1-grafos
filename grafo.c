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
#define NOME_MAX 128
#define ESPACO " "
#define COMENTARIO "//"
#define ARESTA "--"
#define HASH_MAX 1 << 18
#define STRINGS_MAX 4
#define VERTICE_EM_V0 1
#define VERTICE_EM_V1 2
#define VERTICE_EM_V2 3

typedef struct vertice vertice;
typedef struct vizinho vizinho;

struct grafo {
    char nome[TAM_LINHA_MAX];
    LIST_HEAD(listaVertices, vertice) vertices;
    unsigned int numV;
    unsigned int numA;
};

struct vertice {
    char *nome;
    LIST_HEAD(listaVizinhos, vizinho) vizinhos;
    LIST_ENTRY(vertice) entradas;

    /* usado para inserir vertice em outra fila */
    LIST_ENTRY(vertice) entradasTmp;
    /* variavel auxiliar para algoritmos */
    long estado;
    /* pai do vértice | para algoritmos*/
    vertice *pai;
};

struct vizinho {
    /* cada vizinho eh referente a um vertice */
    vertice *verticeRef;
    LIST_ENTRY(vizinho) entradas;
    long peso;
};

/* vetor global usado para salvar ponteiros de strings alocadas
 * usado para dar free em todas strings ao fim de le_grafo */
char *strings[STRINGS_MAX];
unsigned int usadoStrings = 0;

void adicionarVertice(ENTRY *entryP, grafo *grafoP) {
    vertice *novoVertice = malloc(sizeof(vertice));
    assert(novoVertice != NULL);
    LIST_INIT(&novoVertice->vizinhos);
    novoVertice->pai = NULL;

    /* Copia o nome do vértice para o campo nome da struct vertice */
    novoVertice->nome = entryP->key;

    if (LIST_EMPTY(&grafoP->vertices)) {
        LIST_INSERT_HEAD(&grafoP->vertices, novoVertice, entradas);
    } else {
        LIST_INSERT_AFTER(grafoP->vertices.lh_first, novoVertice, entradas);
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
        LIST_INSERT_AFTER(vp1->vizinhos.lh_first, vizinho2, entradas);
    }

    /* insere vertice 1 nos vizinhos do vertice 2 */
    vizinho2->peso = peso;
    vizinho2->verticeRef = vp2;
    if (LIST_EMPTY(&vp2->vizinhos)) {
        LIST_INSERT_HEAD(&vp2->vizinhos, vizinho1, entradas);
    } else {
        LIST_INSERT_AFTER(vp2->vizinhos.lh_first, vizinho1, entradas);
    }
}

grafo *le_grafo(FILE *f) {
    grafo *grafoG;
    char line[TAM_LINHA_MAX];
    int peso;

    grafoG = malloc(sizeof(grafo));
    assert(grafoG != NULL);
    LIST_INIT(&grafoG->vertices);
    grafoG->numV = grafoG->numA = 0;

    ENTRY *entryP1, *entryP2;
    hcreate(HASH_MAX);

    fgets(grafoG->nome, TAM_LINHA_MAX, f);
    grafoG->nome[strlen(grafoG->nome) - 1] = '\0'; /* remover '\n' */
    DEBUG_PRINT("Nome do grafo [%s]\n", grafoG->nome);
    while (fgets(line, TAM_LINHA_MAX, f)) {
        line[strlen(line) - 1] = '\0'; /* remover '\n' */
        if (line[0] == '\0') continue; /* ignora linha em branco */
        if (!strncmp(COMENTARIO, line, sizeof(COMENTARIO)))
            continue; /* ignora comentario */

        char *substring = strtok(line, ESPACO);
        entryP1 = verificaVertice(substring, grafoG);
        DEBUG_PRINT("Primeiro vertice [%s]\n", substring);

        substring = strtok(NULL, ESPACO);
        if (substring != NULL) { /* se ha algo mais, deve ser string ARESTA */
            assert(!strncmp(ARESTA, substring, sizeof(ARESTA)));
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
            DEBUG_PRINT("Peso [%d]\n", peso);
        }
    }

    hdestroy();

    return grafoG;
}

unsigned int destroi_grafo(grafo *g) {
    if (g == NULL) return 0;

    vertice *verticeIt;
    vizinho *vizinhoIt;
    while (!LIST_EMPTY(&g->vertices)) {
        while (!LIST_EMPTY(&g->vertices.lh_first->vizinhos)) {
            vizinhoIt = g->vertices.lh_first->vizinhos.lh_first;
            LIST_REMOVE(vizinhoIt, entradas);
            free(vizinhoIt);
        }
        verticeIt = g->vertices.lh_first;
        LIST_REMOVE(verticeIt, entradas);
        free(verticeIt->nome);
        free(verticeIt);
    }
    free(g);

    for (unsigned int i = 0; i < usadoStrings; i++)
      free(strings[i]);

    return 1;
}

void zerarEstadosVertices(grafo *grafoP) {
    vertice *verticeIt;
    LIST_FOREACH(verticeIt, &grafoP->vertices, entradas) {
        verticeIt->estado = VERTICE_EM_V0;
    }
}

char *nome(grafo *g) { return g->nome; }

int busca_bipartido(grafo *g, vertice *raiz) {
    int bipartido = 1;
    vertice *v, *w, *ultimo;
    vizinho *vizinhoIt;

    LIST_HEAD(lista, vertice) lista;
    LIST_INIT(&lista);
    raiz->estado = VERTICE_EM_V1;
    LIST_INSERT_HEAD(&lista, raiz, entradasTmp);
    ultimo = raiz;

    while (!(LIST_EMPTY(&lista)) && (bipartido)) {
        v = lista.lh_first;
        LIST_FOREACH(vizinhoIt, &v->vizinhos, entradas) {
            w = vizinhoIt->verticeRef;
            if (w->estado == v->estado) {
                bipartido = 0;
                break;
            } else if (w->estado == VERTICE_EM_V0) {
                w->estado = (v->estado == VERTICE_EM_V1) ? VERTICE_EM_V2
                                                         : VERTICE_EM_V1;
                LIST_INSERT_AFTER(ultimo, w, entradasTmp);
                ultimo = w;
            }
        }
        LIST_REMOVE(v, entradasTmp);
    }

    return bipartido;
}

unsigned int bipartido(grafo *g) {
    if (g == NULL) return 0;

    int bipartido = 1;
    vertice *verticeIt;

    zerarEstadosVertices(g);
    LIST_FOREACH(verticeIt, &g->vertices, entradas) {
        if ((verticeIt->estado == VERTICE_EM_V0) && bipartido)
            bipartido = busca_bipartido(g, verticeIt);
        if (!(bipartido)) break;
    }

    return bipartido;
}

unsigned int n_vertices(grafo *g) { return g->numV; }

unsigned int n_arestas(grafo *g) { return g->numA; }

void componente(vertice *v) {
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
                LIST_INSERT_AFTER(verticeIt, vizinhoIt->verticeRef,
                                  entradasTmp);
            }
        }
        LIST_REMOVE(verticeIt, entradasTmp);
        verticeIt->estado = VERTICE_EM_V2;
    }
}

unsigned int n_componentes(grafo *g) {
    unsigned int cont = 0;

    zerarEstadosVertices(g);
    vertice *verticeIt;
    LIST_FOREACH(verticeIt, &g->vertices, entradas) {
        if (verticeIt->estado == VERTICE_EM_V0) {
            componente(verticeIt);
            cont++;
        }
    }

    return cont;
}

char *diametros(grafo *g) {}

char *vertices_corte(grafo *g) {}

char *arestas_corte(grafo *g) {}