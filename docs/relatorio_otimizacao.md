# Relatório de otimização — PjBL 4, Partes B e C

> Esqueleto. Cada parte é escrita pela frente responsável (veja
> `docs/plano-pjbl3-4.md`): a Parte B pela frente DSL/MLIR (`luiz/dsl-mlir`) e a
> Parte C pela frente de benchmark (`luiz/bench`). Uma frente não edita a seção
> da outra.

## Parte B — Geração e lowering via MLIR

<!-- frente dsl-mlir: estratégia (híbrida) e justificativa; núcleo emitido pelo
compilador; o mesmo núcleo em três níveis do pipeline (affine, vector/scf,
dialeto llvm) com o que cada passo preservou e descartou; scripts/pipeline.sh. -->

## Parte C — Estudo experimental de otimização

Esta parte mede o que cada nível de otimização ganha em três núcleos, nos dois
tipos numéricos da linguagem. Os níveis são: otimização escalar, vetorização no
LLVM, vetorização no MLIR e paralelização com OpenMP. Tudo sai de um comando,
`make bench` (Linux/WSL), que grava os dados brutos, as tabelas, os gráficos e
as evidências em `docs/bench/`. Os números citados aqui vêm de
[`docs/bench/resultados.md`](bench/resultados.md), que o próprio `make bench`
gera.

### C.1 Máquina e ambiente

| | |
|---|---|
| CPU | Intel Core i7-7700HQ (Kaby Lake), 2,8 GHz, 4 núcleos físicos e 8 threads (Hyper-Threading) |
| SIMD | SSE4.2, AVX, AVX2 e FMA; **sem AVX-512** |
| Caches | L1d de 32 KiB e L2 de 256 KiB por núcleo; L3 de 6 MiB compartilhada |
| Memória | 2 × 8 GB DDR4-2400, um módulo por canal: pico teórico de 2 canais × 8 B × 2400 MT/s = 38,4 GB/s. O WSL enxerga 7,7 GiB |
| Sistema | Windows 10 com WSL2 (kernel 6.18.33.2-microsoft-standard-WSL2) e Ubuntu 26.04.1 LTS |
| Ferramentas | LLVM, Clang e MLIR 18.1.8 do apt (`llvm-18`, `clang-18`, `mlir-18-tools` e `libomp-18-dev`, versão `1:18.1.8-20ubuntu8`) |

O clang recebe `-march=native` em **todas** as configurações. O que essa opção
vira nesta máquina (CPU alvo e extensões) fica em `docs/bench/maquina.txt`. O
mesmo arquivo guarda a saída completa do `lscpu`, as versões das ferramentas,
os passes e as opções de cada configuração, e a carga da máquina no início e no
fim da medição.

### C.2 Núcleos e tamanhos

| núcleo | operação | vetores percorridos por chamada | forma no MLIR |
|---|---|---|---|
| `soma` | `c[i] = a[i] + b[i]` | 3 (lê `a` e `b`, escreve `c`) | `affine.for` com `affine.load` e `affine.store` |
| `somavet` | `s = Σ a[i]` | 1 | redução: `affine.for` com `iter_args` |
| `prodesc` | `s = Σ a[i] · b[i]` | 2 | redução: `affine.for` com `iter_args` |

Cada núcleo existe em `i64` (`inteira`) e em `f32` (`flut`), com N = 10⁶ e
N = 10⁷ elementos. Com N = 10⁶, um vetor ocupa 4 MB em f32 e 8 MB em i64, e só
`somavet` f32 (4 MB) cabe na L3 de 6 MiB. O resto, e tudo com N = 10⁷ (de 40 a
240 MB por chamada), vem da DRAM. São núcleos de streaming, com no máximo duas
operações aritméticas por elemento lido; o teto deles é a banda de memória, não
a CPU.

Os núcleos medidos foram escritos à mão em `scripts/bench/nucleos/*.mlir.in`,
no nível em que o pipeline da Parte B começa: `func`, `affine` e `arith` sobre
`memref` de forma estática (`memref<1000000xf32>`). Com a forma estática, o
número de iterações é constante (10⁶ e 10⁷ são múltiplos de 64), e nenhum
vetorizador precisa testar o tamanho em tempo de execução. A chamada usa
`--convert-func-to-llvm=use-bare-ptr-memref-call-conv=1`, então cada `memref`
chega ao núcleo como um ponteiro simples, e o driver em C chama o núcleo como
uma função comum.

