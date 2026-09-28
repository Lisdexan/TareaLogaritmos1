# CC4102 – Tarea 1

## Requisitos
- Máquina virtual con Linux (Debian 13), 2 GB de RAM minimo.
- Compilador y make: `sudo apt install build-essential`
- Python 3 y librerías para los gráficos:
  `sudo apt install python3-numpy python3-pandas python3-matplotlib`

## Ejecución
Dentro de la carpeta de la tarea:

1. `make` — compila el programa de experimentos.
2. `./run_experiments` — corre las series A, B, C y D. Tarda 7 minutos aprox. Luego crea una carpeta `resultados/` y escribe los CSV.
3. `python3 plot_results.py resultados` — genera los 12 gráficos en `resultados/graficos/`.

Al final, `run_experiments` imprime cuántos pesos de MST coinciden entre
ambas colas

## Archivos de salida
- `resultados/costo_total_raw.csv`, `costo_total_resumen.csv`: tiempos.
- `resultados/amortizado.csv`.
- `resultados/graficos/*.png`: gráficos.
