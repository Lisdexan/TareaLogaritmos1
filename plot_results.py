#!/usr/bin/env python3
"""
Genera los 12 graficos del enunciado a partir de los CSV de run_experiments.

Uso:  python3 plot_results.py [carpeta_resultados] [carpeta_salida]

  6.3.1 (costo total, 4 graficos): {binomial, fibonacci} x {serie A, serie B}
        curva medida + cota teorica * constante
        binomial: e*log2(v)     fibonacci: e + v*log2(v)
  6.3.2 (costo amortizado, 8 graficos): {tiempo acumulado, conteo de ops}
        x {binomial, fibonacci} x {serie C, serie D}
        cota teorica: binomial k*log2(v) (peor caso por llamada), fibonacci k (O(1) amortizado)
        con k = numero de llamadas a decreaseKey.

La constante de cada cota se ajusta por minimos cuadrados (c = sum(y*f)/sum(f^2)).
Los dos graficos de una misma serie/medicion comparten escala en el eje y.
Justifica en el informe las constantes que salgan.
"""
import sys, os
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

res = sys.argv[1] if len(sys.argv) > 1 else "resultados"
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(res, "graficos")
os.makedirs(out, exist_ok=True)

QUEUES = ["binomial", "fibonacci"]
LABEL = {"binomial": "Cola binomial", "fibonacci": "Cola de Fibonacci"}


def bound_total(q, v, e):
    return e * np.log2(v) if q == "binomial" else e + v * np.log2(v)


def bound_total_label(q):
    return "c·e·log v" if q == "binomial" else "c·(e + v·log v)"


def fit(y, f):
    y, f = np.asarray(y, float), np.asarray(f, float)
    return float((y * f).sum() / (f * f).sum())


# ---------------- 6.3.1: costo total ----------------
summ_path = os.path.join(res, "costo_total_resumen.csv")
if os.path.exists(summ_path):
    S = pd.read_csv(summ_path)
    for serie, xcol, xlab in (("A", "e", "aristas e (v fijo)"), ("B", "v", "vértices v (e fijo)")):
        d = S[S.series == serie]
        if d.empty:
            continue
        ymax = 0
        curves = {}
        for q in QUEUES:
            dq = d[d.queue == q].sort_values(xcol)
            f = bound_total(q, dq.v.values, dq.e.values)
            c = fit(dq.avg_time_s.values, f)
            curves[q] = (dq[xcol].values, dq.avg_time_s.values, c * f, c)
            ymax = max(ymax, dq.avg_time_s.max(), (c * f).max())
        for q in QUEUES:
            x, y, th, c = curves[q]
            plt.figure(figsize=(6, 4))
            plt.plot(x, y, "o-", label=f"{LABEL[q]} (medido)")
            plt.plot(x, th, "s--", label=f"{bound_total_label(q)}, c={c:.3e}")
            plt.xscale("log", base=2)
            plt.ylim(0, ymax * 1.1)
            plt.xlabel(xlab)
            plt.ylabel("Tiempo total de Prim [s] (promedio)")
            plt.title(f"{LABEL[q]} — serie {serie}")
            plt.grid(alpha=0.3)
            plt.legend()
            plt.tight_layout()
            plt.savefig(os.path.join(out, f"total_{q}_serie{serie}.png"), dpi=150)
            plt.close()

# ---------------- 6.3.2: costo amortizado ----------------
am_path = os.path.join(res, "amortizado.csv")
if os.path.exists(am_path):
    A = pd.read_csv(am_path)
    for serie in ("C", "D"):
        d = A[A.series == serie]
        if d.empty:
            continue
        for metric, col, ylab, tag in (
            ("tiempo", "cum_time_s", "Tiempo acumulado en decreaseKey [s]", "tiempo"),
            ("ops", "cum_ops", "Operaciones estructurales acumuladas", "ops"),
        ):
            # curva promedio por configuracion (i,j): se interpola cada repeticion en una grilla comun
            data = {}
            ymax = 0
            for q in QUEUES:
                dq = d[d.queue == q]
                data[q] = []
                for (i, j), dc in dq.groupby(["i", "j"]):
                    v = int(dc.v.iloc[0])
                    kmax = min(g.call_idx.max() for _, g in dc.groupby("rep"))
                    grid = np.linspace(0, kmax, 200)
                    ys = []
                    for _, g in dc.groupby("rep"):
                        g = g.sort_values("call_idx")
                        ys.append(np.interp(grid, np.r_[0, g.call_idx.values], np.r_[0, g[col].values]))
                    ymean = np.mean(ys, axis=0)
                    data[q].append((i, j, v, grid, ymean))
                # cota teorica: binomial k*log2(v) ; fibonacci k
                fs = np.concatenate([(g_ * (np.log2(v_) if q == "binomial" else 1.0)) for (_, _, v_, g_, _) in data[q]])
                ys_all = np.concatenate([y_ for (_, _, _, _, y_) in data[q]])
                c = fit(ys_all, fs)
                data[q] = (data[q], c)
                ymax = max(ymax, ys_all.max())
            for q in QUEUES:
                curves, c = data[q]
                plt.figure(figsize=(6.5, 4.2))
                for idx, (i, j, v, grid, y) in enumerate(curves):
                    lab = f"(i={i}, j={j})"
                    p = plt.plot(grid, y, label=lab)
                    th = c * grid * (np.log2(v) if q == "binomial" else 1.0)
                    plt.plot(grid, th, "--", color=p[0].get_color(), alpha=0.6)
                cota = "c·k·log v" if q == "binomial" else "c·k"
                plt.plot([], [], "k--", label=f"cota {cota}, c={c:.3e}")
                plt.ylim(0, ymax * 1.1)
                plt.xlabel("Llamadas a decreaseKey (k)")
                plt.ylabel(ylab)
                plt.title(f"{LABEL[q]} — {metric} — serie {serie}")
                plt.grid(alpha=0.3)
                plt.legend(fontsize=7)
                plt.tight_layout()
                plt.savefig(os.path.join(out, f"amortizado_{tag}_{q}_serie{serie}.png"), dpi=150)
                plt.close()

print("Graficos escritos en", out)
