#ifndef PQ_H
#define PQ_H

/*
 * Interfaz generica de cola de prioridad de pares (costo, nodo), pensada
 * exactamente para lo que pide la seccion 3.1 del enunciado: extractMin
 * y decreaseKey, con acceso directo al par de cada vertice (pos[]).
 *
 * Cada estructura concreta (cola binomial, cola de Fibonacci) implementa
 * este contrato. Prim se escribe UNA sola vez contra esta interfaz y no
 * sabe (ni le importa) que estructura hay detras.
 */
typedef struct PQ PQ; /* tipo opaco, cada implementacion define su struct real */

typedef struct {
    /* Construye la cola con 'n' elementos, usando costs[v] como llave
     * inicial de cada vertice v (típicamente INFINITY salvo la raiz).
     * Internamente se hace via inserciones sucesivas (seccion 3.4). */
    PQ *(*build)(int n, const double *costs);

    /* Extrae el par de menor costo. Devuelve el vertice extraido y dejar
     * su costo en *out_key. Devuelve -1 si la cola esta vacia. */
    int (*extract_min)(PQ *pq, double *out_key);

    /* Reduce la llave del vertice 'vtx' a 'newkey' (se asume newkey <=
     * llave actual, tal como exige el enunciado: nunca se aumenta).
     * Devuelve la cantidad de operaciones estructurales realizadas
     * (intercambios en binomial / cortes en cascada en Fibonacci),
     * para poder medir el costo amortizado en la seccion 6.3.2. */
    int (*decrease_key)(PQ *pq, int vtx, double newkey);

    int (*is_empty)(const PQ *pq);

    void (*destroy)(PQ *pq);

    const char *name; /* "binomial" | "fibonacci", para logs/CSV */
} PQInterface;

#endif
