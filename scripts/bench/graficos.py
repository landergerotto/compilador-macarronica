#!/usr/bin/env python3
"""Tabelas e gráficos da Parte C, a partir dos CSV gravados por medir.py.

Lê <saida>/execucao.csv, <saida>/compilacao.csv e <saida>/maquina.txt e grava
na mesma pasta:
  resultados.md               todas as tabelas (a versão em texto dos gráficos)
  speedup_configuracoes.png   speedup de C1-escalar, C1, C2 e C3 sobre C0
  escalabilidade_threads.png  speedup de C3 com 1, 2, 4 e 8 threads
  pergunta_reducao_flut.png   tempo e erro das reduções de flut por variante
  tempo_compilacao.png        tempo de compilação por etapa e configuração

Uso: python3 graficos.py --saida docs/bench
"""

from __future__ import annotations

import argparse
import csv
import re
import statistics
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.lines import Line2D  # noqa: E402
from matplotlib.patches import Patch  # noqa: E402
from matplotlib.ticker import FixedLocator, FuncFormatter, NullLocator  # noqa: E402

NUCLEOS = ("soma", "somavet", "prodesc")
TIPOS = ("i64", "f32")
NOME_TIPO = {"i64": "i64 (inteira)", "f32": "f32 (flut)"}
THREADS = (1, 2, 4, 8)
CONFIGS_PRINCIPAIS = ("C0", "C1-escalar", "C1", "C2", "C3")
# Variantes da pergunta obrigatória, na ordem em que aparecem na tabela.
CONFIGS_PERGUNTA = ("C0", "C1", "C1-ffast-math", "C1-ordenada", "C-O2",
                    "C1-reassoc", "C-ffast-math", "C2", "C3")

# Paleta de referência (validada com validate_palette.js no modo claro):
# azul, laranja, água e amarelo, nessa ordem fixa. Texto nunca usa a cor da
# série; grade e eixos ficam recessivos.
SUPERFICIE = "#fcfcfb"
TINTA = "#0b0b0b"
TINTA_2 = "#52514e"
APAGADO = "#898781"
GRADE = "#e1e0d9"
EIXO = "#c3c2b7"
SERIES = ("#2a78d6", "#eb6834", "#1baf7a", "#eda100")

plt.rcParams.update({
    "figure.facecolor": SUPERFICIE, "axes.facecolor": SUPERFICIE,
    "savefig.facecolor": SUPERFICIE, "savefig.dpi": 150,
    "font.family": "sans-serif", "font.size": 9, "mathtext.default": "regular",
    "axes.edgecolor": EIXO, "axes.linewidth": 0.8, "axes.labelcolor": TINTA_2,
    "axes.titlesize": 10, "axes.titleweight": "semibold", "axes.titlecolor": TINTA,
    "axes.titlelocation": "left", "axes.spines.top": False,
    "axes.spines.right": False, "axes.grid": True, "axes.axisbelow": True,
    "grid.color": GRADE, "grid.linewidth": 0.8, "grid.linestyle": "-",
    "xtick.color": EIXO, "ytick.color": EIXO, "xtick.labelcolor": TINTA_2,
    "ytick.labelcolor": TINTA_2, "legend.frameon": False, "legend.fontsize": 9,
    "lines.linewidth": 1.6, "lines.solid_capstyle": "round",
    "lines.solid_joinstyle": "round",
})


def br(valor: float, casas: int = 2) -> str:
    """Número com vírgula decimal e ponto de milhar, como no resto da docs/."""
    texto = f"{valor:,.{casas}f}"
    return texto.replace(",", "§").replace(".", ",").replace("§", ".")


def br_curto(valor: float) -> str:
    """Até 3 algarismos significativos, para rótulos curtos."""
    if valor >= 100:
        return br(valor, 0)
    if valor >= 10:
        return br(valor, 1)
    return br(valor, 2)


