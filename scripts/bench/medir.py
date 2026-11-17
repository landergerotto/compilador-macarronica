#!/usr/bin/env python3
"""Estudo experimental de otimização: PjBL 4, Parte C (frente bench).

Para cada núcleo (soma, somavet, prodesc), tipo (i64 = inteira, f32 = flut),
tamanho N (10^6 e 10^7) e configuração (C0 a C3 e as variantes da pergunta
obrigatória), este script:

1. gera o .mlir a partir do modelo em nucleos/ (troca @N@ e @FMF@);
2. roda o pipeline da configuração (mlir-opt, mlir-translate e clang -c),
   cronometrando cada etapa 5 vezes: é o tempo de compilação;
3. liga o objeto do núcleo ao driver.c e executa o programa; o driver faz
   1 chamada de aquecimento e 5 cronometradas: é o tempo de execução;
4. grava execucao.csv, compilacao.csv, maquina.txt e as evidências (MLIR
   transformado, LLVM IR otimizado, assembly e relatórios -Rpass).

As tabelas e os gráficos saem de graficos.py, a partir dos CSV.

Uso (normalmente pelo make bench, que copia esta pasta para /tmp antes):
    python3 medir.py --trabalho /tmp/macaronica-bench/completo --saida docs/bench
    python3 medir.py --modo rapido ...  só N = 10^6 e 1 compilação por configuração
    python3 medir.py --modo teste ...   N = 4096: só confere resultados e -Rpass
"""

from __future__ import annotations

import argparse
import csv
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

AQUI = Path(__file__).resolve().parent

NUCLEOS = ("soma", "somavet", "prodesc")
TIPOS = ("i64", "f32")
THREADS = (1, 2, 4, 8)
BYTES_ELEMENTO = {"i64": 8, "f32": 4}
# Vetores percorridos por chamada (soma lê a e b e escreve c), para o GB/s.
VETORES_POR_CHAMADA = {"soma": 3, "somavet": 1, "prodesc": 2}

# modo: (tamanhos, repetições da compilação, repetições da execução)
MODOS = {
    "completo": ((10**6, 10**7), 5, 5),
    "rapido": ((10**6,), 1, 5),
    "teste": ((4096,), 1, 1),
}
# Tamanho cujas IR, assembly e relatórios -Rpass vão para evidencias/.
N_EVIDENCIA = 10**6

# Mesmas flags de CPU em todas as configurações (C1 e C2 em particular).
MARCH = os.environ.get("BENCH_MARCH") or "native"
OMP_PROC_BIND = os.environ.get("BENCH_OMP_PROC_BIND") or "spread"
OMP_PLACES = os.environ.get("BENCH_OMP_PLACES") or "cores"

# ---------------------------------------------------------------------------
# Pipelines do mlir-opt (MLIR 18.1.8). Um passo por item, com o que ele faz.
# ---------------------------------------------------------------------------

# Do dialeto cf/arith/memref/func até o dialeto llvm. Comum a todas.
PARA_LLVM = (
    "--convert-arith-to-llvm",        # arith.addi/addf/muli -> llvm.add/fadd/mul
    "--finalize-memref-to-llvm",      # memref.load/store -> llvm.getelementptr + load/store
    # func.func -> llvm.func. Com bare-ptr, cada memref estático vira um ptr
    # simples (sem o descritor de 5 campos), que é o que driver.c declara.
    "--convert-func-to-llvm=use-bare-ptr-memref-call-conv=1",
    "--convert-cf-to-llvm",           # cf.br/cf.cond_br -> llvm.br/llvm.cond_br
    "--reconcile-unrealized-casts",   # remove os casts provisórios entre tipos
)

# C0, C1 e variantes: só rebaixa; o núcleo continua escalar.
REBAIXA_ESCALAR = (
    "--lower-affine",                 # affine.for/load/store -> scf.for + memref.load/store
    "--convert-scf-to-cf",            # scf.for -> blocos básicos com cf.br/cf.cond_br
    *PARA_LLVM,
)

