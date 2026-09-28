#ifndef FIBONACCI_HEAP_H
#define FIBONACCI_HEAP_H

#include "pq.h"
#include <stddef.h>

extern const PQInterface FIBONACCI_PQ;

/* Tamano en bytes del nodo interno de la cola de Fibonacci (para la
 * estimacion de memoria de la seccion 6.2 del enunciado). */
size_t fibonacci_node_size(void);

#endif
