# Defesa: frente bench (Parte C do PjBL 4)

Material de estudo do Luiz para a arguição. Cada seção diz o que foi feito, por
que foi feito assim e onde está no repositório. O relatório formal é a Parte C
de `docs/relatorio_otimizacao.md`; os números saem de `make bench` em
`docs/bench/resultados.md`.

> **Estado:** a medição rodou em 17/11/2026 no WSL (LLVM/MLIR 18.1.8,
> i7-7700HQ, `-march=native` = skylake). Os números estão em
> `docs/bench/resultados.md` e a análise, nas seções C.5 a C.9 de
> `docs/relatorio_otimizacao.md`.

## 1. O que foi medido e por quê

Três núcleos, cada um em `inteira` (i64) e `flut` (f32), com N = 10⁶ e 10⁷:

| núcleo | operação | por que está aqui |
|---|---|---|
| `soma` | `c[i] = a[i] + b[i]` | elemento a elemento, sem dependência entre iterações: qualquer vetorizador e qualquer divisão em threads servem |
| `somavet` | `s = Σ a[i]` | redução: cada iteração depende da anterior pelo acumulador; é aqui que i64 e f32 se separam |
| `prodesc` | `s = Σ a[i]·b[i]` | redução com multiplicação: põe na mesa o FMA (f32) e a falta de multiplicação i64 no AVX2 |

**Dois tamanhos, duas situações.** Com 10⁶, um vetor f32 tem 4 MB e só `somavet`
f32 cabe na L3 (6 MiB); com 10⁷, tudo vem da DRAM (40 a 240 MB por chamada).
Esses núcleos fazem uma ou duas contas por elemento lido, então são limitados
pela banda de memória: o teto é 38,4 GB/s (DDR4-2400, dois canais), e na
prática uma thread sozinha não chega perto dele.

**Por que núcleos escritos à mão?** A frente dsl-mlir (Parte B) ainda não
estava integrada quando a medição foi preparada. Os núcleos estão em
`scripts/bench/nucleos/*.mlir.in`, no nível em que o pipeline começa (`affine` +
`arith` + `memref` estático). A metodologia não depende de quem escreveu o
núcleo: trocar pelos núcleos do compilador é trocar os arquivos `.mlir.in`.

## 2. Como a medição funciona (metodologia)

- **Driver em C** (`scripts/bench/driver.c`), compilado uma vez por núcleo,
  tipo e N, sempre com as mesmas opções (`clang -O2 -ffp-contract=off`). O
  núcleo vem de um objeto separado: o driver não consegue embutir o núcleo nem
  propagar constantes para dentro dele.
- **Dados fixos:** gerador splitmix64 com semente fixa; i64 em [−1000, 1000]
  (o produto escalar de 10⁷ termos cabe folgado em i64) e f32 em [0, 1) com 24
  bits (todo valor é exato em f32).
- **Cronômetro:** `clock_gettime(CLOCK_MONOTONIC)` só em volta da chamada. Antes,
  uma chamada de aquecimento descartada (cache e criação das threads do
  OpenMP). Depois, 5 chamadas cronometradas; a tabela mostra a **mediana**.
- **Compilação medida à parte** (`scripts/bench/medir.py`): cada etapa
  (`mlir-opt`, `mlir-translate`, `clang -c`) roda como processo 5 vezes.
- **Conferência de todo resultado** (seção 5 explica a tolerância de f32), e a
  coluna "estável" diz se as 5 chamadas deram os mesmos bits.
- **Medição numa cópia em `/tmp`**: no WSL2, `/mnt/d` passa pelo 9P, que é lento.
- **Tudo em um comando:** `make bench`. Se alguma conferência falha, ou falta
  uma mensagem `-Rpass` esperada, o comando falha e não sobrescreve
  `docs/bench/`.

## 3. O que cada configuração faz