# C2: vetorização explícita no MLIR.
VETORIZA_MLIR = (
    # affine.for de passo 1 -> affine.for de passo 8 com vector.transfer_read/
    # transfer_write e aritmética em vector<8xT>. vectorize-reductions=true
    # vetoriza também os laços com iter_args (somavet e prodesc): o acumulador
    # vira vector<8xT> e um vector.reduction junta as 8 parcelas no fim.
    "--affine-super-vectorize=virtual-vector-size=8 vectorize-reductions=true",
    "--canonicalize",                 # dobra constantes e limpa o que sobrou
)
REBAIXA_VETOR = (
    "--lower-affine",
    "--convert-vector-to-scf",        # transfers de n dimensões -> laços de 1 dimensão
    "--convert-scf-to-cf",
    # vector.* -> instruções vetoriais do LLVM (<8 x float>, llvm.masked.load,
    # llvm.vector.reduce.*)
    "--convert-vector-to-llvm",
    *PARA_LLVM,
)

# C3: paralelismo com OpenMP.
PARALELIZA_MLIR = (
    # affine.for sem dependência entre iterações -> affine.parallel. Com
    # parallel-reductions=true, as reduções (iter_args) viram
    # affine.parallel ... reduce ("addf"/"addi"). max-nested=1: só o laço externo.
    "--affine-parallelize=max-nested=1 parallel-reductions=true",
    "--lower-affine",                 # affine.parallel -> scf.parallel (+ scf.reduce)
    "--convert-scf-to-openmp",        # scf.parallel -> omp.parallel + omp.wsloop (+ redução)
    "--canonicalize",                 # o MLIR 18 precisa dele antes de rebaixar o omp
)
REBAIXA_OPENMP = (
    "--convert-scf-to-cf",
    *PARA_LLVM[:-1],
    "--convert-openmp-to-llvm",       # operandos e regiões do omp -> tipos do dialeto llvm
    "--reconcile-unrealized-casts",
)

CPU = (f"-march={MARCH}",)
RPASS = ("-Rpass=loop-vectorize", "-Rpass-missed=loop-vectorize",
         "-Rpass-analysis=loop-vectorize")
SEM_VETORIZADOR_LLVM = ("-fno-vectorize", "-fno-slp-vectorize")


@dataclass(frozen=True)
class Config:
    nome: str
    descricao: str
    clang: tuple                      # flags do clang para o núcleo (.ll -> .o)
    transforma: tuple = ()            # passos do mlir-opt que mudam o núcleo (C2, C3)
    rebaixa: tuple = REBAIXA_ESCALAR  # passos que só rebaixam até o dialeto llvm
    fmf: str = ""                     # atributo fastmath da soma nas reduções de flut
    openmp: bool = False
    threads: tuple = (1,)
    fonte_c: bool = False             # núcleo escrito em C (referencia_c.c), sem MLIR
    so_reducao_flut: bool = False     # só somavet e prodesc em f32 (pergunta obrigatória)

    @property
    def passos(self) -> tuple:
        return (*self.transforma, *self.rebaixa)

    def aplica(self, nucleo: str, tipo: str) -> bool:
        return not self.so_reducao_flut or (tipo == "f32" and nucleo != "soma")


