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
#define STRINGS_BASE 1 << 6
#define STRINGS_MAX 1 << 18
#define VERTICE_EM_V0 1
#define VERTICE_EM_V1 2
#define VERTICE_EM_V2 3

#define VERTICE_BRANCO VERTICE_EM_V0
#define VERTICE_AZUL VERTICE_EM_V1
#define VERTICE_VERMELHO VERTICE_EM_V2

typedef struct vertice vertice;
typedef struct vizinho vizinho;

struct grafo {
    char nome[TAM_LINHA_MAX];
    LIST_HEAD(listaVertices, vertice) vertices;
    unsigned int numV;
    unsigned int numA;

    unsigned int numVcorte;
};

struct vertice {
    LIST_HEAD(listaVizinhos, vizinho) vizinhos;
    LIST_ENTRY(vertice) entradas;
    LIST_ENTRY(vertice)
    entradasTmp;     /* usado para inserir vertice em outra fila */
    long estado;     /* variavel auxiliar para algoritmos */
    vertice *pai;    /* Pai do vértice | para algoritmos */
    int L, lowpoint; /* L(v) | l(v)*/
    int corte;       /* Indica se o vértice é de corte*/
    int filhos;      /* Indica a quantidade de filhos na árvore. */
    char *nome;      /* Nome do vértice */
};

struct vizinho {
    /* cada vizinho eh referente a um vertice */
    vertice *verticeRef;
    LIST_ENTRY(vizinho) entradas;
    long peso;
};

/*
 * lista auxiliar para guardar vértices
 * estrutura definida para uso de ponteiros para lista.
 */
struct listaAuxiliar {
    struct vertice *lh_first;
};

/* vetor global usado para salvar ponteiros de strings alocadas
 * usado para dar free em todas strings ao fim de le_grafo */
char **strings = NULL;
unsigned int usadoStrings = 0;
unsigned int tamanhoAtual = 0;
char *vertices_de_corte = NULL;

void Merge(char **v, int a, int m, int b);
void MergeSort(char **v, int a, int b);

char **ordenaLista(void *headLista, int tamanho) {
    char **v = malloc(tamanho * sizeof(char *));
    if (v == NULL) return NULL;

    int i = 0;
    vertice *verticeIt;
    struct listaAuxiliar *head = headLista;
    LIST_FOREACH(verticeIt, head, entradasTmp) {
        v[i] = verticeIt->nome;
        i++;
    }

    MergeSort(v, 0, tamanho - 1);

    return v;
}

void adicionarVertice(ENTRY *entryP, grafo *grafoP) {
    vertice *novoVertice;
    int len = strlen(entryP->key);
    novoVertice = malloc(sizeof(vertice));
    assert(novoVertice != NULL);
    LIST_INIT(&novoVertice->vizinhos);

    novoVertice->pai = NULL;
    novoVertice->nome = entryP->key;
    novoVertice->L = novoVertice->lowpoint = 0;
    novoVertice->corte = novoVertice->filhos = 0;
    strings[usadoStrings++] = entryP->key;

    if (usadoStrings >= tamanhoAtual) {
        tamanhoAtual = tamanhoAtual * 2;
        char **tmp = realloc(strings, tamanhoAtual * sizeof(char *));
        if (tmp == NULL) {
            fprintf(stderr, "Falha na realocação de strings.\n");
            exit(EXIT_FAILURE);
        }
        strings = tmp;
    }

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
    grafoG->numV = grafoG->numA = grafoG->numVcorte = 0;

    tamanhoAtual = STRINGS_BASE;
    strings = malloc(tamanhoAtual * sizeof(char *));

    ENTRY *entryP1, *entryP2;
    hcreate(STRINGS_MAX);

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
        free(verticeIt);
    }

    for (int i = 0; i < usadoStrings; ++i) {
        free(strings[i]);
    }

    if (strings) free(strings);

    if (vertices_de_corte) free(vertices_de_corte);
    free(g);

    return 1;
}

void zerarEstadosVertices(grafo *grafoP) {
    vertice *verticeIt;
    LIST_FOREACH(verticeIt, &grafoP->vertices, entradas) {
        verticeIt->estado = VERTICE_EM_V0;
        verticeIt->pai = NULL;
        verticeIt->corte = 0;
        verticeIt->filhos = 0;
    }
}