### C.3 Configurações

| config | MLIR (`mlir-opt`) | clang (sobre o `.ll` do `mlir-translate`) | o que isola |
|---|---|---|---|
| C0 | só rebaixa: `lower-affine`, `convert-scf-to-cf` e as conversões para o dialeto `llvm` | `-O0` | a linha de base, sem otimização |
| C1-escalar | igual a C0 | `-O2 -fno-vectorize -fno-slp-vectorize` | o que o `-O2` ganha sem SIMD |
| C1 | igual a C0 | `-O2` (vetorizador de laço e SLP do LLVM) | a vetorização automática do LLVM |
| C2 | `affine-super-vectorize="virtual-vector-size=8 vectorize-reductions=true"`, `canonicalize`, depois `convert-vector-to-scf` e `convert-vector-to-llvm` | `-O2 -fno-vectorize -fno-slp-vectorize` | a vetorização feita no MLIR: o LLVM não vetoriza nada |
| C3 | `affine-parallelize="max-nested=1 parallel-reductions=true"`, `lower-affine`, `convert-scf-to-openmp`, `canonicalize` e `convert-openmp-to-llvm` | `-O2`, ligado com a `libomp` do LLVM 18 | o paralelismo de threads (1, 2, 4 e 8) |

Em todas: `-march=native`. De C1 em diante, o clang também recebe
`-Rpass=loop-vectorize -Rpass-missed=loop-vectorize
-Rpass-analysis=loop-vectorize`, e as mensagens ficam em
`docs/bench/evidencias/rpass/`. Em C3, `OMP_NUM_THREADS` vale 1, 2, 4 e 8, com
`OMP_PROC_BIND=spread` e `OMP_PLACES=cores`.

Cinco variantes, só para `somavet` f32 e `prodesc` f32, respondem à pergunta
obrigatória (seção C.8):

| variante | o que muda em relação a C1 |
|---|---|
| C1-reassoc | o núcleo MLIR marca a soma do acumulador com `fastmath<reassoc>`, que vira `fadd reassoc` na IR do LLVM |
| C1-ffast-math | o mesmo `.ll` de C1, com `-ffast-math` no clang |
| C1-ordenada | o mesmo `.ll` de C1, com `-mllvm -force-ordered-reductions` (vetoriza sem mudar a ordem das somas) |
| C-O2 | o núcleo escrito em C (`scripts/bench/referencia_c.c`), `clang -O2 -ffp-contract=off` |
| C-ffast-math | o núcleo em C, `clang -O2 -ffast-math` |

### C.4 Metodologia

- **Só a execução é cronometrada.** O driver (`scripts/bench/driver.c`) aloca
  os vetores alinhados em 64 bytes e os preenche com valores pseudoaleatórios
  de semente fixa (splitmix64; i64 em [−1000, 1000] e f32 em [0, 1) com 24
  bits). Depois chama o núcleo uma vez para aquecer (traz os vetores para a
  cache quando cabem e cria as threads do OpenMP) e cronometra 5 chamadas com
  `clock_gettime(CLOCK_MONOTONIC)` em volta de cada chamada. As tabelas usam a
  mediana das 5. O mínimo, o máximo e os 5 tempos ficam em
  `docs/bench/execucao.csv`.
- **O núcleo é um objeto separado.** Ele é compilado à parte e ligado ao driver,
  e o driver é o mesmo em todas as configurações (`clang -O2
  -ffp-contract=off`). Assim nenhuma configuração mexe no código que mede, e o
  núcleo não pode ser embutido no driver.
- **A compilação é medida à parte.** Cada etapa (`mlir-opt`, `mlir-translate` e
  `clang -c`) roda como processo 5 vezes, e a mediana vai para
  `docs/bench/compilacao.csv`.
- **Todo resultado é conferido** antes de entrar na tabela:
  - `soma`: bit a bit, contra `a[i] + b[i]`.
  - Reduções i64: igual à soma sequencial.
  - Reduções f32: dentro de uma tolerância em relação a uma referência em
    `double` com soma compensada (Kahan), |r − ref| ≤ max(2·|seq − ref|,
    10⁻³·|ref|), onde `seq` é a soma em f32 na ordem do programa. A tolerância
    é relativa ao erro da soma sequencial porque a própria ordem do programa
    erra (seção C.8).

  O driver também confere se as 5 chamadas devolveram os mesmos bits (coluna
  "estável"). O `make bench` falha, sem tocar em `docs/bench/`, se alguma
  conferência falhar ou se faltar uma mensagem `-Rpass` esperada. Por exemplo,
  C1 precisa vetorizar `somavet` i64 e recusar `somavet` f32.
