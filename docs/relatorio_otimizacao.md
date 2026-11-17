# Experimentos de otimização: PjBL 4, Parte C

O experimento em `scripts/bench/run.sh` mede kernels produzidos a partir de
fontes Macaronica. Os templates em `scripts/bench/fixtures/` contêm adição
elemento a elemento (`c receba a + b`) e produto escalar (`prodesc(a,b)`).
O runner instancia comprimentos de **1.000.000** e **10.000.000** elementos:
`flut` para adição e `inteira` para produto escalar. O driver C fornece
entradas, relógio e validação; o corpo do kernel
vem do compilador e do lowering MLIR, sem substituição por implementação C
escrita à mão.

## Execução reproduzível

Use Bash com Clang, ferramentas LLVM e MLIR compatíveis no PATH, além de um
runtime OpenMP compatível com Clang para C3. Compile a versão release do
compilador antes de executar:

```bash
make release
bash scripts/bench/run.sh build/release/macaronica build/bench
```

No MSYS2, acrescente `.exe` ao nome do compilador. Configure caminhos
individuais quando necessário:

```bash
LLVM_CLANG=/caminho/clang \
MLIR_OPT=/caminho/mlir-opt MLIR_TRANSLATE=/caminho/mlir-translate \
LLVM_AS=/caminho/llvm-as LLVM_OPT=/caminho/opt \
BENCH_REPEATS=7 BENCH_THREADS="1 2 4" \
bash scripts/bench/run.sh build/release/macaronica build/bench
```

`BENCH_REPEATS` aceita no mínimo cinco repetições medidas. Por padrão, C3
executa com 1, 2, 4 e sucessivas potências de dois até o número de CPUs
lógicas. `BENCH_THREADS` restringe a lista; o runner rejeita valores maiores
que as CPUs lógicas. `BENCH_SIZES="1000000"` permite uma execução curta;
a execução final deve conter ambos os tamanhos. `BENCH_PIPELINE` permite
selecionar outro caminho para o mesmo contrato de pipeline.

## Configurações comparadas

| Configuração | Lowering do mesmo kernel | Compilação do kernel |
|---|---|---|
| C0 | MLIR escalar | Clang `-O0 -fno-vectorize -fno-slp-vectorize` |
| C1 | MLIR escalar | Clang `-O2`, com reports de vetorização |
| C2 | MLIR com vetorização explícita | Clang `-O2`, vetorização automática desativada |
| C3 | MLIR paralelo, SCF para OpenMP | Clang `-O2 -fopenmp` |

A compilação do driver usa `-O2` em todas as configurações, em uma unidade
separada. Assim, os reports de C1 no estágio de compilação do kernel não
confundem os laços de inicialização e verificação do driver com o kernel.
O frontend gera MLIR uma vez por operação/tamanho; cada configuração mede
separadamente o tempo de pipeline, compilação e ligação, registrado em
`compile.csv`. Esse tempo inclui compilação do driver e exclui geração de
MLIR pelo frontend. O tempo de execução começa imediatamente antes da
chamada ao kernel e termina ao retornar.

O produto escalar deste experimento usa inteiros de 32 bits, cuja soma
modular é associativa e permite redução vetorial/paralela. Reduções reais
mantêm ordem escalar no pipeline atual e não integram esta matriz de
desempenho. A execução de um programa real sob C2/C3 não constitui por si
só evidência de SIMD ou paralelismo na redução. O arquivo
`transformations.csv` registra presença de tipos vetoriais e chamadas ao
runtime OpenMP no LLVM IR anterior à compilação Clang. C2/C3
somente são medidos se a transformação correspondente aparecer na IR.
A ausência de vetor nesse arquivo em C1 não significa que o Clang deixou
de vetorizar: a IR inspecionada é a entrada do Clang; consulte os reports
de C1 para a decisão de autovetorização.

## Medição e correção

