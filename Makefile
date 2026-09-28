CC = gcc
CFLAGS = -O2 -Wall -Wextra -Iinclude
LDFLAGS = -lm

CORE_SRC = src/graph.c src/binomial_heap.c src/fibonacci_heap.c src/prim.c
CORE_OBJ = $(CORE_SRC:.c=.o)

all: run_experiments validate mem_estimate

run_experiments: $(CORE_OBJ) src/run_experiments.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

validate: $(CORE_OBJ) src/validate.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

mem_estimate: $(CORE_OBJ) src/mem_estimate.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CORE_OBJ) src/*.o run_experiments validate mem_estimate test_binomial

.PHONY: all clean