| config | em uma frase |
|---|---|
| C0 | rebaixa o MLIR direto para LLVM e compila com `-O0`: tudo passa pela pilha, nada é otimizado. É a linha de base dos speedups. |
| C1-escalar | `-O2` com os vetorizadores desligados (`-fno-vectorize -fno-slp-vectorize`): mostra quanto vem só de registradores, `mem2reg` e laço limpo. |
| C1 | `-O2` normal: o vetorizador de laço do LLVM usa AVX2 (8 f32 ou 4 i64 por registrador) e intercala várias somas parciais. |
| C2 | a vetorização acontece no MLIR (`affine-super-vectorize`, vetores de 8), e o LLVM fica proibido de vetorizar. Compara o vetorizador do MLIR com o do LLVM. |
| C3 | `affine-parallelize` transforma o laço em `affine.parallel` (com redução), e `convert-scf-to-openmp` o transforma em OpenMP; roda com 1, 2, 4 e 8 threads. |

As cinco variantes da pergunta obrigatória (só `somavet` f32 e `prodesc` f32)
estão na seção 5. **Opções de CPU:** `-march=native` em todas as
configurações, como pede o enunciado (as mesmas em C1 e C2). Sem ela, o x86-64
básico só tem SSE2 (vetores de 128 bits) e nada de FMA. O que ela virou nesta
máquina está em `docs/bench/maquina.txt`.

**Passes exatos** (também gravados em `maquina.txt`):

- C0, C1, C1-escalar: `--lower-affine --convert-scf-to-cf --convert-arith-to-llvm
  --finalize-memref-to-llvm --convert-func-to-llvm=use-bare-ptr-memref-call-conv=1
  --convert-cf-to-llvm --reconcile-unrealized-casts`.
- C2: `--affine-super-vectorize="virtual-vector-size=8 vectorize-reductions=true"
  --canonicalize`, depois `--lower-affine --convert-vector-to-scf
  --convert-scf-to-cf --convert-vector-to-llvm` e as conversões para `llvm`.
- C3: `--affine-parallelize="max-nested=1 parallel-reductions=true"
  --lower-affine --convert-scf-to-openmp --canonicalize`, depois
  `--convert-scf-to-cf`, as conversões para `llvm` e `--convert-openmp-to-llvm`.
  Ambiente: `OMP_NUM_THREADS` = 1, 2, 4, 8; `OMP_PROC_BIND=spread`;
  `OMP_PLACES=cores`.

## 4. Como ler os gráficos (`docs/bench/*.png`)

- **`speedup_configuracoes.png`:** um painel por N; um grupo de barras por
  núcleo e tipo; cada barra é o speedup sobre C0 (C0 = 1×, a linha cinza). O
  eixo é logarítmico de base 2: cada linha de grade dobra o ganho, então 2× e
  16× ficam a distâncias comparáveis. O número em cima é o maior speedup do
  grupo.
- **`escalabilidade_threads.png`:** C3 com k threads dividido por C3 com 1
  thread. A diagonal cinza é o ideal (speedup = threads). Quando a curva
  achata, o limite é outro (banda de memória, núcleos físicos), não o número de
  threads.
- **`pergunta_reducao_flut.png`:** para N = 10⁷, cada variante das reduções f32
  tem duas barras: tempo (esquerda) e erro relativo à referência em `double`
  (direita), as duas em escala log. Azul: o resultado tem os mesmos bits da
  soma na ordem do programa. Laranja: o resultado mudou (a ordem das somas
  mudou, ou entrou um FMA).
- **`tempo_compilacao.png`:** quanto cada etapa (`mlir-opt`, `mlir-translate`,
  `clang -c`) leva por configuração. Cada etapa conta como processo inteiro
  (inclui carregar a ferramenta).
- As mesmas informações estão em tabela em `docs/bench/resultados.md`: tempos,
  speedups, GB/s, eficiência das threads, a tabela da pergunta e os tempos de
  compilação.

## 5. A pergunta obrigatória

**Pergunta:** por que a redução de `flut` não vetoriza sem `-ffast-math` (ou
`reassoc`), e a de `inteira` vetoriza?