CONFIGS = (
    Config("C0", "linha de base: pipeline sem vetorização; clang -O0 sobre o IR "
           "do mlir-translate", clang=("-O0", *CPU)),
    Config("C1-escalar", "clang -O2 com os vetorizadores do LLVM desligados "
           "(-fno-vectorize -fno-slp-vectorize): só otimização escalar",
           clang=("-O2", *CPU, *SEM_VETORIZADOR_LLVM)),
    Config("C1", "clang -O2: otimização genérica e autovetorização do LLVM",
           clang=("-O2", *CPU, *RPASS)),
    Config("C2", "affine-super-vectorize no MLIR (vetor virtual de 8); clang -O2 "
           "com os vetorizadores do LLVM desligados",
           transforma=VETORIZA_MLIR, rebaixa=REBAIXA_VETOR,
           clang=("-O2", *CPU, *SEM_VETORIZADOR_LLVM, *RPASS)),
    Config("C3", "affine-parallelize e convert-scf-to-openmp (libomp do LLVM 18); "
           "clang -O2", transforma=PARALELIZA_MLIR, rebaixa=REBAIXA_OPENMP,
           clang=("-O2", *CPU, *RPASS), openmp=True, threads=THREADS),
    # Pergunta obrigatória: só nas reduções de flut (somavet e prodesc).
    Config("C1-reassoc", "C1 com fastmath<reassoc> na soma do núcleo MLIR "
           "(vira fadd reassoc no LLVM IR)", clang=("-O2", *CPU, *RPASS),
           fmf=" fastmath<reassoc>", so_reducao_flut=True),
    Config("C1-ffast-math", "C1 com -ffast-math no clang, sobre o mesmo .ll "
           "sem reassoc", clang=("-O2", *CPU, "-ffast-math", *RPASS),
           so_reducao_flut=True),
    Config("C1-ordenada", "C1 autorizando o LLVM a vetorizar a redução sem "
           "reassociar (-mllvm -force-ordered-reductions)",
           clang=("-O2", *CPU, "-mllvm", "-force-ordered-reductions", *RPASS),
           so_reducao_flut=True),
    Config("C-O2", "a mesma redução escrita em C (referencia_c.c), clang -O2 "
           "-ffp-contract=off", clang=("-O2", *CPU, "-ffp-contract=off", *RPASS),
           fonte_c=True, so_reducao_flut=True),
    Config("C-ffast-math", "a mesma redução escrita em C, clang -O2 -ffast-math",
           clang=("-O2", *CPU, "-ffast-math", *RPASS), fonte_c=True,
           so_reducao_flut=True),
)


class Falha(RuntimeError):
    """Um passo do pipeline terminou com erro."""


def roda(cmd, env=None) -> tuple[float, str, str]:
    """Executa cmd e devolve (segundos, stdout, stderr); Falha se der erro."""
    cmd = [str(c) for c in cmd]
    t0 = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True, env=env)
    segundos = time.perf_counter() - t0
    if proc.returncode != 0:
        raise Falha(f"falhou ({proc.returncode}): {' '.join(cmd)}\n"
                    f"{proc.stdout}{proc.stderr}")
    return segundos, proc.stdout, proc.stderr


class Ferramentas:
    """Localiza o LLVM/MLIR 18: $LLVM_BINDIR, /usr/lib/llvm-18/bin ou <nome>-18."""

    def __init__(self) -> None:
        self.bindir = os.environ.get("LLVM_BINDIR") or "/usr/lib/llvm-18/bin"
        self.mlir_opt = self._acha("mlir-opt")
        self.mlir_translate = self._acha("mlir-translate")
        self.clang = self._acha("clang")
        self.libdir = str(Path(self.clang).resolve().parent.parent / "lib")

    def _acha(self, nome: str) -> str:
        candidatos = [Path(self.bindir) / nome, shutil.which(f"{nome}-18")]
        for caminho in candidatos:
            if caminho and os.access(caminho, os.X_OK):
                return str(caminho)
        raise SystemExit(
            f"medir.py: {nome} do LLVM/MLIR 18 não encontrado. Instale "
            "llvm-18, clang-18, mlir-18-tools e libomp-18-dev (apt) ou "
            "defina LLVM_BINDIR.")

    def versao(self, ferramenta: str) -> str:
        _, saida, erro = roda([ferramenta, "--version"])
        return (saida or erro).strip()


def gera_mlir(nucleo: str, tipo: str, n: int, fmf: str) -> str:
    modelo = (AQUI / "nucleos" / f"{nucleo}_{tipo}.mlir.in").read_text(
        encoding="utf-8")
    return modelo.replace("@N@", str(n)).replace("@FMF@", fmf)


def defines(nucleo: str, tipo: str, n: int) -> list[str]:
    return [f"-DN={n}", f"-DNUCLEO_{nucleo.upper()}", f"-DTIPO_{tipo.upper()}",
            f"-DFUNCAO={nucleo}_{tipo}"]


def sem_rpass(flags) -> list[str]:
    return [f for f in flags if not f.startswith("-Rpass")]