- **A medição roda numa cópia em `/tmp`.** No WSL2, `/mnt/d` é montado via 9P,
  que é lento, e o tempo de compilação mediria o disco.
- **Ruído.** A carga da máquina (`/proc/loadavg`) é gravada em cada linha da
  medição (coluna `carga`).

### C.5 Resultados

A medição rodou em 17/11/2026, no WSL, com LLVM/MLIR 18.1.8 num i7-7700HQ
(4 núcleos, 8 threads, AVX2 e FMA). As 116 medições acertaram a soma de
referência. As tabelas completas (tempo, speedup, banda, dispersão, C3, a
pergunta obrigatória e o tempo de compilação) e os gráficos estão em
[`docs/bench/resultados.md`](bench/resultados.md).

Speedup sobre C0 com N = 10⁷ (mediana de 5 execuções):

| núcleo | C1-escalar | C1 | C2 | C3·4 |
| --- | ---: | ---: | ---: | ---: |
| soma i64 | 1,94× | 2,18× | 2,49× | 1,58× |
| soma f32 | 3,65× | 4,29× | 4,39× | 4,58× |
| somavet i64 | 5,91× | 6,63× | 6,42× | 10,91× |
| somavet f32 | 4,02× | 3,95× | 19,74× | 13,49× |
| prodesc i64 | 3,11× | 3,36× | 2,39× | 4,74× |
| prodesc f32 | 2,85× | 3,45× | 8,27× | 11,88× |

- **C0 → C1-escalar (2× a 6×):** no `-O0`, o back-end guarda os valores na
  pilha a cada bloco, e o acumulador e o índice vão e voltam da memória a cada
  iteração. O `-O2` os mantém em registradores e simplifica o laço.
- **C1-escalar → C1:** o vetorizador do LLVM ganha pouco nos inteiros
  (`somavet` i64: 5,91× → 6,63×) e nada nas reduções de flut (`somavet` f32:
  4,02× → 3,95×), que ele recusa (seção C.8).
- **C2 e C3** só se destacam nas reduções de flut, as duas que reassociam a
  soma (seções C.6 e C.7).

### C.6 C1 (vetorização no LLVM) × C2 (vetorização no MLIR)

- **Reduções de flut: C2 ganha de longe.** `somavet` f32 com N = 10⁷: 12,19 ms
  em C1 e 2,44 ms em C2 (5,0×); `prodesc` f32: 11,67 ms e 4,86 ms (2,4×). O
  `affine-super-vectorize` com `vectorize-reductions=true` troca o acumulador
  escalar por um `vector<8xf32>` (`docs/bench/evidencias/C2/*.vector.mlir`) e
  reassocia a soma sem pedir `fastmath`. O LLVM, em C1, recusa a mesma redução
  (seção C.8). O preço é o resultado: em `somavet` f32 com N = 10⁶, C2 dá
  499865,281 em vez de 499869,312. Os bits mudam, e o erro em relação à
  referência em `double` cai de 8,66e-06 para 5,96e-07.
- **Inteiros: C1 empata ou ganha.** `somavet` i64: 4,56 ms (C1) e 4,72 ms
  (C2); `prodesc` i64: 8,84 ms e 12,39 ms. Os dois vetorizam. O LLVM usa
  largura 4 e intercala 4 (`vectorization width: 4, interleaved count: 4`),
  com 4 acumuladores independentes. C2 usa um `vector<8xi64>`, que no AVX2 vira
  2 registradores de 4, e o `-fno-vectorize` de C2 impede o LLVM de intercalar
  por cima. No `prodesc` i64 pesa também a multiplicação de 64 bits, que o AVX2
  não tem numa instrução só (`vpmullq` é do AVX-512).
- **`soma` (c = a + b): empate.** 6,74 ms e 6,59 ms em f32 com N = 10⁷. O laço
  não tem dependência entre iterações e, com 3 vetores de 40 MB, o limite é a
  banda de memória, não a vetorização.

### C.7 Escalabilidade com OpenMP (C3)

Speedup de C3 sobre C3 com 1 thread (4 núcleos físicos; 8 threads usam o
Hyper-Threading):

