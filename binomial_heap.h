#ifndef BINOMIAL_HEAP_H
#define BINOMIAL_HEAP_H

#include "pq.h"
#include <stddef.h>

/* Tabla de funciones para usar la cola binomial a traves de la
 * interfaz generica PQInterface (ver pq.h). */
extern const PQInterface BINOMIAL_PQ;

/* Tamano en bytes del nodo interno de la cola binomial (para la
 * estimacion de memoria de la seccion 6.2 del enunciado). */
size_t binomial_node_size(void);

#endif