def compila_nucleo(f: Ferramentas, cfg: Config, nucleo: str, tipo: str, n: int,
                   pasta: Path, reps: int) -> dict:
    """Compila o núcleo pela configuração, reps vezes, cronometrando as etapas."""
    pasta.mkdir(parents=True, exist_ok=True)
    base = pasta / f"{nucleo}_{tipo}"
    obj = base.with_suffix(".o")
    tempos = {"mlir_opt": [], "mlir_translate": [], "clang": []}
    if cfg.fonte_c:
        fonte = AQUI / "referencia_c.c"
        etapas = []
        cmd_cc = [f.clang, *cfg.clang, *defines(nucleo, tipo, n), "-c", fonte,
                  "-o", obj]
    else:
        fonte = base.with_suffix(".ll")
        mlir = base.with_suffix(".mlir")
        mlir.write_text(gera_mlir(nucleo, tipo, n, cfg.fmf), encoding="utf-8")
        llvm_mlir = pasta / f"{nucleo}_{tipo}.llvm.mlir"
        etapas = [
            ("mlir_opt", [f.mlir_opt, *cfg.passos, mlir, "-o", llvm_mlir]),
            ("mlir_translate", [f.mlir_translate, "--mlir-to-llvmir", llvm_mlir,
                                "-o", fonte]),
        ]
        cmd_cc = [f.clang, *cfg.clang, "-c", fonte, "-o", obj]
    remarks = ""
    for _ in range(reps):
        for nome, cmd in etapas:
            tempos[nome].append(roda(cmd)[0])
        segundos, _, remarks = roda(cmd_cc)
        tempos["clang"].append(segundos)
    return {"tempos": tempos, "obj": obj, "fonte": fonte, "remarks": remarks}


def compila_driver(f: Ferramentas, nucleo: str, tipo: str, n: int,
                   openmp: bool, pasta: Path) -> Path:
    """driver.c sempre igual (-O2, sem fast-math, sem FMA); um por núcleo e N."""
    pasta.mkdir(parents=True, exist_ok=True)
    obj = pasta / f"driver_{nucleo}_{tipo}{'_omp' if openmp else ''}.o"
    if not obj.exists():
        cmd = [f.clang, "-O2", "-std=c99", "-Wall", "-Wextra", "-ffp-contract=off",
               *defines(nucleo, tipo, n), "-c", AQUI / "driver.c", "-o", obj]
        if openmp:
            cmd[1:1] = ["-fopenmp", "-DBENCH_OPENMP"]
        roda(cmd)
    return obj


def liga(f: Ferramentas, driver: Path, nucleo_obj: Path, openmp: bool,
         exe: Path) -> Path:
    cmd = [f.clang, driver, nucleo_obj, "-o", exe, "-lm"]
    if openmp:
        cmd += ["-fopenmp", f"-Wl,-rpath,{f.libdir}"]
    roda(cmd)
    return exe


def executa(exe: Path, reps: int, threads: int, openmp: bool) -> dict:
    """Roda o binário e devolve os pares chave=valor da linha do driver."""
    env = dict(os.environ)
    if openmp:
        env.update(OMP_NUM_THREADS=str(threads), OMP_PROC_BIND=OMP_PROC_BIND,
                   OMP_PLACES=OMP_PLACES)
    proc = subprocess.run([str(exe), str(reps)], capture_output=True, text=True,
                          env=env)
    linhas = proc.stdout.strip().splitlines()
    if not linhas or "tempos_ns=" not in linhas[-1]:
        raise Falha(f"{exe} não imprimiu a linha de medição "
                    f"(código {proc.returncode}):\n{proc.stdout}{proc.stderr}")
    dados = dict(par.split("=", 1) for par in linhas[-1].split())
    dados["stderr"] = proc.stderr.strip()
    return dados


def carga() -> str:
    try:
        return Path("/proc/loadavg").read_text().split()[0]
    except OSError:
        return ""


def ms(segundos_ou_ns, escala: float) -> float:
    return segundos_ou_ns * escala


def limpa(texto: str, trabalho: Path) -> str:
    """Tira das evidências o caminho da pasta temporária."""
    return texto.replace(str(trabalho) + "/", "").replace(str(trabalho), ".")