| núcleo | N | 2 thr. | 4 thr. | 8 thr. |
| --- | ---: | ---: | ---: | ---: |
| somavet f32 | 10⁶ | 1,81× | 3,71× | 2,94× |
| prodesc f32 | 10⁷ | 2,02× | 3,99× | 3,49× |
| somavet i64 | 10⁷ | 1,54× | 1,62× | 1,70× |
| soma f32 | 10⁷ | 1,08× | 1,06× | 0,89× |

- **As reduções de flut escalam quase linearmente até 4 threads**, com
  eficiência de 79% a 100%. Com 1 thread, o laço é escalar e preso à latência
  da soma de flut: cada `fadd` espera o anterior. Com `parallel-reductions`,
  cada thread soma a sua faixa com o seu acumulador, e as 4 cadeias rodam em 4
  núcleos.
- **O resto quase não escala** (1,1× a 1,8× com 8 threads). Esses laços já
  saem vetorizados pelo LLVM em cada thread, e o limite é a banda de memória,
  que os núcleos dividem: mais threads não trazem mais banda.
- **8 threads não ajudam.** Com `OMP_PLACES=cores`, as threads 5 a 8 dividem
  núcleo com as 4 primeiras (Hyper-Threading) e disputam as mesmas unidades e
  a mesma banda. No pior caso, `somavet` f32 com N = 10⁷, 8 threads levam
  22,85 ms, contra 3,57 ms com 4 (seção C.9).

### C.8 Pergunta obrigatória: por que a redução de flut não vetoriza sem `-ffast-math`

1. **A soma de inteiros é associativa; a de ponto flutuante não.** Vetorizar uma
   redução muda a ordem das somas. Com 8 faixas, o laço acumula 8 somas
   parciais (s₀ = a₀ + a₈ + a₁₆ + …, s₁ = a₁ + a₉ + …) e as junta no fim; com
   *interleave*, são várias vezes 8. Em i64, a conta é módulo 2⁶⁴ e o resultado
   é idêntico em qualquer ordem, até com overflow. Em f32, cada soma arredonda
   para 24 bits de mantissa, então (a + b) + c pode diferir de a + (b + c):
   mudar a ordem muda o resultado.
2. **Sem *fast-math flags*, a IR do LLVM exige a ordem do programa.** Um `fadd`
   sem a flag `reassoc` tem de ser executado como está escrito (IEEE 754: um
   arredondamento por operação, na ordem). O vetorizador de laço reconhece a
   redução, mas a recusa com a mensagem `loop not vectorized: cannot prove it
   is safe to reorder floating-point operations`. O mesmo laço em i64 sai com
   `vectorized loop (vectorization width: …, interleaved count: …)`.
3. **O que libera a vetorização é a permissão de reassociar, não a opção em
   si.**
   - `reassoc` no `fadd`: no MLIR, `arith.addf … fastmath<reassoc>`
     (C1-reassoc); em C, `-ffast-math` ou `-fassociative-math`, que fazem o
     clang marcar cada operação ao **gerar** a IR (C-ffast-math).
   - `-ffast-math` sobre um `.ll` vindo do MLIR (C1-ffast-math) não deve
     bastar. As flags ficam em cada instrução, e o clang não reescreve as
     instruções de uma IR de entrada.
   - Uma redução vetorizada em ordem (C1-ordenada,
     `-force-ordered-reductions`) carrega 8 elementos por vez, mas soma um por
     um na ordem original. O x86 deixa isso desligado por padrão, porque a
     cadeia de somas dependentes continua lá.
   - O vetorizador `affine` do MLIR (C2) e o paralelizador (C3) reassociam a
     redução f32 sem pedir licença. Com `vectorize-reductions=true` e
     `parallel-reductions=true`, eles aceitam `arith.addf` como redução
     associativa, com ou sem `fastmath`.
4. **O resultado muda, mas não necessariamente para pior.** Somas parciais
   mantêm os acumuladores menores, e o erro cresce mais devagar, como numa soma
   em pares. A soma na ordem do programa não é "a certa": é só a que o programa
   escreveu. A seção de resultados compara as duas com a referência em
   `double`.

**Evidências (`make bench`, 17/11/2026), que confirmam os itens 2 a 4:**

