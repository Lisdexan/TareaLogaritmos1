#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>

/*
 * Grafo no dirigido, ponderado, representado en formato CSR (compact
 * adjacency): cada arista {u,v} se guarda dos veces (una por sentido).
 *
 *   head[v] .. head[v+1]-1   son los indices en to[]/weight[] de los
 *   vecinos de v.
 *
 * Esta representacion es la mas compacta posible para lectura (no hay
 * punteros por nodo, solo 3 arreglos planos), lo que facilita la
 * estimacion de memoria pedida en la seccion 6.2 del enunciado.
 */
typedef struct {
    int n;          /* cantidad de vertices */
    long long m;    /* cantidad de aristas NO dirigidas (unicas) */
    int *head;      /* tamano n+1 */
    int *to;        /* tamano 2*m */
    double *weight; /* tamano 2*m */
} Graph;

void graph_free(Graph *g);

/*
 * Genera un grafo aleatorio conexo y simple con 'n' vertices y 'm'
 * aristas, siguiendo el metodo sugerido en el enunciado (seccion 6.1):
 *   1) arbol cobertor: cada vertice i (1<=i<n) se conecta a un vertice
 *      aleatorio en [0, i-1].
 *   2) se agregan las m-(n-1) aristas restantes al azar, descartando
 *      repetidas o reflexivas.
 * Los pesos son aleatorios uniformes en (0,1].
 *
 * 'seed' fija la semilla del generador (para reproducibilidad).
 * Requiere m >= n-1 y m <= n*(n-1)/2 (grafo simple).
 */
Graph *graph_generate_random(int n, long long m, uint64_t seed);

/* Guarda/carga el grafo en un archivo de texto plano:
 *   linea 1: "n m"
 *   siguientes m lineas: "u v w"   (peso con precision suficiente)
 */
int graph_save(const Graph *g, const char *filename);
Graph *graph_load(const char *filename);

/* Memoria (en bytes) que ocupan los 3 arreglos del grafo (sin overhead
 * de malloc). Util para la seccion 6.2 del informe. */
long long graph_memory_bytes(const Graph *g);

#endif