def gera_evidencias(f: Ferramentas, cfg: Config, nucleo: str, tipo: str, n: int,
                    comp: dict, pasta: Path, trabalho: Path,
                    evidencias: Path) -> None:
    """IR otimizado, assembly e (C2/C3) o MLIR transformado, para N_EVIDENCIA."""
    destino = evidencias / cfg.nome
    destino.mkdir(parents=True, exist_ok=True)
    nome = f"{nucleo}_{tipo}"
    flags = sem_rpass(cfg.clang)
    extra = defines(nucleo, tipo, n) if cfg.fonte_c else []
    if not cfg.fonte_c:
        # Núcleo .mlir de entrada (o de C1-reassoc tem o fastmath<reassoc>).
        alvo_mlir = evidencias / "nucleos" / (
            f"{nome}{'_reassoc' if cfg.fmf else ''}.mlir")
        alvo_mlir.parent.mkdir(parents=True, exist_ok=True)
        if not alvo_mlir.exists():
            shutil.copyfile(pasta / f"{nome}.mlir", alvo_mlir)
        if cfg.transforma:
            # O núcleo logo depois da transformação: dialeto vector (C2) ou omp (C3).
            sufixo = "vector" if cfg.nome == "C2" else "omp"
            _, saida, _ = roda([f.mlir_opt, *cfg.transforma, pasta / f"{nome}.mlir"])
            (destino / f"{nome}.{sufixo}.mlir").write_text(saida, encoding="utf-8")
        # LLVM IR como sai do mlir-translate (sem tripla de alvo, sem otimização).
        texto = Path(comp["fonte"]).read_text(encoding="utf-8")
        (destino / f"{nome}.mlir-translate.ll").write_text(limpa(texto, trabalho),
                                                          encoding="utf-8")
    for opcoes, sufixo in ((["-S", "-emit-llvm"], "otimizado.ll"), (["-S"], "s")):
        saida_arq = destino / f"{nome}.{sufixo}"
        roda([f.clang, *flags, *extra, *opcoes, comp["fonte"], "-o", saida_arq])
        saida_arq.write_text(limpa(saida_arq.read_text(encoding="utf-8"), trabalho),
                             encoding="utf-8")


def descreve_maquina(f: Ferramentas, modo: str) -> str:
    partes = []

    def secao(titulo: str, texto: str) -> None:
        partes.append(f"== {titulo} ==\n{texto.rstrip()}\n")

    def saida_de(cmd) -> str:
        try:
            _, saida, erro = roda(cmd)
            return saida + erro
        except (Falha, OSError) as erro:
            return f"(indisponível: {erro})"

    secao("Data (UTC) e modo",
          time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime()) + f"  modo={modo}")
    secao("Sistema", f"{platform.platform()}\n"
          + saida_de(["uname", "-a"])
          + ("WSL2 (Hyper-V)\n" if "microsoft" in platform.release().lower() else ""))
    secao("lscpu", re.sub(r"(?m)^Vulnerability.*\n?", "", saida_de(["lscpu"])))
    try:
        memoria = [l for l in Path("/proc/meminfo").read_text().splitlines()
                   if l.startswith("MemTotal")]
    except OSError:
        memoria = []
    secao("Memória", "\n".join(memoria))
    secao("Carga no início (/proc/loadavg)", saida_de(["cat", "/proc/loadavg"]))
    secao("Ferramentas", "\n".join([
        f.versao(f.clang).splitlines()[0],
        f.versao(f.mlir_opt).replace("\n", " "),
        f"mlir-translate: {f.mlir_translate}",
        f"python {platform.python_version()}",
    ]))
    # O que -march=native virou nesta máquina (CPU e extensões SIMD).
    nativo = saida_de([f.clang, f"-march={MARCH}", "-###", "-c", "-x", "c",
                       "/dev/null", "-o", "/dev/null"])
    cpu = re.search(r'"-target-cpu" "([^"]+)"', nativo)
    feats = re.findall(r'"-target-feature" "([+-][^"]+)"', nativo)
    simd = [x for x in feats if re.match(
        r"[+-](sse4\.2|avx|avx2|fma|avx512f|avx512dq|avx512vl)$", x)]
    secao(f"-march={MARCH}", f"target-cpu: {cpu.group(1) if cpu else '?'}\n"
          f"extensões: {' '.join(simd)}")
    secao("OpenMP (C3)", f"OMP_NUM_THREADS in {THREADS}; "
          f"OMP_PROC_BIND={OMP_PROC_BIND}; OMP_PLACES={OMP_PLACES}; "
          f"libomp de {f.libdir}")
    linhas = []
    for cfg in CONFIGS:
        linhas.append(f"{cfg.nome}: {cfg.descricao}")
        if not cfg.fonte_c:
            linhas.append("  mlir-opt " + " ".join(f"'{p}'" if " " in p else p
                                                   for p in cfg.passos))
            linhas.append("  mlir-translate --mlir-to-llvmir")
        linhas.append("  clang " + " ".join(cfg.clang) + " -c")
    secao("Configurações", "\n".join(linhas))
    return "\n".join(partes)