**Resposta curta (para falar):** vetorizar uma redução é somar em outra ordem:
8 somas parciais, juntadas no fim. Com inteiros a soma é associativa, então o
resultado é o mesmo, bit a bit. Com ponto flutuante, cada soma arredonda, e a
ordem muda o resultado. A IR do LLVM sem *fast-math flags* promete a ordem do
programa, então o vetorizador recusa: "cannot prove it is safe to reorder
floating-point operations". Com `reassoc` (`-ffast-math` em C, ou
`fastmath<reassoc>` no MLIR), o programador autoriza a troca de ordem, e aí
vetoriza.

**Detalhes que podem ser perguntados:**

- **Onde está a permissão?** Em cada instrução (`fadd reassoc float`), não numa
  opção global. Por isso `-ffast-math` sobre o `.ll` vindo do MLIR
  (C1-ffast-math) não deve mudar nada: o clang não reescreve as instruções de
  uma IR de entrada. Em C (C-ffast-math), é o próprio clang que gera a IR e
  marca cada operação.
- **Dá para vetorizar sem mudar a ordem?** Dá: `-force-ordered-reductions`
  (C1-ordenada) carrega 8 elementos por vez, mas soma um a um em ordem. No x86
  isso vem desligado porque a cadeia de somas dependentes (latência de ~4
  ciclos por soma) continua lá.
- **O MLIR respeita isso?** O `affine-super-vectorize` com
  `vectorize-reductions=true` e o `affine-parallelize` com
  `parallel-reductions=true` aceitam `arith.addf` como redução sem olhar o
  `fastmath`. Então C2 e C3 mudam o resultado de f32, e o LLVM não mudaria. A
  coluna "mesmos bits" da tabela mostra isso.
- **O resultado reassociado é pior?** Não necessariamente: com somas parciais,
  os acumuladores ficam menores e o erro cresce mais devagar (parecido com a
  soma em pares). A soma na ordem do programa também erra. No produto escalar
  de 10⁷ termos em [0, 1)², a soma sequencial em f32 fica cerca de 1% abaixo
  do valor exato: quando o acumulador passa de alguns milhões, um termo menor
  que meio ulp do acumulador se perde inteiro. Isso foi visto numa execução
  preliminar do driver no Windows; o número oficial sai de `make bench`.
- **Por isso a tolerância de f32 no driver:** o resultado é aceito se
  |r − ref| ≤ max(2·|seq − ref|, 10⁻³·|ref|). Ou seja, o núcleo não pode errar
  mais que o dobro do que a própria ordem do programa erra.
- **E o FMA?** `a*b + s` num FMA arredonda uma vez em vez de duas e também muda
  os bits. Os núcleos MLIR usam `mulf` sem `contract`, e o C de referência sem
  `-ffast-math` usa `-ffp-contract=off`. Assim, quando muda o resultado de uma
  variante, a causa é a ordem das somas. Com `-ffast-math`, o FMA pode entrar
  junto.

**Evidências (`make bench`, 17/11/2026):** `docs/bench/evidencias/rpass/`
(mensagens de cada configuração), `docs/bench/evidencias/C1/somavet_f32.*` e
`docs/bench/evidencias/C1-reassoc/somavet_f32.*` (IR e assembly com e sem
`reassoc`), e a tabela "Pergunta obrigatória" em `docs/bench/resultados.md`
(tempos e diferença numérica).

## 6. Perguntas prováveis

**Por que mediana e não média?** Uma troca de contexto ou outro processo pode
deixar uma execução 2× mais lenta. A média absorve esse valor; a mediana de 5
ignora até 2 execuções ruins. O mínimo e o máximo também estão no CSV.

**Por que aquecer antes de medir?** A primeira chamada paga a criação das
threads do OpenMP, *page faults* e cache fria. Medir isso misturaria custo de
partida com custo do núcleo.