def potencia(n: int) -> str:
    expoente = len(str(n)) - 1
    if n != 10 ** expoente:
        return str(n)
    return "10" + str(expoente).translate(str.maketrans("0123456789",
                                                        "⁰¹²³⁴⁵⁶⁷⁸⁹"))


def potencia_tex(n: int) -> str:
    expoente = len(str(n)) - 1
    return f"$10^{{{expoente}}}$" if n == 10 ** expoente else str(n)


def le_csv(caminho: Path) -> list[dict]:
    with open(caminho, newline="", encoding="utf-8") as arq:
        return list(csv.DictReader(arq))


class Dados:
    """Acesso às medições por (núcleo, tipo, N, configuração, threads)."""

    def __init__(self, execucao: list[dict], compilacao: list[dict]) -> None:
        self.exec = {(l["nucleo"], l["tipo"], int(l["n"]), l["config"],
                      int(l["threads"])): l for l in execucao}
        self.comp = compilacao
        self.tamanhos = sorted({int(l["n"]) for l in execucao})

    def linha(self, nucleo, tipo, n, config, threads=1):
        return self.exec.get((nucleo, tipo, n, config, threads))

    def tempo(self, nucleo, tipo, n, config, threads=1):
        linha = self.linha(nucleo, tipo, n, config, threads)
        return float(linha["mediana_ms"]) if linha else None


def eixo_log2(eixo, valores, formato) -> None:
    """Eixo em escala log2 com marcas nas potências de 2 que cobrem os valores."""
    baixo = min(min(valores), 1.0)
    alto = max(max(valores), 1.0)
    marcas = [2.0 ** k for k in range(-4, 9) if baixo / 1.5 <= 2.0 ** k <= alto * 1.5]
    eixo.set_yscale("log", base=2)
    eixo.yaxis.set_major_locator(FixedLocator(marcas))
    eixo.yaxis.set_minor_locator(NullLocator())
    eixo.yaxis.set_major_formatter(FuncFormatter(formato))
    eixo.set_ylim(baixo / 1.6, alto * 1.6)


def fmt_vezes(valor, _pos=None) -> str:
    return (br(valor, 0) if valor >= 1 else br(valor, 2)) + "×"