def grava_csv(caminho: Path, linhas: list[dict]) -> None:
    if not linhas:
        return
    with open(caminho, "w", newline="", encoding="utf-8") as arq:
        escritor = csv.DictWriter(arq, fieldnames=list(linhas[0].keys()))
        escritor.writeheader()
        escritor.writerows(linhas)


def confere(linhas_exec: list[dict], remarks: dict) -> list[str]:
    """Verificações do modo teste (e do completo): devolve a lista de erros."""
    erros = []
    for l in linhas_exec:
        if l["ok"] != "1":
            erros.append(f"{l['nucleo']}_{l['tipo']} {l['config']} "
                         f"t={l['threads']}: resultado errado ({l['resultado']})")
    # Inteiros e soma elemento a elemento: todas as configurações, mesmos bits.
    grupos: dict = {}
    for l in linhas_exec:
        if l["tipo"] == "i64" or l["nucleo"] == "soma":
            grupos.setdefault((l["nucleo"], l["tipo"], l["n"]), set()).add(
                l["resultado"])
    for chave, valores in grupos.items():
        if len(valores) > 1:
            erros.append(f"{chave}: configurações divergem: {sorted(valores)}")
    # As afirmações do relatório que dependem do -Rpass.
    esperado = [
        (("C1", "soma", "f32"), "vectorized loop"),
        (("C1", "somavet", "i64"), "vectorized loop"),
        (("C1", "somavet", "f32"), "cannot prove it is safe to reorder"),
        (("C1-reassoc", "somavet", "f32"), "vectorized loop"),
    ]
    for chave, trecho in esperado:
        if chave in remarks and trecho not in remarks[chave]:
            erros.append(f"-Rpass de {chave} sem '{trecho}':\n{remarks[chave]}")
    return erros


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--modo", choices=sorted(MODOS), default="completo")
    parser.add_argument("--trabalho", required=True,
                        help="pasta de build (no sistema de arquivos do Linux)")
    parser.add_argument("--saida", required=True,
                        help="onde gravar CSV, maquina.txt e evidencias/")
    args = parser.parse_args()

    tamanhos, reps_comp, reps_exec = MODOS[args.modo]
    trabalho = Path(args.trabalho).resolve()
    saida = Path(args.saida).resolve()
    trabalho.mkdir(parents=True, exist_ok=True)
    saida.mkdir(parents=True, exist_ok=True)
    evidencias = saida / "evidencias" if args.modo == "completo" else None
    if evidencias and evidencias.exists():
        shutil.rmtree(evidencias)

    f = Ferramentas()
    maquina = descreve_maquina(f, args.modo)
    linhas_exec: list[dict] = []
    linhas_comp: list[dict] = []
    remarks: dict = {}

    total = sum(1 for n in tamanhos for nu in NUCLEOS for t in TIPOS
                for c in CONFIGS if c.aplica(nu, t))
    feitos = 0
    for n in tamanhos:
        for nucleo in NUCLEOS:
            for tipo in TIPOS:
                for cfg in CONFIGS:
                    if not cfg.aplica(nucleo, tipo):
                        continue
                    feitos += 1
                    pasta = trabalho / f"n{n}" / cfg.nome
                    comp = compila_nucleo(f, cfg, nucleo, tipo, n, pasta, reps_comp)
                    if n == min(tamanhos):
                        remarks[(cfg.nome, nucleo, tipo)] = limpa(
                            comp["remarks"], trabalho)
                    mediana = {k: statistics.median(v) * 1e3 if v else 0.0
                               for k, v in comp["tempos"].items()}
                    totais = [sum(x) * 1e3 for x in zip(*[v for v in
                              comp["tempos"].values() if v])]
                    linhas_comp.append({
                        "nucleo": nucleo, "tipo": tipo, "n": n, "config": cfg.nome,
                        "reps": reps_comp,
                        "mlir_opt_ms": f"{mediana['mlir_opt']:.2f}",
                        "mlir_translate_ms": f"{mediana['mlir_translate']:.2f}",
                        "clang_ms": f"{mediana['clang']:.2f}",
                        "total_ms": f"{statistics.median(totais):.2f}",
                        "totais_ms": ";".join(f"{x:.2f}" for x in totais),
                    })
                    driver = compila_driver(f, nucleo, tipo, n, cfg.openmp,
                                            trabalho / f"n{n}" / "driver")
                    exe = liga(f, driver, comp["obj"], cfg.openmp,
                               pasta / f"bench_{nucleo}_{tipo}")
                    for threads in cfg.threads:
                        d = executa(exe, reps_exec, threads, cfg.openmp)
                        tempos_ms = [int(x) / 1e6 for x in d["tempos_ns"].split(",")]
                        med = statistics.median(tempos_ms)
                        gbytes = (VETORES_POR_CHAMADA[nucleo] * n
                                  * BYTES_ELEMENTO[tipo] / 1e9)
                        linhas_exec.append({
                            "nucleo": nucleo, "tipo": tipo, "n": n,
                            "config": cfg.nome, "threads": d["threads"],
                            "reps": d["reps"],
                            "mediana_ms": f"{med:.4f}",
                            "min_ms": f"{min(tempos_ms):.4f}",
                            "max_ms": f"{max(tempos_ms):.4f}",
                            "tempos_ms": ";".join(f"{x:.4f}" for x in tempos_ms),
                            "gb_s": f"{gbytes / (med / 1e3):.2f}",
                            "resultado": d.get("resultado", ""),
                            "sequencial": d.get("sequencial", ""),
                            "referencia": d.get("referencia", ""),
                            "ok": d["ok"], "estavel": d["estavel"],
                            "carga": carga(),
                        })
                        print(f"[{feitos:3d}/{total}] {nucleo:7s} {tipo} N={n:<8d} "
                              f"{cfg.nome:13s} t={d['threads']} mediana "
                              f"{med:9.3f} ms (min {min(tempos_ms):.3f}, max "
                              f"{max(tempos_ms):.3f}) ok={d['ok']}"
                              + (f" [{d['stderr']}]" if d["stderr"] else ""),
                              flush=True)
                    if evidencias and n == N_EVIDENCIA:
                        gera_evidencias(f, cfg, nucleo, tipo, n, comp, pasta,
                                        trabalho, evidencias)

    erros = confere(linhas_exec, remarks)
    if args.modo != "teste":
        grava_csv(saida / "execucao.csv", linhas_exec)
        grava_csv(saida / "compilacao.csv", linhas_comp)
        maquina += f"\n== Carga no fim (/proc/loadavg) ==\n{carga()}\n"
        (saida / "maquina.txt").write_text(maquina, encoding="utf-8")
    if evidencias:
        pasta_rpass = evidencias / "rpass"
        pasta_rpass.mkdir(parents=True, exist_ok=True)
        for cfg in CONFIGS:
            blocos = [f"### {nu}_{t} (N = {N_EVIDENCIA})\n{texto.strip() or '(nenhum)'}\n"
                      for (nome, nu, t), texto in remarks.items() if nome == cfg.nome]
            if blocos and any("-Rpass" in x for x in cfg.clang):
                cabecalho = (f"# -Rpass de {cfg.nome}: {cfg.descricao}\n"
                             f"# clang {' '.join(cfg.clang)}\n\n")
                (pasta_rpass / f"{cfg.nome}.txt").write_text(
                    cabecalho + "\n".join(blocos), encoding="utf-8")
    for erro in erros:
        print(f"ERRO: {erro}", file=sys.stderr)
    print(f"medir.py: {len(linhas_exec)} medições, {len(erros)} erro(s)")
    return 1 if erros else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Falha as erro:
        print(f"medir.py: {erro}", file=sys.stderr)
        sys.exit(1)