**Como garantir que o compilador não apagou a conta?** O núcleo está num objeto
separado (o driver não vê o corpo), o resultado vai para uma variável
`volatile` e é conferido depois: na `soma`, cada elemento de `c` é comparado
bit a bit com `a[i] + b[i]`.

**Por que existe o C1-escalar?** Para separar dois ganhos que o `-O2` mistura: o
escalar (valores em registrador em vez da pilha, laço limpo) e o vetorial
(SIMD). C1 ÷ C1-escalar é o ganho só da vetorização.

**O AVX2 não tem multiplicação de i64. E aí?** A instrução `vpmullq` é do
AVX-512DQ, que este processador não tem. No AVX2, o LLVM emula a multiplicação
64×64 com três `vpmuludq` (32×32→64) mais deslocamentos e somas. O modelo de
custo decide se ainda compensa vetorizar o `prodesc` i64; o `-Rpass` e o
assembly em `docs/bench/evidencias/` mostram a decisão.

**Por que C3 não escala até 8×?** (Hipótese; confirmar com os números.) Os
núcleos são limitados pela banda de memória. Duas ou três threads já ocupam boa
parte da banda que a máquina entrega, e as threads extras ficam esperando a
memória. Além disso, são 4 núcleos físicos: com 8 threads, cada par divide as
mesmas unidades de execução e a mesma L1/L2. No WSL2, as CPUs são virtuais e o
Hyper-V escolhe onde cada uma roda.

**Por que C2 poderia perder de C1?** (Hipóteses; conferir no assembly.) Dois
motivos prováveis:

- O `affine-super-vectorize` gera `vector.transfer_read` sem `in_bounds`, que
  pode virar carga com máscara.
- Nas reduções, ele usa um acumulador vetorial só. Cada iteração espera a
  soma anterior (latência de ~4 ciclos). O vetorizador do LLVM intercala 4
  acumuladores e esconde essa latência.

**O resultado de flut muda com o número de threads?** Pode mudar: cada thread
soma o seu pedaço, e as somas parciais se juntam no fim, então 2, 4 e 8
threads agrupam diferente. Com o mesmo número de threads, a coluna "estável"
diz se as 5 execuções deram os mesmos bits.

**O que é o GB/s da tabela?** Bytes que o núcleo precisa percorrer (vetores ×
N × tamanho do elemento) divididos pelo tempo. Na `soma`, o hardware ainda lê
`c` antes de escrever (*write-allocate*), então o tráfego real é 4 vetores, e
não 3. O número da tabela subestima o tráfego real em 25%.

**O Docker foi testado?** Não: o Docker não está instalado nesta máquina. O
`Dockerfile` instala os mesmos pacotes (e a mesma versão do LLVM) do WSL onde a
medição roda, e chama os mesmos alvos (`make test` e `make bench`).

## 7. Onde está cada coisa

| arquivo | papel |
|---|---|
| `scripts/bench/nucleos/*.mlir.in` | os 6 núcleos MLIR (`@N@` = tamanho, `@FMF@` = `fastmath` da variante) |
| `scripts/bench/driver.c` | inicializa, cronometra, confere e imprime uma linha `chave=valor` |
| `scripts/bench/referencia_c.c` | as reduções f32 em C (variantes C-O2 e C-ffast-math) |
| `scripts/bench/medir.py` | configurações, compilação cronometrada, execução, CSV, evidências e conferências |
| `scripts/bench/graficos.py` | tabelas (`resultados.md`) e os 4 gráficos |
| `scripts/bench/bench.sh` | cópia em `/tmp`, roda tudo e copia para `docs/bench/` se passar |
| `mk/bench.mk` | `make bench`, `make bench-rapido`, `make bench-teste` (no `make test`) |
| `Dockerfile`, `.dockerignore` | ambiente reproduzível (Ubuntu 26.04 + LLVM 18 preso) |
| `docs/bench/` | saída: `execucao.csv`, `compilacao.csv`, `maquina.txt`, `resultados.md`, PNGs, `evidencias/` |
