/*
 * Estimacion de memoria (seccion 6.2) para v=2^15, e=2^20 (o los
 * exponentes que se pasen: ./mem_estimate [i] [j]).
 *
 * Genera de verdad el grafo y mide con sizeof en TU maquina y compilador.
 * Compara el resultado con la RAM de tu equipo (por ejemplo `free -h`).
 * El overhead de malloc (~8 bytes de cabecera + redondeo a 16) se
 * reporta aparte como estimacion.
 */
#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "binomial_heap.h"
#include "fibonacci_heap.h"

static double mib(long long b) { return (double)b / (1024.0 * 1024.0); }
static size_t malloc_chunk(size_t req) { /* glibc x86_64: req+8 redondeado a multiplo de 16, minimo 32 */
    size_t c = ((req + 8 + 15) / 16) * 16;
    return c < 32 ? 32 : c;
}

int main(int argc, char **argv) {
    int i = argc > 1 ? atoi(argv[1]) : 15;
    int j = argc > 2 ? atoi(argv[2]) : 20;
    long long n = 1LL << i, m = 1LL << j;

    Graph *g = graph_generate_random((int)n, m, 1);
    long long adj = graph_memory_bytes(g);

    long long aux = n * (long long)(sizeof(double) + sizeof(int) + sizeof(char)); /* costos, parent, in_queue */
    long long ptrs = n * (long long)sizeof(void *);                               /* pos[] */

    size_t nb = binomial_node_size(), nf = fibonacci_node_size();
    long long qb = n * (long long)nb, qf = n * (long long)nf;
    long long qb_m = n * (long long)malloc_chunk(nb), qf_m = n * (long long)malloc_chunk(nf);

    printf("Configuracion: v=2^%d=%lld, e=2^%d=%lld (aristas reales generadas: %lld)\n\n", i, n, j, m, g->m);
    printf("sizeof(int)=%zu sizeof(double)=%zu sizeof(void*)=%zu\n", sizeof(int), sizeof(double), sizeof(void *));
    printf("sizeof(nodo binomial)=%zu B   sizeof(nodo fibonacci)=%zu B\n\n", nb, nf);
    printf("Lista de adyacencia (CSR, cada arista 2 veces): %12lld B = %8.2f MiB\n", adj, mib(adj));
    printf("Arreglos auxiliares costos+parent+in_queue:     %12lld B = %8.2f MiB\n", aux, mib(aux));
    printf("Arreglo pos[] de punteros a nodos de Q:         %12lld B = %8.2f MiB\n", ptrs, mib(ptrs));
    printf("Nodos cola binomial   (sizeof * v):             %12lld B = %8.2f MiB  (con overhead malloc ~ %.2f MiB)\n", qb, mib(qb), mib(qb_m));
    printf("Nodos cola fibonacci  (sizeof * v):             %12lld B = %8.2f MiB  (con overhead malloc ~ %.2f MiB)\n\n", qf, mib(qf), mib(qf_m));
    printf("TOTAL Prim+binomial : %8.2f MiB\n", mib(adj + aux + ptrs + qb));
    printf("TOTAL Prim+fibonacci: %8.2f MiB (+ tabla de grados, despreciable)\n", mib(adj + aux + ptrs + qf));
    printf("\nNota: durante la generacion el programa mantiene ademas la lista de aristas\n"
           "(m * %zu B) y el hash set de duplicados, asi que el PICO de memoria al generar\n"
           "es mayor que el de Prim. Revisalo si e=2^24 te da problemas en tu RAM.\n", (size_t)(2 * sizeof(int) + sizeof(double)));
    graph_free(g);
    return 0;
}