char *nome(grafo *g) { return g->nome; }

int busca_bipartido(grafo *g, vertice *raiz) {
    int bipartido = 1;
    vertice *v, *w, *ultimo;
    vizinho *vizinhoIt;

    LIST_HEAD(lista, vertice) lista;
    LIST_INIT(&lista);
    raiz->estado = VERTICE_VERMELHO;
    LIST_INSERT_HEAD(&lista, raiz, entradasTmp);
    ultimo = raiz;

    while (!(LIST_EMPTY(&lista)) && (bipartido)) {
        v = lista.lh_first;
        LIST_FOREACH(vizinhoIt, &v->vizinhos, entradas) {
            w = vizinhoIt->verticeRef;
            if (w->estado == v->estado) {
                bipartido = 0;
                break;
            } else if (w->estado == VERTICE_BRANCO) {
                w->estado = (v->estado == VERTICE_VERMELHO) ? VERTICE_AZUL
                                                            : VERTICE_VERMELHO;
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
        if ((verticeIt->estado == VERTICE_BRANCO) && bipartido)
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

void lowpoint(grafo *g, vertice *raiz, void *listaAuxiliar, long int *total) {
    vertice *w;
    vizinho *vizinhoIt;
    raiz->estado = VERTICE_EM_V1;

    struct listaAuxiliar *headp = listaAuxiliar;

    LIST_FOREACH(vizinhoIt, &raiz->vizinhos, entradas) {
        w = vizinhoIt->verticeRef;
        if ((w->estado == VERTICE_EM_V1) && (w->lowpoint < raiz->L) &&
            w != raiz->pai) {
            raiz->lowpoint = w->L;
        } else if (w->estado == VERTICE_EM_V0) {
            w->pai = raiz;
            w->L = w->lowpoint = raiz->lowpoint + 1;
            raiz->filhos++;
            lowpoint(g, w, headp, total);
            if (w->lowpoint < raiz->L) {
                raiz->lowpoint = w->lowpoint;
            } else {
                if (raiz->corte == 0) {
                    raiz->corte = 1;
                    g->numVcorte++;
                    *total += strlen(raiz->nome) + 1;
                    LIST_INSERT_HEAD(headp, raiz, entradasTmp);
                }
            }
        }
    }

    raiz->estado = VERTICE_EM_V2;
}

char *vertices_corte(grafo *g) {
    if (g == NULL) return NULL;

    if (vertices_de_corte != NULL) return vertices_de_corte;

    g->numVcorte = 0;
    int cont = 0;
    zerarEstadosVertices(g);
    vertice *verticeIt;

    LIST_HEAD(listaAuxiliar, vertice) listaAuxiliar;
    LIST_INIT(&listaAuxiliar);
    struct listaAuxiliar *headp = &listaAuxiliar;

    long int total = 1;
    LIST_FOREACH(verticeIt, &g->vertices, entradas) {
        if (verticeIt->estado == VERTICE_EM_V0) {
            verticeIt->L = verticeIt->lowpoint = 0;
            lowpoint(g, verticeIt, headp, &total);
            if ((verticeIt->corte) && (verticeIt->filhos == 1)) {
                LIST_REMOVE(verticeIt, entradasTmp);
                g->numVcorte--;
                total -= strlen(verticeIt->nome) + 1;
            }
        }
    }

    char **v = ordenaLista(headp, g->numVcorte);

    char *s = malloc(total);
    if (!(s)) {
        free(v);
        return NULL;
    }
    s[0] = '\0';

    for (int i = 0; i < g->numVcorte; ++i) {
        strcat(s, v[i]);
        if (i < g->numVcorte - 1) strcat(s, " ");
    }

    vertices_de_corte = s;

    free(v);

    return s;
}

char *arestas_corte(grafo *g) {}

void Merge(char **v, int a, int m, int b) {
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

void MergeSort(char **v, int a, int b) {
    if (a < b) {
        int m = (a + b) / 2;
        MergeSort(v, a, m);
        MergeSort(v, m + 1, b);
        Merge(v, a, m, b);
    }
}