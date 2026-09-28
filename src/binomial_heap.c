#include "binomial_heap.h"
#include <stdlib.h>

/*
 * Cola binomial estandar (CLRS cap. 19), especializada a pares
 * (costo, vertice) con acceso directo via pos[vertice].
 *
 * Decision de diseno para decreaseKey (documentar esto mismo en el
 * informe, seccion 4.2 del enunciado): en vez de mover el NODO hacia
 * la raiz, se intercambia el CONTENIDO (key, vtx) entre el nodo y su
 * padre en cada paso, tal como sugiere el enunciado en 3.2. Esto es
 * mas simple que religar punteros del arbol, pero exige mantener
 * pos[] sincronizado en cada intercambio: despues de intercambiar,
 * cada nodo representa a un vertice distinto del que representaba
 * antes, asi que pos[] de AMBOS vertices involucrados se actualiza
 * en el mismo paso.
 */

typedef struct BNode {
    double key;
    int vtx;
    int degree;
    struct BNode *parent, *child, *sibling;
} BNode;

struct PQ {
    BNode *head;  /* lista de raices, ordenada por grado ascendente */
    BNode **pos;  /* pos[v]: nodo que actualmente representa a v (NULL si ya se extrajo) */
};

static BNode *new_node(int vtx, double key) {
    BNode *x = malloc(sizeof(BNode));
    x->key = key; x->vtx = vtx; x->degree = 0;
    x->parent = x->child = x->sibling = NULL;
    return x;
}

/* liga y bajo z, asumiendo y.key >= z.key: y pasa a ser hijo de z. */
static void bh_link(BNode *y, BNode *z) {
    y->parent = z;
    y->sibling = z->child;
    z->child = y;
    z->degree++;
}

/* fusiona dos listas de raices ordenadas por grado (merge de listas
 * ordenadas); puede dejar grados repetidos, eso lo arregla consolidate. */
static BNode *merge_root_lists(BNode *h1, BNode *h2) {
    BNode dummy; dummy.sibling = NULL;
    BNode *tail = &dummy;
    while (h1 && h2) {
        if (h1->degree <= h2->degree) { tail->sibling = h1; h1 = h1->sibling; }
        else { tail->sibling = h2; h2 = h2->sibling; }
        tail = tail->sibling;
    }
    tail->sibling = h1 ? h1 : h2;
    return dummy.sibling;
}

/* dada una lista ya fusionada (ordenada por grado, con a lo mas dos
 * arboles por grado), la deja con a lo mas UN arbol por grado. */
static BNode *consolidate(BNode *head) {
    if (!head) return NULL;
    BNode *prev = NULL, *cur = head, *next = cur->sibling;
    while (next) {
        if (cur->degree != next->degree ||
            (next->sibling && next->sibling->degree == cur->degree)) {
            prev = cur;
            cur = next;
        } else if (cur->key <= next->key) {
            cur->sibling = next->sibling;
            bh_link(next, cur);
        } else {
            if (prev) prev->sibling = next; else head = next;
            bh_link(cur, next);
            cur = next;
        }
        next = cur->sibling;
    }
    return head;
}

static BNode *bh_union_lists(BNode *h1, BNode *h2) {
    return consolidate(merge_root_lists(h1, h2));
}

static BNode *find_min_root(BNode *head, BNode **prev_out) {
    if (!head) { if (prev_out) *prev_out = NULL; return NULL; }
    BNode *best = head, *best_prev = NULL;
    BNode *prev = head, *cur = head->sibling;
    while (cur) {
        if (cur->key < best->key) { best = cur; best_prev = prev; }
        prev = cur; cur = cur->sibling;
    }
    if (prev_out) *prev_out = best_prev;
    return best;
}

static PQ *bh_build(int n, const double *costs) {
    PQ *h = malloc(sizeof(struct PQ));
    h->head = NULL;
    h->pos = malloc(sizeof(BNode *) * (size_t)n);
    /* construccion por inserciones sucesivas (seccion 3.4 del enunciado) */
    for (int v = 0; v < n; v++) {
        BNode *x = new_node(v, costs[v]);
        h->pos[v] = x;
        h->head = bh_union_lists(h->head, x);
    }
    return h;
}

static int bh_is_empty(const PQ *h) { return h->head == NULL; }

static int bh_extract_min(PQ *h, double *out_key) {
    if (!h->head) return -1;
    BNode *prev_min;
    BNode *min_node = find_min_root(h->head, &prev_min);

    if (prev_min) prev_min->sibling = min_node->sibling;
    else h->head = min_node->sibling;

    /* los hijos de min_node (ordenados por grado descendente) se
     * revierten para quedar ascendentes, tal como exige merge_root_lists */
    BNode *child_list = NULL;
    BNode *c = min_node->child;
    while (c) {
        BNode *next = c->sibling;
        c->sibling = child_list;
        c->parent = NULL;
        child_list = c;
        c = next;
    }

    h->head = bh_union_lists(h->head, child_list);

    int vtx = min_node->vtx;
    *out_key = min_node->key;
    h->pos[vtx] = NULL;
    free(min_node);
    return vtx;
}

static int bh_decrease_key(PQ *h, int vtx, double newkey) {
    BNode *x = h->pos[vtx];
    x->key = newkey;
    int swaps = 0;
    BNode *y = x->parent;
    while (y && x->key < y->key) {
        double tk = x->key; x->key = y->key; y->key = tk;
        int tv = x->vtx; x->vtx = y->vtx; y->vtx = tv;
        h->pos[x->vtx] = x;
        h->pos[y->vtx] = y;
        x = y;
        y = x->parent;
        swaps++;
    }
    return swaps;
}

static void free_tree(BNode *x) {
    if (!x) return;
    free_tree(x->child);
    free_tree(x->sibling);
    free(x);
}

static void bh_destroy(PQ *h) {
    free_tree(h->head);
    free(h->pos);
    free(h);
}

size_t binomial_node_size(void) { return sizeof(BNode); }

const PQInterface BINOMIAL_PQ = {
    .build = bh_build,
    .extract_min = bh_extract_min,
    .decrease_key = bh_decrease_key,
    .is_empty = bh_is_empty,
    .destroy = bh_destroy,
    .name = "binomial"
};
