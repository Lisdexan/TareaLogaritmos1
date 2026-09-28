#include "fibonacci_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/*
 * Cola de Fibonacci estandar (CLRS cap. 19), especializada a pares
 * (costo, vertice) con acceso directo via pos[vertice].
 *
 * Todas las listas de hermanos (raices y listas de hijos) son listas
 * circulares doblemente enlazadas via left/right, lo que permite
 * insertar/cortar un nodo en O(1).
 *
 * decreaseKey (seccion 3.3 del enunciado): se corta x de su padre
 * directamente (sin subirlo) y se aplica corte en cascada hacia arriba
 * segun la marca de cada ancestro. Se devuelve la cantidad de cortes
 * realizados (1 por el corte de x, mas uno por cada corte en cascada),
 * pensado para la medicion de la seccion 6.3.2.
 */

typedef struct FNode {
    double key;
    int vtx;
    int degree;
    int mark;
    struct FNode *parent, *child, *left, *right;
} FNode;

struct PQ {
    FNode *min;          /* nodo minimo, tambien parte de la lista de raices */
    FNode **pos;         /* pos[v]: nodo que representa a v (NULL si ya se extrajo) */
    FNode **degree_table; /* buffer reusable para consolidate, tamano deg_cap */
    int deg_cap;
};

static FNode *fnode_new(int vtx, double key) {
    FNode *x = malloc(sizeof(FNode));
    x->key = key; x->vtx = vtx; x->degree = 0; x->mark = 0;
    x->parent = NULL; x->child = NULL;
    x->left = x->right = x;
    return x;
}

/* Inserta la lista circular 'b' (puede ser un nodo suelto, left=right=b)
 * justo despues de 'a' dentro de la lista de 'a'. No devuelve nada:
 * 'a' sigue siendo un representante valido de la lista fusionada. */
static void list_splice_after(FNode *a, FNode *b) {
    if (!a || !b) return;
    FNode *a_next = a->right;
    FNode *b_last = b->left;
    a->right = b;
    b->left = a;
    b_last->right = a_next;
    a_next->left = b_last;
}

static void fib_insert_node(PQ *h, FNode *x) {
    x->left = x->right = x;
    if (!h->min) {
        h->min = x;
    } else {
        list_splice_after(h->min, x);
        if (x->key < h->min->key) h->min = x;
    }
}

static PQ *fib_build(int n, const double *costs) {
    PQ *h = malloc(sizeof(struct PQ));
    h->min = NULL;
    h->pos = malloc(sizeof(FNode *) * (size_t)n);

    double logn = log2((double)(n < 2 ? 2 : n));
    h->deg_cap = (int)(2.5 * logn) + 16; /* cota real es ~1.4405*log2(n); dejamos harto margen */
    h->degree_table = malloc(sizeof(FNode *) * (size_t)h->deg_cap);

    for (int v = 0; v < n; v++) {
        FNode *x = fnode_new(v, costs[v]);
        h->pos[v] = x;
        fib_insert_node(h, x);
    }
    return h;
}

static int fib_is_empty(const PQ *h) { return h->min == NULL; }