def cabecalho(fig, titulo: str, handles, ncol: int) -> None:
    """Título, legenda e painéis com distâncias fixas (em polegadas) do topo."""
    altura = fig.get_figheight()
    linhas = -(-len(handles) // ncol)
    # O tight_layout vem antes do título e da legenda, para não reservar o
    # espaço deles de novo; a posição dos dois é fixada aqui.
    fig.tight_layout(rect=(0, 0, 1, 1 - (0.36 + 0.2 * linhas) / altura))
    fig.suptitle(titulo, x=0.01, y=1 - 0.08 / altura, ha="left", va="top",
                 fontsize=10, color=TINTA, fontweight="semibold")
    fig.legend(handles=handles, loc="upper left", ncol=ncol, borderaxespad=0,
               bbox_to_anchor=(0.01, 1 - 0.34 / altura))


def fig_speedup(d: Dados, saida: Path) -> None:
    """Speedup de cada configuração sobre C0 (mesmo núcleo, mesmo N)."""
    series = (("C1-escalar", 1, "C1-escalar: -O2 sem vetorizar"),
              ("C1", 1, "C1: -O2 com o vetorizador do LLVM"),
              ("C2", 1, "C2: affine-super-vectorize (MLIR)"),
              ("C3", 8, "C3: OpenMP, 8 threads"))
    grupos = [(nu, t) for nu in NUCLEOS for t in TIPOS]
    largura, folga = 0.14, 0.025
    fig, eixos = plt.subplots(len(d.tamanhos), 1, squeeze=False, sharex=True,
                              figsize=(9, 2.7 * len(d.tamanhos) + 0.8))
    todos = []
    for linha, n in enumerate(d.tamanhos):
        eixo = eixos[linha][0]
        for g, (nu, t) in enumerate(grupos):
            base = d.tempo(nu, t, n, "C0")
            valores = []
            for i, (cfg, threads, _) in enumerate(series):
                v = d.tempo(nu, t, n, cfg, threads)
                if not base or not v:
                    continue
                s = base / v
                x = g + (i - (len(series) - 1) / 2) * (largura + folga)
                # A barra cresce a partir de 1× (C0), para cima ou para baixo.
                eixo.bar(x, s - 1, bottom=1, width=largura, color=SERIES[i],
                         linewidth=0)
                valores.append((s, x))
                todos.append(s)
            if valores:
                s, x = max(valores)
                eixo.annotate(fmt_vezes(s) if s >= 10 else br(s, 1) + "×",
                              (x, s), xytext=(0, 3), textcoords="offset points",
                              ha="center", va="bottom", fontsize=8, color=TINTA_2)
        eixo.axhline(1, color=APAGADO, linewidth=1)
        eixo.set_title(f"N = {potencia_tex(n)} elementos", pad=4)
        eixo.set_ylabel("speedup sobre C0")
        eixo.grid(axis="x", visible=False)
    for linha in range(len(d.tamanhos)):
        eixo_log2(eixos[linha][0], todos, fmt_vezes)
    eixo = eixos[-1][0]
    eixo.set_xticks(range(len(grupos)))
    eixo.set_xticklabels([f"{nu}\n{NOME_TIPO[t]}" for nu, t in grupos])
    cabecalho(fig, "Speedup sobre C0 (pipeline sem otimização), mediana de 5 "
              "execuções; a linha em 1× é o próprio C0",
              [Patch(color=SERIES[i], label=r) for i, (_, _, r) in
               enumerate(series)], 2)
    fig.savefig(saida / "speedup_configuracoes.png", bbox_inches="tight")
    plt.close(fig)


def fig_threads(d: Dados, saida: Path) -> None:
    """Speedup de C3 com k threads sobre C3 com 1 thread."""
    fig, eixos = plt.subplots(len(d.tamanhos), len(NUCLEOS), squeeze=False,
                              sharex=True, sharey=True,
                              figsize=(9, 2.5 * len(d.tamanhos) + 0.8))
    curvas = {}
    for n in d.tamanhos:
        for nu in NUCLEOS:
            for t in TIPOS:
                base = d.tempo(nu, t, n, "C3", 1)
                tempos = [d.tempo(nu, t, n, "C3", th) for th in THREADS]
                if base and None not in tempos:
                    curvas[(n, nu, t)] = [base / x for x in tempos]
    # Escala comum aos painéis; abaixo de 1 só se alguma curva piorar.
    menor = min([0.75, *(min(v) / 1.15 for v in curvas.values())])
    marcas = [m for m in (0.25, 0.5) if m >= menor] + list(THREADS)
    for r, n in enumerate(d.tamanhos):
        for c, nu in enumerate(NUCLEOS):
            eixo = eixos[r][c]
            eixo.plot(THREADS, THREADS, color=EIXO, linewidth=1)
            for k, t in enumerate(TIPOS):
                if (n, nu, t) in curvas:
                    eixo.plot(THREADS, curvas[(n, nu, t)], color=SERIES[k],
                              marker="o", markersize=5.5,
                              markeredgecolor=SUPERFICIE, markeredgewidth=1.5)
            eixo.set_xscale("log", base=2)
            eixo.set_yscale("log", base=2)
            for ax, locais in ((eixo.xaxis, THREADS), (eixo.yaxis, marcas)):
                ax.set_major_locator(FixedLocator(locais))
                ax.set_minor_locator(NullLocator())
                ax.set_major_formatter(FuncFormatter(
                    lambda v, _p: br(v, 0) if v >= 1 else br(v, 2)))
            eixo.set_ylim(menor, 10)
            eixo.set_title(f"{nu}, N = {potencia_tex(n)}", pad=4)
            if r == len(d.tamanhos) - 1:
                eixo.set_xlabel("threads (OMP_NUM_THREADS)")
            if c == 0:
                eixo.set_ylabel("speedup sobre 1 thread")
    handles = [Line2D([], [], color=SERIES[k], marker="o", markersize=5.5,
                      markeredgecolor=SUPERFICIE, label=NOME_TIPO[t])
               for k, t in enumerate(TIPOS)]
    handles.append(Line2D([], [], color=EIXO, linewidth=1,
                          label="ideal (speedup = threads)"))
    cabecalho(fig, "C3: escalabilidade com OpenMP (4 núcleos físicos, 8 threads "
              "com Hyper-Threading)", handles, 3)
    fig.savefig(saida / "escalabilidade_threads.png", bbox_inches="tight")
    plt.close(fig)


def variantes_pergunta(d: Dados, nucleo: str, n: int) -> list[tuple]:
    """(rótulo, linha do CSV) das variantes da pergunta, na ordem da tabela."""
    saida = []
    for cfg in CONFIGS_PERGUNTA:
        if cfg == "C3":
            for th in THREADS:
                linha = d.linha(nucleo, "f32", n, cfg, th)
                if linha:
                    saida.append((f"C3, {th} thread{'s' if th > 1 else ''}", linha))
        else:
            linha = d.linha(nucleo, "f32", n, cfg)
            if linha:
                saida.append((cfg, linha))
    return saida


def erro_relativo(linha: dict) -> float:
    ref = float(linha["referencia"])
    return abs(float(linha["resultado"]) - ref) / abs(ref)


def mesma_ordem(linha: dict) -> bool:
    return linha["resultado"] == linha["sequencial"]


def fig_pergunta(d: Dados, saida: Path) -> None:
    """Tempo e erro de somavet e prodesc em flut, por variante (maior N)."""
    n = max(d.tamanhos)
    nucleos = [nu for nu in ("somavet", "prodesc") if variantes_pergunta(d, nu, n)]
    if not nucleos:
        return
    fig, eixos = plt.subplots(len(nucleos), 2, squeeze=False,
                              figsize=(9, 3.1 * len(nucleos) + 0.8))
    for r, nu in enumerate(nucleos):
        itens = [(rot, l) for rot, l in variantes_pergunta(d, nu, n)
                 if not (rot.startswith("C3") and not rot.endswith("8 threads"))]
        ys = list(range(len(itens)))[::-1]
        cores = [SERIES[0] if mesma_ordem(l) else SERIES[1] for _, l in itens]
        for c, (titulo, valor, fmt) in enumerate((
                ("tempo (ms, mediana de 5)",
                 lambda l: float(l["mediana_ms"]), lambda v: br_curto(v)),
                ("erro relativo (referência em double)",
                 lambda l: max(erro_relativo(l), 1e-12),
                 lambda v: f"{v:.1e}".replace(".", ",")))):
            eixo = eixos[r][c]
            valores = [valor(l) for _, l in itens]
            eixo.barh(ys, valores, height=0.62, color=cores, linewidth=0)
            eixo.set_xscale("log")
            eixo.set_xlim(min(valores) / 2, max(valores) * 4)
            for y, v in zip(ys, valores):
                eixo.annotate(fmt(v), (v, y), xytext=(3, 0),
                              textcoords="offset points", va="center",
                              fontsize=8, color=TINTA_2)
            eixo.set_yticks(ys)
            eixo.set_yticklabels([rot for rot, _ in itens] if c == 0 else [])
            eixo.tick_params(axis="y", length=0)
            eixo.grid(axis="y", visible=False)
            eixo.set_title(f"{nu}: {titulo}", pad=4)
    cabecalho(fig, f"Pergunta obrigatória: reduções de flut (f32), N = "
              f"{potencia_tex(n)}, por variante de compilação", [
                  Patch(color=SERIES[0],
                        label="mesmos bits da soma na ordem do programa"),
                  Patch(color=SERIES[1],
                        label="resultado diferente (ordem ou FMA mudou)")], 2)
    fig.savefig(saida / "pergunta_reducao_flut.png", bbox_inches="tight")
    plt.close(fig)


def mediana_compilacao(d: Dados, config: str, n: int) -> dict | None:
    linhas = [l for l in d.comp if l["config"] == config and int(l["n"]) == n]
    if not linhas:
        return None
    return {k: statistics.median(float(l[k]) for l in linhas)
            for k in ("mlir_opt_ms", "mlir_translate_ms", "clang_ms", "total_ms")}


def fig_compilacao(d: Dados, saida: Path) -> None:
    n = max(d.tamanhos)
    configs = [c for c in CONFIGS_PRINCIPAIS if mediana_compilacao(d, c, n)]
    etapas = (("mlir_opt_ms", "mlir-opt"), ("mlir_translate_ms", "mlir-translate"),
              ("clang_ms", "clang -c"))
    fig, eixo = plt.subplots(figsize=(9, 0.42 * len(configs) + 1.3))
    ys = list(range(len(configs)))[::-1]
    for y, cfg in zip(ys, configs):
        m = mediana_compilacao(d, cfg, n)
        inicio = 0.0
        for k, (chave, _) in enumerate(etapas):
            # A borda na cor da superfície é o vão de 2 px entre os segmentos.
            eixo.barh(y, m[chave], left=inicio, height=0.6, color=SERIES[k],
                      edgecolor=SUPERFICIE, linewidth=1.5)
            inicio += m[chave]
        eixo.annotate(f"{br(m['total_ms'], 0)} ms", (inicio, y), xytext=(4, 0),
                      textcoords="offset points", va="center", fontsize=8,
                      color=TINTA_2)
    eixo.set_yticks(ys)
    eixo.set_yticklabels(configs)
    eixo.tick_params(axis="y", length=0)
    eixo.grid(axis="y", visible=False)
    eixo.set_xlabel("milissegundos (mediana de 5 compilações; mediana entre os "
                    "6 núcleos)")
    eixo.xaxis.set_major_formatter(FuncFormatter(lambda v, _p: br(v, 0)))
    eixo.set_xlim(0, max(mediana_compilacao(d, c, n)["total_ms"]
                         for c in configs) * 1.15)
    cabecalho(fig, f"Tempo de compilação do núcleo por etapa (N = "
              f"{potencia_tex(n)})", [Patch(color=SERIES[k], label=r) for k, (_, r)
                                      in enumerate(etapas)], 3)
    fig.savefig(saida / "tempo_compilacao.png", bbox_inches="tight")
    plt.close(fig)


def tabela(cabecalho: list[str], linhas: list[list[str]]) -> str:
    alinha = ["---"] + ["---:"] * (len(cabecalho) - 1)
    texto = ["| " + " | ".join(cabecalho) + " |", "| " + " | ".join(alinha) + " |"]
    texto += ["| " + " | ".join(l) + " |" for l in linhas]
    return "\n".join(texto)


def resumo_maquina(saida: Path) -> str:
    try:
        texto = (saida / "maquina.txt").read_text(encoding="utf-8")
    except OSError:
        return ""
    achados = []
    for rotulo, padrao in (("CPU", r"Model name:\s*(.+)"),
                           ("núcleos por soquete", r"Core\(s\) per socket:\s*(.+)"),
                           ("threads por núcleo", r"Thread\(s\) per core:\s*(.+)"),
                           ("L3", r"L3(?: cache)?:\s*(.+)"),
                           ("-march=native", r"target-cpu: (.+)"),
                           ("extensões SIMD", r"extensões: (.+)"),
                           ("clang", r"(Ubuntu clang version .+|clang version .+)"),
                           ("data (UTC)", r"== Data \(UTC\) e modo ==\n(.+)")):
        m = re.search(padrao, texto)
        if m:
            achados.append(f"- {rotulo}: `{m.group(1).strip()}`")
    return "\n".join(achados)


def gera_markdown(d: Dados, saida: Path) -> None:
    partes = [
        "# Parte C: resultados do make bench",
        "",
        "Gerado por `scripts/bench/graficos.py` a partir de `execucao.csv` e "
        "`compilacao.csv` (gravados por `scripts/bench/medir.py`). Não edite à "
        "mão: `make bench` regera este arquivo, os CSV e os gráficos. Cada tempo "
        "é a mediana de 5 execuções (o CSV tem as 5); a máquina completa está em "
        "`maquina.txt`.",
        "",
        resumo_maquina(saida),
        "",
    ]
    rotulos = [*CONFIGS_PRINCIPAIS[:-1], *(f"C3·{t}" for t in THREADS)]
    chaves = [*((c, 1) for c in CONFIGS_PRINCIPAIS[:-1]), *(("C3", t) for t in THREADS)]

    def linhas_por(valor) -> list[list[str]]:
        saida_linhas = []
        for n in d.tamanhos:
            for nu in NUCLEOS:
                for t in TIPOS:
                    celulas = [f"{nu} {t}", potencia(n)]
                    for cfg, th in chaves:
                        v = valor(nu, t, n, cfg, th)
                        celulas.append(v if v is not None else "—")
                    saida_linhas.append(celulas)
        return saida_linhas

    def tempo_fmt(nu, t, n, cfg, th):
        v = d.tempo(nu, t, n, cfg, th)
        return br(v, 3) if v is not None else None

    def speedup_fmt(nu, t, n, cfg, th):
        base, v = d.tempo(nu, t, n, "C0"), d.tempo(nu, t, n, cfg, th)
        return br(base / v, 2) + "×" if base and v else None

    def banda_fmt(nu, t, n, cfg, th):
        linha = d.linha(nu, t, n, cfg, th)
        return br(float(linha["gb_s"]), 1) if linha else None

    cab = ["núcleo", "N", *rotulos]
    partes += ["## Tempo de execução (ms, mediana de 5)", "",
               "C3·k é C3 com k threads. Só a chamada do núcleo é cronometrada.",
               "", tabela(cab, linhas_por(tempo_fmt)), ""]
    partes += ["## Speedup sobre C0", "", tabela(cab, linhas_por(speedup_fmt)), ""]
    partes += ["## Banda efetiva (GB/s)", "",
               "Bytes dos vetores percorridos por chamada (soma: 3 vetores; "
               "prodesc: 2; somavet: 1) divididos pelo tempo.", "",
               tabela(cab, linhas_por(banda_fmt)), ""]

    # Dispersão: (máximo - mínimo) / mediana das 5 repetições.
    dispersoes = [(float(l["max_ms"]) - float(l["min_ms"])) / float(l["mediana_ms"])
                  for l in d.exec.values() if float(l["mediana_ms"]) > 0]
    if dispersoes:
        partes += ["## Dispersão das repetições", "",
                   f"(máximo − mínimo) / mediana: mediana {br(100 * statistics.median(dispersoes), 1)}%, "
                   f"pior caso {br(100 * max(dispersoes), 1)}% em "
                   f"{len(dispersoes)} medições.", ""]

    # Escalabilidade de C3.
    linhas = []
    for n in d.tamanhos:
        for nu in NUCLEOS:
            for t in TIPOS:
                base = d.tempo(nu, t, n, "C3", 1)
                tempos = [d.tempo(nu, t, n, "C3", th) for th in THREADS]
                if not base or None in tempos:
                    continue
                s = [base / x for x in tempos]
                linhas.append([f"{nu} {t}", potencia(n), *(br(x, 2) + "×" for x in s),
                               br(100 * s[2] / 4, 0) + "%", br(100 * s[3] / 8, 0) + "%",
                               br(float(d.linha(nu, t, n, "C3", 8)["gb_s"]), 1)])
    partes += ["## C3: speedup sobre 1 thread", "",
               "Eficiência = speedup / threads. A máquina tem 4 núcleos físicos; "
               "8 threads usam o Hyper-Threading.", "",
               tabela(["núcleo", "N", *(f"{t} thr." for t in THREADS),
                       "efic. 4", "efic. 8", "GB/s com 8"], linhas), ""]

    # Pergunta obrigatória.
    partes += ["## Pergunta obrigatória: reduções de flut", "",
               "Δ ordem = resultado − soma sequencial em f32 (a ordem do "
               "programa); erro = |resultado − referência| / referência, com a "
               "referência em double e soma compensada. \"Mesmos bits\" diz se o "
               "resultado é idêntico ao da ordem do programa.", ""]
    for n in d.tamanhos:
        for nu in ("somavet", "prodesc"):
            itens = variantes_pergunta(d, nu, n)
            if not itens:
                continue
            base = d.tempo(nu, "f32", n, "C1")
            linhas = []
            for rot, l in itens:
                med = float(l["mediana_ms"])
                linhas.append([
                    rot, br(med, 3), br(base / med, 2) + "×" if base else "—",
                    l["resultado"].replace(".", ","),
                    br(float(l["resultado"]) - float(l["sequencial"]), 3),
                    f"{erro_relativo(l):.2e}".replace(".", ","),
                    "sim" if mesma_ordem(l) else "não",
                    "sim" if l["estavel"] == "1" else "não"])
            partes += [f"### {nu} f32, N = {potencia(n)}", "",
                       tabela(["variante", "ms", "speedup sobre C1", "resultado",
                               "Δ ordem", "erro", "mesmos bits", "estável"], linhas),
                       ""]

    # Compilação.
    linhas = []
    for n in d.tamanhos:
        for cfg in (*CONFIGS_PRINCIPAIS, "C1-reassoc", "C-O2"):
            m = mediana_compilacao(d, cfg, n)
            if m:
                linhas.append([cfg, potencia(n), br(m["mlir_opt_ms"], 1),
                               br(m["mlir_translate_ms"], 1), br(m["clang_ms"], 1),
                               br(m["total_ms"], 1)])
    partes += ["## Tempo de compilação do núcleo (ms)", "",
               "Mediana de 5 compilações de cada núcleo e, na tabela, a mediana "
               "entre os núcleos. Cada etapa é um processo (inclui o tempo de "
               "carregar a ferramenta). Em C-O2 o núcleo é C, então só há clang.",
               "", tabela(["config", "N", "mlir-opt", "mlir-translate", "clang -c",
                           "total"], linhas), ""]
    partes += ["## Gráficos", "",
               "![Speedup sobre C0](speedup_configuracoes.png)", "",
               "![Escalabilidade de C3](escalabilidade_threads.png)", "",
               "![Pergunta obrigatória](pergunta_reducao_flut.png)", "",
               "![Tempo de compilação](tempo_compilacao.png)", ""]
    (saida / "resultados.md").write_text("\n".join(partes), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--saida", required=True,
                        help="pasta com os CSV; recebe as tabelas e os gráficos")
    args = parser.parse_args()
    saida = Path(args.saida)
    d = Dados(le_csv(saida / "execucao.csv"), le_csv(saida / "compilacao.csv"))
    fig_speedup(d, saida)
    fig_threads(d, saida)
    fig_pergunta(d, saida)
    fig_compilacao(d, saida)
    gera_markdown(d, saida)
    print(f"graficos.py: tabelas e gráficos em {saida}")


if __name__ == "__main__":
    main()
