#include "prim.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

PrimResult prim_run(const Graph *g, int root, const PQInterface *pqi) {
    int n = g->n;
    double *costs = malloc(sizeof(double) * (size_t)n);
    int *parent = malloc(sizeof(int) * (size_t)n);
    char *in_queue = malloc((size_t)n);

    for (int v = 0; v < n; v++) {
        costs[v] = INFINITY;
        parent[v] = -1;
        in_queue[v] = 1;
    }
    costs[root] = 0.0;

    PQ *Q = pqi->build(n, costs);

    double total = 0.0;
    long long dk_calls = 0, dk_ops = 0;

    while (!pqi->is_empty(Q)) {
        double c;
        int v = pqi->extract_min(Q, &c);
        in_queue[v] = 0;
        if (v != root) total += c;

        for (int k = g->head[v]; k < g->head[v + 1]; k++) {
            int u = g->to[k];
            double w = g->weight[k];
            if (in_queue[u] && w < costs[u]) {
                costs[u] = w;
                parent[u] = v;
                dk_ops += pqi->decrease_key(Q, u, w);
                dk_calls++;
            }
        }
    }

    pqi->destroy(Q);
    free(in_queue);

    PrimResult res;
    res.parent = parent;
    res.cost = costs;
    res.total_weight = total;
    res.decrease_key_calls = dk_calls;
    res.decrease_key_struct_ops = dk_ops;
    return res;
}

void prim_result_free(PrimResult *r) {
    free(r->parent);
    free(r->cost);
}

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

PrimResult prim_run_instrumented(const Graph *g, int root, const PQInterface *pqi,
                                  long long sample_every, DKLog *log_out) {
    if (sample_every < 1) sample_every = 1;
    int n = g->n;
    double *costs = malloc(sizeof(double) * (size_t)n);
    int *parent = malloc(sizeof(int) * (size_t)n);
    char *in_queue = malloc((size_t)n);

    for (int v = 0; v < n; v++) {
        costs[v] = INFINITY;
        parent[v] = -1;
        in_queue[v] = 1;
    }
    costs[root] = 0.0;

    PQ *Q = pqi->build(n, costs);

    long long max_samples = g->m / sample_every + 2;
    log_out->call_idx = malloc(sizeof(long long) * (size_t)max_samples);
    log_out->cumulative_time = malloc(sizeof(double) * (size_t)max_samples);
    log_out->cumulative_ops = malloc(sizeof(long long) * (size_t)max_samples);
    log_out->n_samples = 0;

    double total = 0.0;
    long long dk_calls = 0, dk_ops = 0;
    double dk_time_sum = 0.0;

    while (!pqi->is_empty(Q)) {
        double c;
        int v = pqi->extract_min(Q, &c);
        in_queue[v] = 0;
        if (v != root) total += c;

        for (int k = g->head[v]; k < g->head[v + 1]; k++) {
            int u = g->to[k];
            double w = g->weight[k];
            if (in_queue[u] && w < costs[u]) {
                costs[u] = w;
                parent[u] = v;

                double t0 = now_seconds();
                int ops = pqi->decrease_key(Q, u, w);
                double t1 = now_seconds();

                dk_time_sum += (t1 - t0);
                dk_ops += ops;
                dk_calls++;

                if (dk_calls % sample_every == 0) {
                    long long idx = log_out->n_samples++;
                    log_out->call_idx[idx] = dk_calls;
                    log_out->cumulative_time[idx] = dk_time_sum;
                    log_out->cumulative_ops[idx] = dk_ops;
                }
            }
        }
    }
    /* asegurar que el ultimo punto quede registrado */
    if (dk_calls > 0 &&
        (log_out->n_samples == 0 || log_out->call_idx[log_out->n_samples - 1] != dk_calls)) {
        long long idx = log_out->n_samples++;
        log_out->call_idx[idx] = dk_calls;
        log_out->cumulative_time[idx] = dk_time_sum;
        log_out->cumulative_ops[idx] = dk_ops;
    }

    pqi->destroy(Q);
    free(in_queue);

    PrimResult res;
    res.parent = parent;
    res.cost = costs;
    res.total_weight = total;
    res.decrease_key_calls = dk_calls;
    res.decrease_key_struct_ops = dk_ops;
    return res;
}

void dklog_free(DKLog *log) {
    free(log->call_idx);
    free(log->cumulative_time);
    free(log->cumulative_ops);
}