static void consolidate(PQ *h) {
    /* IMPORTANTE: primero se recolectan los nodos raiz actuales en un
     * arreglo FIJO, antes de tocar nada. No basta con ir capturando
     * "el siguiente" sobre la marcha (como en la cola binomial): en
     * Fibonacci, un mismo nodo puede fusionarse en cascada con raices
     * que estan arbitrariamente mas adelante en la lista (no solo la
     * adyacente), asi que un puntero "next" capturado antes de esa
     * cascada puede terminar apuntando a un nodo que, momentos
     * despues, ya fue absorbido como hijo de otro -- causando que el
     * recorrido lo revisite como si aun fuera raiz. Con un arreglo de
     * indices fijos esto no pasa: por construccion, cuando se procesa
     * roots[i], solo pueden haberse consumido nodos de indices < i
     * (los que ya quedaron guardados en A[]), nunca uno de indice
     * mayor que todavia no fue visitado. */
    int count = 0;
    FNode *w = h->min, *start = h->min;
    do { count++; w = w->right; } while (w != start);

    FNode **roots = malloc(sizeof(FNode *) * (size_t)count);
    w = start;
    for (int i = 0; i < count; i++) { roots[i] = w; w = w->right; }

    int D = h->deg_cap;
    FNode **A = h->degree_table;
    for (int i = 0; i < D; i++) A[i] = NULL;

    for (int i = 0; i < count; i++) {
        FNode *x = roots[i];
        int d = x->degree;
        while (d < D && A[d] != NULL) {
            FNode *y = A[d];
            if (x->key > y->key) { FNode *t = x; x = y; y = t; }
            /* y pasa a ser hijo de x: primero se saca de la lista de raices
             * y se AISLA (left=right=y), porque list_splice_after asume que
             * el segundo argumento es una lista de un solo nodo; si y
             * conservara sus punteros viejos, el splice arrastraria a sus
             * antiguos vecinos de la lista de raices dentro de la lista
             * de hijos de x. */
            y->left->right = y->right;
            y->right->left = y->left;
            y->left = y->right = y;
            y->parent = x;
            y->mark = 0;
            if (!x->child) x->child = y;
            else list_splice_after(x->child, y);
            x->degree++;
            A[d] = NULL;
            d++;
        }
        if (d >= D) { fprintf(stderr, "fibonacci: cota de grado excedida, aumentar deg_cap\n"); exit(1); }
        A[d] = x;
    }
    free(roots);

    h->min = NULL;
    for (int i = 0; i < D; i++) {
        if (!A[i]) continue;
        A[i]->left = A[i]->right = A[i];
        if (!h->min) h->min = A[i];
        else {
            list_splice_after(h->min, A[i]);
            if (A[i]->key < h->min->key) h->min = A[i];
        }
    }
}

static int fib_extract_min(PQ *h, double *out_key) {
    FNode *z = h->min;
    if (!z) return -1;

    if (z->child) {
        FNode *c = z->child, *start = c;
        do { c->parent = NULL; c = c->right; } while (c != start);
        list_splice_after(z, z->child);
    }

    FNode *z_right = z->right;
    z->left->right = z->right;
    z->right->left = z->left;

    int vtx = z->vtx;
    *out_key = z->key;

    if (z == z_right) {
        h->min = NULL;
    } else {
        h->min = z_right;
        consolidate(h);
    }

    h->pos[vtx] = NULL;
    free(z);
    return vtx;
}

/* corta x de su padre y (asume y != NULL) y lo deja como raiz */
static void fib_cut(PQ *h, FNode *x, FNode *y) {
    if (y->child == x) {
        y->child = (x->right == x) ? NULL : x->right;
    }
    x->left->right = x->right;
    x->right->left = x->left;
    y->degree--;

    x->left = x->right = x;
    x->parent = NULL;
    x->mark = 0;
    list_splice_after(h->min, x);
    if (x->key < h->min->key) h->min = x;
}

/* corte en cascada iterativo (evita recursion profunda en casos adversos) */
static int fib_cascading_cut(PQ *h, FNode *y) {
    int cuts = 0;
    FNode *z = y->parent;
    while (z) {
        if (!y->mark) {
            y->mark = 1;
            break;
        }
        fib_cut(h, y, z);
        cuts++;
        y = z;
        z = y->parent;
    }
    return cuts;
}

static int fib_decrease_key(PQ *h, int vtx, double newkey) {
    FNode *x = h->pos[vtx];
    x->key = newkey;
    int ops = 0;
    FNode *y = x->parent;
    if (y && x->key < y->key) {
        fib_cut(h, x, y);
        ops++;
        ops += fib_cascading_cut(h, y);
    }
    if (x->key < h->min->key) h->min = x;
    return ops;
}

static void free_subtree_recursive(FNode *x) {
    if (x->child) {
        FNode *c = x->child, *start = c;
        FNode *cur = c;
        do {
            FNode *next = cur->right;
            free_subtree_recursive(cur);
            cur = next;
        } while (cur != start);
    }
    free(x);
}

static void fib_destroy(PQ *h) {
    if (h->min) {
        FNode *start = h->min, *cur = start;
        do {
            FNode *next = cur->right;
            free_subtree_recursive(cur);
            cur = next;
        } while (cur != start);
    }
    free(h->degree_table);
    free(h->pos);
    free(h);
}

size_t fibonacci_node_size(void) { return sizeof(FNode); }

const PQInterface FIBONACCI_PQ = {
    .build = fib_build,
    .extract_min = fib_extract_min,
    .decrease_key = fib_decrease_key,
    .is_empty = fib_is_empty,
    .destroy = fib_destroy,
    .name = "fibonacci"
};
