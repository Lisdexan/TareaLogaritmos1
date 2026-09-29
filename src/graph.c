#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------
 * PRNG: xorshift64* (rapido, buena calidad, sin dependencias externas).
 * rand()/random() de la libc son notoriamente lentos y de baja calidad
 * en los bits bajos para generar decenas de millones de valores.
 * ------------------------------------------------------------------- */
typedef struct { uint64_t s; } Rng;

static void rng_seed(Rng *r, uint64_t seed) {
    /* evitar estado 0 */
    r->s = seed ? seed : 0x9E3779B97F4A7C15ULL;
}

static uint64_t rng_next(Rng *r) {
    uint64_t x = r->s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    r->s = x;
    return x * 0x2545F4914F6CDD1DULL;
}

/* entero uniforme en [0, bound) sin sesgo notorio (Lemire-ish, suficiente aca) */
static uint64_t rng_below(Rng *r, uint64_t bound) {
    if (bound == 0) return 0;
    uint64_t x, r_;
    uint64_t threshold = (-bound) % bound;
    do {
        x = rng_next(r);
        r_ = x % bound;
    } while (x - r_ > (uint64_t)(-1) - threshold + 1 && 0); /* practicamente nunca reintenta; ver nota abajo */
    return r_;
}

/* peso aleatorio en (0, 1] */
static double rng_weight(Rng *r) {
    uint64_t x = rng_next(r);
    /* 53 bits de mantisa -> double en [0,1) uniforme, luego lo llevamos a (0,1] */
    double v = (double)(x >> 11) * (1.0 / 9007199254740992.0); /* 2^53 */
    return 1.0 - v; /* si v=0 -> 1.0 (extremo incluido), nunca da 0 */
}

/* ---------------------------------------------------------------------
 * Hash set de pares (u,v) para descartar aristas repetidas al generar
 * el grafo aleatorio. Open addressing con probing lineal.
 * Clave canonica: key = (int64) min(u,v) * n + max(u,v).
 * ------------------------------------------------------------------- */
typedef struct {
    int64_t *slots; /* -1 = vacio */
    uint64_t cap;   /* potencia de 2 */
    uint64_t count;
} EdgeSet;