- Mensagens do vetorizador (`docs/bench/evidencias/rpass/`). Em C1,
  C1-ffast-math e C-O2, as duas reduções de flut saem com `loop not
  vectorized: cannot prove it is safe to reorder floating-point operations`.
  Em C1-reassoc e C-ffast-math, saem com `vectorized loop (vectorization
  width: 8, interleaved count: 4)`. Os núcleos i64 vetorizam em C1.
- IR e assembly com e sem `reassoc`: `docs/bench/evidencias/C1/somavet_f32.*`
  e `docs/bench/evidencias/C1-reassoc/somavet_f32.*`.
- Tempos e resultado de `somavet` f32 com N = 10⁶ (tabela completa em
  `docs/bench/resultados.md`, seção "Pergunta obrigatória"):

| variante | ms | sobre C1 | resultado | erro relativo |
| --- | ---: | ---: | ---: | ---: |
| C1 | 1,153 | 1,00× | 499869,312 | 8,66e-06 |
| C1-ffast-math | 1,158 | 1,00× | 499869,312 | 8,66e-06 |
| C1-ordenada | 1,217 | 0,95× | 499869,312 | 8,66e-06 |
| C1-reassoc | 0,146 | 7,89× | 499864,938 | 9,03e-08 |
| C-ffast-math | 0,086 | 13,33× | 499864,938 | 9,03e-08 |
| C2 | 0,154 | 7,48× | 499865,281 | 5,96e-07 |

- C1-ffast-math dá os mesmos bits e o mesmo tempo de C1: o `-ffast-math` no
  clang não marca as instruções de um `.ll` que já chega pronto (item 3).
- C1-ordenada vetoriza, mas não ganha nada: soma um elemento por vez, na
  ordem do programa, e os bits não mudam.
- Com a reassociação, o resultado muda de 499869,312 para 499864,938 e fica
  cerca de 96 vezes mais perto da referência em `double` (item 4).

### C.9 Resultados inesperados

- **C-ffast-math é mais rápido que C1-reassoc** em `somavet` f32 (0,086 ms
  contra 0,146 ms com N = 10⁶), com os mesmos bits no resultado e a mesma
  vetorização (largura 8, intercalação 4). Não investigamos a diferença. A
  hipótese é o código em volta do laço: o núcleo do MLIR recebe o descritor do
  memref (base, deslocamento, tamanho e passo), e o do C recebe um ponteiro.
- **C2 perde de C1 em `prodesc` i64** (12,39 ms contra 8,84 ms com N = 10⁷): o
  vetorizador do MLIR não é sempre melhor que o do LLVM (seção C.6).
- **8 threads pioram `somavet` f32 com N = 10⁷**: 22,85 ms (0,49× sobre 1
  thread), contra 3,57 ms com 4. É a mediana de 5 execuções, então não é um
  ponto isolado. A hipótese é a disputa de núcleo do Hyper-Threading, somada
  ao WSL, que divide os núcleos com o Windows.
- **C3 com 1 thread perde de C1 em `soma` i64** (1,915 ms contra 1,208 ms com
  N = 10⁶): é o custo da região paralela da `libomp` e do laço reescrito pelo
  `convert-scf-to-openmp`, sem ganho de paralelismo para compensar.
- **Ruído:** a dispersão (máximo − mínimo) / mediana tem mediana de 16,9% e
  pior caso de 204,3% nas 116 medições. Por isso cada tempo é a mediana de 5,
  e as conclusões acima só se apoiam em diferenças bem maiores que esse ruído.

### C.10 Reprodutibilidade

- `make bench` refaz tudo: mede, confere e regrava os CSV, as tabelas, os
  gráficos e as evidências em `docs/bench/`. `make bench-rapido` mede só N = 10⁶
  e não toca em `docs/bench/`. `make bench-teste` (N = 4096) entra no
  `make test` e garante que todo pipeline compila, roda e acerta.
- O `Dockerfile` monta o mesmo ambiente do WSL (Ubuntu 26.04, os mesmos
  pacotes, LLVM preso em `1:18.1.8-20ubuntu8`) e roda `make test` e
  `make bench`. **Ele não foi testado:** o Docker não está instalado na máquina
  de desenvolvimento. A medição é feita no WSL, instalado com os mesmos
  pacotes e versões da imagem.
- O que se reproduz em outra máquina são as configurações, as conferências e a
  forma dos resultados: quem vetoriza e quais resultados mudam de bits. Os
  tempos absolutos dependem do hardware, que fica registrado em
  `docs/bench/maquina.txt`.
