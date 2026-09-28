/*
 * Programa de VALIDACION (no es parte de la entrega). Compara, sobre
 * los mismos grafos aleatorios:
 *   - Prim + cola binomial
 *   - Prim + cola de Fibonacci
 *   - Kruskal + union-find (referencia externa, independiente de
 *     nuestras colas)
 * y chequea que las tres den el mismo peso de MST -- exactamente la
 * verificacion que pide la seccion 6.3.1 ("verifiquen que ambas
 * implementaciones produzcan un MST del mismo peso total"), sumando
 * ademas un tercer algoritmo de referencia para mayor confianza.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#include "graph.h"
#include "pq.h"
#include "binomial_heap.h"
#include "fibonacci_heap.h"
#include "prim.h"

/* ---- Union-Find + Kruskal de referencia (identico al de test_binomial.c) ---- */
typedef struct { int *p, *r; } DSU;
static void dsu_init(DSU *d, int n) { d->p = malloc(sizeof(int)*n); d->r = calloc(n, sizeof(int)); for (int i=0;i<n;i++) d->p[i]=i; }
static int dsu_find(DSU *d, int x) { while (d->p[x]!=x){d->p[x]=d->p[d->p[x]];x=d->p[x];} return x; }
static int dsu_union(DSU *d, int a, int b) {
    a=dsu_find(d,a); b=dsu_find(d,b);
    if (a==b) return 0;
    if (d->r[a]<d->r[b]) { int t=a;a=b;b=t; }
    d->p[b]=a;
    if (d->r[a]==d->r[b]) d->r[a]++;
    return 1;
}
static void dsu_free(DSU *d) { free(d->p); free(d->r); }

typedef struct { int u,v; double w; } E;
static int cmp_edge(const void *a, const void *b) {
    double wa=((const E*)a)->w, wb=((const E*)b)->w;
    return (wa>wb)-(wa<wb);
}
static double kruskal_reference(const Graph *g) {
    long long m = g->m;
    E *edges = malloc(sizeof(E)*m);
    long long idx=0;
    for (int u=0; u<g->n; u++)
        for (int k=g->head[u]; k<g->head[u+1]; k++)
            if (g->to[k]>u) edges[idx++] = (E){u, g->to[k], g->weight[k]};
    qsort(edges, idx, sizeof(E), cmp_edge);
    DSU d; dsu_init(&d, g->n);
    double total=0.0; int used=0;
    for (long long i=0; i<idx && used<g->n-1; i++)
        if (dsu_union(&d, edges[i].u, edges[i].v)) { total += edges[i].w; used++; }
    dsu_free(&d); free(edges);
    return total;
}

static double now_seconds(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec*1e-9;
}

int main(void) {
    int sizes[] = {5, 10, 50, 200, 1000, 5000};
    int nsizes = sizeof(sizes)/sizeof(sizes[0]);
    int total_cases = 0, ok_cases = 0;

    printf("== Validacion: Kruskal vs Prim+binomial vs Prim+fibonacci ==\n");
    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        for (int trial = 0; trial < 4; trial++) {
            long long max_m = (long long)n*(n-1)/2;
            long long m = n - 1 + (trial * (max_m - (n-1))) / 4;
            if (m < n-1) m = n-1;
            uint64_t seed = 7000ULL*n + trial + 1;

            Graph *g = graph_generate_random(n, m, seed);
            double kw = kruskal_reference(g);

            PrimResult pb = prim_run(g, 0, &BINOMIAL_PQ);
            PrimResult pf = prim_run(g, 0, &FIBONACCI_PQ);

            double d_bk = fabs(pb.total_weight - kw);
            double d_fk = fabs(pf.total_weight - kw);
            double d_bf = fabs(pb.total_weight - pf.total_weight);
            int pass = (d_bk < 1e-6) && (d_fk < 1e-6) && (d_bf < 1e-6);

            int eb=0, ef=0;
            for (int v=0; v<n; v++) { if (pb.parent[v]!=-1) eb++; if (pf.parent[v]!=-1) ef++; }
            int trees_ok = (eb == n-1) && (ef == n-1);

            total_cases++;
            ok_cases += (pass && trees_ok);

            printf("  n=%6d m=%9lld | Kruskal=%.6f Binomial=%.6f Fibonacci=%.6f | %s\n",
                   n, m, kw, pb.total_weight, pf.total_weight,
                   (pass && trees_ok) ? "OK" : "*** MISMATCH ***");

            prim_result_free(&pb);
            prim_result_free(&pf);
            graph_free(g);
        }
    }
    printf("Resultado: %d/%d casos correctos.\n\n", ok_cases, total_cases);

    printf("== Demo de timing binomial vs fibonacci ==\n");
    int demo_i[] = {10, 12, 14, 16, 18};
    for (int t = 0; t < 5; t++) {
        int n = 1 << demo_i[t];
        long long m = (long long)n * 8;
        long long max_m = (long long)n*(n-1)/2;
        if (m > max_m) m = max_m;

        Graph *g = graph_generate_random(n, m, 123 + t);

        double t0 = now_seconds();
        PrimResult pb = prim_run(g, 0, &BINOMIAL_PQ);
        double t1 = now_seconds();
        PrimResult pf = prim_run(g, 0, &FIBONACCI_PQ);
        double t2 = now_seconds();

        printf("  v=2^%-2d e=%9lld | binomial=%.4fs (dk=%lld,ops=%lld) | fibonacci=%.4fs (dk=%lld,ops=%lld) | pesos %s\n",
               demo_i[t], m,
               t1 - t0, pb.decrease_key_calls, pb.decrease_key_struct_ops,
               t2 - t1, pf.decrease_key_calls, pf.decrease_key_struct_ops,
               fabs(pb.total_weight - pf.total_weight) < 1e-6 ? "OK" : "*** DIFIEREN ***");

        prim_result_free(&pb);
        prim_result_free(&pf);
        graph_free(g);
    }

    printf("\nTamano de nodo interno: binomial=%zu bytes, fibonacci=%zu bytes\n",
           binomial_node_size(), fibonacci_node_size());

    return ok_cases == total_cases ? 0 : 1;
}