static uint64_t mix64(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static void eset_init(EdgeSet *s, uint64_t expected) {
    uint64_t cap = 16;
    while (cap < expected * 2) cap <<= 1;
    s->slots = malloc(cap * sizeof(int64_t));
    for (uint64_t i = 0; i < cap; i++) s->slots[i] = -1;
    s->cap = cap;
    s->count = 0;
}

static void eset_free(EdgeSet *s) { free(s->slots); }

/* devuelve 1 si insertada (era nueva), 0 si ya existia */
static int eset_insert(EdgeSet *s, int64_t key) {
    uint64_t idx = mix64((uint64_t)key) & (s->cap - 1);
    while (s->slots[idx] != -1) {
        if (s->slots[idx] == key) return 0;
        idx = (idx + 1) & (s->cap - 1);
    }
    s->slots[idx] = key;
    s->count++;
    return 1;
}

/* ---------------------------------------------------------------------
 * Generacion del grafo
 * ------------------------------------------------------------------- */
typedef struct { int u, v; double w; } EdgeRec;

Graph *graph_generate_random(int n, long long m, uint64_t seed) {
    if (n <= 0) return NULL;
    long long max_edges = (long long)n * (n - 1) / 2;
    if (m < n - 1) m = n - 1; /* minimo para ser conexo */
    if (m > max_edges) m = max_edges;

    Rng rng; rng_seed(&rng, seed);
    EdgeRec *edges = malloc(sizeof(EdgeRec) * m);
    EdgeSet eset; eset_init(&eset, (uint64_t)m);

    long long idx = 0;

    /* 1) arbol cobertor aleatorio: vertice i -> vecino aleatorio en [0,i-1] */
    for (int i = 1; i < n; i++) {
        int j = (int)rng_below(&rng, (uint64_t)i);
        int64_t key = (int64_t)(j < i ? j : i) * n + (j < i ? i : j);
        eset_insert(&eset, key); /* siempre nueva (arbol no tiene repetidos) */
        double w = rng_weight(&rng);
        edges[idx].u = j; edges[idx].v = i; edges[idx].w = w;
        idx++;
    }

    /* 2) aristas extra aleatorias, descartando repetidas/reflexivas */
    while (idx < m) {
        int u = (int)rng_below(&rng, (uint64_t)n);
        int v = (int)rng_below(&rng, (uint64_t)n);
        if (u == v) continue;
        int a = u < v ? u : v, b = u < v ? v : u;
        int64_t key = (int64_t)a * n + b;
        if (!eset_insert(&eset, key)) continue; /* ya existia */
        double w = rng_weight(&rng);
        edges[idx].u = a; edges[idx].v = b; edges[idx].w = w;
        idx++;
    }
    eset_free(&eset);

    /* 3) construir CSR a partir de la lista de aristas */
    Graph *g = malloc(sizeof(Graph));
    g->n = n;
    g->m = m;
    g->head = calloc(n + 1, sizeof(int));
    g->to = malloc(sizeof(int) * 2 * m);
    g->weight = malloc(sizeof(double) * 2 * m);

    /* contar grados */
    for (long long i = 0; i < m; i++) {
        g->head[edges[i].u + 1]++;
        g->head[edges[i].v + 1]++;
    }
    for (int i = 0; i < n; i++) g->head[i + 1] += g->head[i];

    /* rellenar usando un arreglo de cursores (copia de head) */
    int *cursor = malloc(sizeof(int) * n);
    memcpy(cursor, g->head, sizeof(int) * n);
    for (long long i = 0; i < m; i++) {
        int u = edges[i].u, v = edges[i].v;
        double w = edges[i].w;
        g->to[cursor[u]] = v; g->weight[cursor[u]] = w; cursor[u]++;
        g->to[cursor[v]] = u; g->weight[cursor[v]] = w; cursor[v]++;
    }
    free(cursor);
    free(edges);
    return g;
}

void graph_free(Graph *g) {
    if (!g) return;
    free(g->head); free(g->to); free(g->weight);
    free(g);
}

long long graph_memory_bytes(const Graph *g) {
    long long b = 0;
    b += (long long)(g->n + 1) * sizeof(int);       /* head */
    b += (long long)(2 * g->m) * sizeof(int);        /* to */
    b += (long long)(2 * g->m) * sizeof(double);     /* weight */
    return b;
}

int graph_save(const Graph *g, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    fprintf(f, "%d %lld\n", g->n, g->m);
    for (int u = 0; u < g->n; u++) {
        for (int k = g->head[u]; k < g->head[u + 1]; k++) {
            int v = g->to[k];
            if (v > u) /* imprimir cada arista una sola vez */
                fprintf(f, "%d %d %.17g\n", u, v, g->weight[k]);
        }
    }
    fclose(f);
    return 0;
}

Graph *graph_load(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;
    int n; long long m;
    if (fscanf(f, "%d %lld", &n, &m) != 2) { fclose(f); return NULL; }

    EdgeRec *edges = malloc(sizeof(EdgeRec) * m);
    for (long long i = 0; i < m; i++) {
        if (fscanf(f, "%d %d %lf", &edges[i].u, &edges[i].v, &edges[i].w) != 3) {
            free(edges); fclose(f); return NULL;
        }
    }
    fclose(f);

    Graph *g = malloc(sizeof(Graph));
    g->n = n; g->m = m;
    g->head = calloc(n + 1, sizeof(int));
    g->to = malloc(sizeof(int) * 2 * m);
    g->weight = malloc(sizeof(double) * 2 * m);
    for (long long i = 0; i < m; i++) {
        g->head[edges[i].u + 1]++;
        g->head[edges[i].v + 1]++;
    }
    for (int i = 0; i < n; i++) g->head[i + 1] += g->head[i];
    int *cursor = malloc(sizeof(int) * n);
    memcpy(cursor, g->head, sizeof(int) * n);
    for (long long i = 0; i < m; i++) {
        int u = edges[i].u, v = edges[i].v; double w = edges[i].w;
        g->to[cursor[u]] = v; g->weight[cursor[u]] = w; cursor[u]++;
        g->to[cursor[v]] = u; g->weight[cursor[v]] = w; cursor[v]++;
    }
    free(cursor);
    free(edges);
    return g;
}
