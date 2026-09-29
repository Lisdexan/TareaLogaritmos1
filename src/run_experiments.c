/*
 * Harness de experimentos (secciones 6.3.1 y 6.3.2 del enunciado).
 *
 * Uso:
 *   ./run_experiments                 corre TODO con los tamanos reales del enunciado
 *   ./run_experiments --demo          version reducida (exponentes -4, 3 repeticiones)
 *   ./run_experiments --shift K       resta K a todos los exponentes (i, j)
 *   ./run_experiments --reps N        repeticiones por configuracion (defecto 10)
 *   ./run_experiments --series ABCD   solo algunas series (defecto ABCD)
 *   ./run_experiments --seed S        semilla base (defecto 12345)
 *   ./run_experiments --out DIR       carpeta de salida de los CSV (defecto ./resultados)
 *
 * Series (exactamente las del enunciado):
 *   A: i=20, j in {20..24}     B: j=24, i in {18..22}       (costo total)
 *   C: i=18, j in {18..22}     D: j=22, i in {14..18}       (costo amortizado)
 *
 * Salidas (en DIR):
 *   costo_total_raw.csv      una fila por (serie, cola, i, j, rep)
 *   costo_total_resumen.csv  promedio de las repeticiones por configuracion
 *   amortizado.csv           curvas de decreaseKey: llamadas vs tiempo/ops acumulados
 *
 * La generacion del grafo NO entra en la medicion. Series A/B usan
 * prim_run sin instrumentar; C/D usan prim_run_instrumented.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

#include "graph.h"
#include "binomial_heap.h"
#include "fibonacci_heap.h"
#include "prim.h"

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

typedef struct { char series; int i, j; } Config;

static int build_configs(const char *series, int shift, Config *out) {
    int n = 0;
    for (const char *s = series; *s; s++) {
        if (*s == 'A') for (int j = 20; j <= 24; j++) out[n++] = (Config){'A', 20 - shift, j - shift};
        if (*s == 'B') for (int i = 18; i <= 22; i++) out[n++] = (Config){'B', i - shift, 24 - shift};
        if (*s == 'C') for (int j = 18; j <= 22; j++) out[n++] = (Config){'C', 18 - shift, j - shift};
        if (*s == 'D') for (int i = 14; i <= 18; i++) out[n++] = (Config){'D', i - shift, 22 - shift};
    }
    return n;
}

int main(int argc, char **argv) {
    int shift = 0, reps = 10;
    unsigned long long base_seed = 12345;
    const char *series = "ABCD";
    const char *outdir = "resultados";

    for (int a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "--demo")) { shift = 4; reps = 3; }
        else if (!strcmp(argv[a], "--shift") && a + 1 < argc) shift = atoi(argv[++a]);
        else if (!strcmp(argv[a], "--reps") && a + 1 < argc) reps = atoi(argv[++a]);
        else if (!strcmp(argv[a], "--series") && a + 1 < argc) series = argv[++a];
        else if (!strcmp(argv[a], "--seed") && a + 1 < argc) base_seed = strtoull(argv[++a], NULL, 10);
        else if (!strcmp(argv[a], "--out") && a + 1 < argc) outdir = argv[++a];
        else { fprintf(stderr, "argumento desconocido: %s\n", argv[a]); return 1; }
    }

    mkdir(outdir, 0755);
    char path[512];
    snprintf(path, sizeof path, "%s/costo_total_raw.csv", outdir);
    FILE *f_raw = fopen(path, "w");
    snprintf(path, sizeof path, "%s/costo_total_resumen.csv", outdir);
    FILE *f_sum = fopen(path, "w");
    snprintf(path, sizeof path, "%s/amortizado.csv", outdir);
    FILE *f_am = fopen(path, "w");
    if (!f_raw || !f_sum || !f_am) { perror("no pude abrir CSVs de salida"); return 1; }

    fprintf(f_raw, "series,queue,i,j,v,e,rep,time_s,mst_weight,dk_calls,dk_struct_ops\n");
    fprintf(f_sum, "series,queue,i,j,v,e,avg_time_s,reps\n");
    fprintf(f_am, "series,queue,i,j,v,e,rep,call_idx,cum_time_s,cum_ops\n");

    Config cfgs[64];
    int ncfg = build_configs(series, shift, cfgs);
    const PQInterface *queues[2] = { &BINOMIAL_PQ, &FIBONACCI_PQ };

    int checks = 0, mismatches = 0;

    for (int c = 0; c < ncfg; c++) {
        Config cf = cfgs[c];
        int n = 1 << cf.i;
        long long m_req = 1LL << cf.j;
        int amortized = (cf.series == 'C' || cf.series == 'D');
        double sum_t[2] = {0, 0};
        long long m_real = 0;

        printf("[serie %c] i=%d j=%d (v=%d, e=%lld) x%d reps\n", cf.series, cf.i, cf.j, n, m_req, reps);
        fflush(stdout);

        for (int r = 0; r < reps; r++) {
            uint64_t seed = base_seed + 1000003ULL * (uint64_t)r + 7919ULL * (uint64_t)c;
            Graph *g = graph_generate_random(n, m_req, seed);
            m_real = g->m;
            double w[2] = {0, 0};

            for (int q = 0; q < 2; q++) {
                PrimResult res;
                double dt;
                if (!amortized) {
                    double t0 = now_seconds();
                    res = prim_run(g, 0, queues[q]);
                    dt = now_seconds() - t0;
                } else {
                    DKLog lg;
                    long long every = g->m / 4000; if (every < 1) every = 1;
                    double t0 = now_seconds();
                    res = prim_run_instrumented(g, 0, queues[q], every, &lg);
                    dt = now_seconds() - t0;
                    for (int s = 0; s < lg.n_samples; s++)
                        fprintf(f_am, "%c,%s,%d,%d,%d,%lld,%d,%lld,%.9f,%lld\n",
                                cf.series, queues[q]->name, cf.i, cf.j, n, g->m, r,
                                lg.call_idx[s], lg.cumulative_time[s], lg.cumulative_ops[s]);
                    dklog_free(&lg);
                }
                w[q] = res.total_weight;
                sum_t[q] += dt;
                fprintf(f_raw, "%c,%s,%d,%d,%d,%lld,%d,%.9f,%.12f,%lld,%lld\n",
                        cf.series, queues[q]->name, cf.i, cf.j, n, g->m, r, dt,
                        res.total_weight, res.decrease_key_calls, res.decrease_key_struct_ops);
                prim_result_free(&res);
            }

            checks++;
            if (fabs(w[0] - w[1]) > 1e-6) {
                mismatches++;
                fprintf(stderr, "  !! pesos distintos (binomial=%.9f fibonacci=%.9f) en rep %d\n", w[0], w[1], r);
            }
            graph_free(g);
        }
        for (int q = 0; q < 2; q++)
            fprintf(f_sum, "%c,%s,%d,%d,%d,%lld,%.9f,%d\n", cf.series, queues[q]->name,
                    cf.i, cf.j, n, m_real, sum_t[q] / reps, reps);
        printf("    promedio binomial=%.4fs  fibonacci=%.4fs\n", sum_t[0] / reps, sum_t[1] / reps);
        fflush(f_raw); fflush(f_sum); fflush(f_am);
    }

    printf("\nVerificacion de pesos de MST (binomial vs fibonacci): %d/%d coinciden\n",
           checks - mismatches, checks);
    fclose(f_raw); fclose(f_sum); fclose(f_am);
    return mismatches ? 2 : 0;
}
