#ifndef PRIM_H
#define PRIM_H

#include "graph.h"
#include "pq.h"

typedef struct {
    int *parent;              /* parent[v]: predecesor en el MST, -1 si es la raiz */
    double *cost;             /* costo con el que v entro al arbol (peso de su arista) */
    double total_weight;      /* peso total del MST */
    long long decrease_key_calls;      /* cuantas veces se llamo a decreaseKey */
    long long decrease_key_struct_ops; /* suma de operaciones estructurales (swaps/cortes) */
} PrimResult;

/* Corre Prim sobre 'g' partiendo de 'root', usando la cola de prioridad
 * descrita por 'pqi' (BINOMIAL_PQ o FIBONACCI_PQ). */
PrimResult prim_run(const Graph *g, int root, const PQInterface *pqi);
void prim_result_free(PrimResult *r);

/* Muestras tomadas cada 'sample_every' llamadas a decreaseKey, para
 * graficar el costo amortizado (seccion 6.3.2): tiempo acumulado y
 * conteo acumulado de operaciones estructurales, contra la cantidad
 * de llamadas realizadas hasta ese punto. */
typedef struct {
    long long *call_idx;         /* cuantas llamadas a decreaseKey iban hasta esta muestra */
    double *cumulative_time;     /* suma de tiempo de TODAS las llamadas a decreaseKey hasta aca */
    long long *cumulative_ops;   /* suma de operaciones estructurales hasta aca */
    int n_samples;
} DKLog;

/* Igual que prim_run, pero ademas cronometra cada llamada individual a
 * decreaseKey (con clock_gettime) y guarda una muestra cada
 * 'sample_every' llamadas en 'log_out'. Solo usar esta version para la
 * seccion 6.3.2: el cronometrado por-llamada agrega overhead que no
 * queremos en la medicion de costo TOTAL (seccion 6.3.1), para eso se
 * usa prim_run tal cual. */
PrimResult prim_run_instrumented(const Graph *g, int root, const PQInterface *pqi,
                                  long long sample_every, DKLog *log_out);
void dklog_free(DKLog *log);

#endif