O driver usa `QueryPerformanceCounter` em Windows ou `CLOCK_MONOTONIC` em
POSIX. Alocação, inicialização e validação ficam fora da região medida.
Há um aquecimento descartado e no mínimo cinco amostras por configuração.
A tabela usa a mediana, incluindo média dos dois valores centrais quando
o número de amostras é par. O speedup é `mediana(C0) / mediana(configuração)`
para a mesma operação e comprimento.

As entradas contêm `a[i] = b[i] = 1`. A adição verifica **todos** os
elementos de saída e o checksum em `double`, fora da região medida.
O produto escalar precisa produzir exatamente N: os produtos e somas
parciais cabem em `int32_t` para os tamanhos usados. Cada
amostra, inclusive o aquecimento, passa por validação; resultados incorretos
não entram na tabela de desempenho. Esses dados verificam o percurso de
medição; testes de compilador devem cobrir também valores variados, sinais,
caudas e casos de borda.

O buffer de saída da adição é limpo antes de cada chamada para evitar que
uma execução anterior esconda um kernel que deixou de escrever posições.
Esse procedimento também aquece parte do conjunto de trabalho. O método
mede execução repetida com entradas residentes; não representa latência de
primeiro acesso a dados frios nem um programa completo com E/S.

## Artefatos e interpretação

O diretório de saída contém:

- `samples.csv`: amostras individuais, checksum e número de threads.
- `summary.csv`: medianas, speedups e tempo de compilação por configuração.
- `compile.csv`: tempo de pipeline, compilação e ligação separado do runtime.
- `transformations.csv`: evidência de vetores/OpenMP antes do Clang.
- `status.csv`: configurações medidas, indisponíveis ou incorretas.
- `environment.txt`: data UTC, SO, versões, CPU e caches disponíveis.
- `medians.svg`: gráfico das medianas, gerado sem dependências gráficas.
- Subdiretórios por operação/tamanho com fonte instanciada, MLIR, IR,
  estágios do pipeline, comandos e logs de compilação/execução.

O runner retorna 3 se nenhuma configuração pôde ser medida, e 1 quando
observou erro de correção, transformação exigida ausente ou uma matriz
apenas parcialmente medida. Ausência de
ferramenta ou falha de ligação é registrada como `skipped` com o motivo e
logs. Resultados parciais precisam ser lidos junto com `status.csv`;
sucesso de uma configuração não comprova conclusão da matriz C0–C3.

Para adição `flut`, os três vetores ocupam aproximadamente 12 MB em N=10⁶
e 120 MB em N=10⁷. Para produto escalar, os dois vetores ocupam 8 MB e
80 MB. Compare esses volumes aos caches registrados. Adição tem pouca
aritmética por byte e pode ficar limitada por banda de memória; produto
escalar serial pode sofrer com a dependência da acumulação. Custos de
threads, sincronização, acesso a memória e limites de frequência podem
impedir speedup proporcional ao número de threads. Essas são hipóteses
para interpretar medições, e não resultados observados.

## Estado verificado nesta implementação

Em 17/11/2026, o driver compilou para ambas as operações com GCC 16.1.0,
C99, `-Wall -Wextra -pedantic`, e o runner passou pela verificação sintática
do Bash. O driver também aceitou kernels auxiliares corretos, rejeitou um
resultado incorreto e rejeitou quatro repetições. Esses kernels auxiliares
validam apenas o mecanismo de medição e não geram resultados de desempenho
do compilador. A primeira execução de diagnóstico gerou CSV e SVG sem medições,
registrando pipeline indisponível naquele instante. O Clang identificado
foi 23.1.1 para `x86_64-pc-windows-msvc`; as ferramentas MLIR não foram
encontradas na instalação nativa inspecionada. Os caches informados por
`getconf` foram L1d=49.152 B, L2=2.097.152 B e L3=37.748.736 B.

**Não há números de aceleração comprovados nesta verificação inicial.**
A matriz C0–C3 precisa ser executada com a toolchain MLIR/LLVM e o runtime
OpenMP disponíveis. Só os CSV produzidos nessa execução permitem preencher
resultados e concluir a análise experimental da Parte C.
