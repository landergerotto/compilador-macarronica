# Plano do PjBL 3 e 4: frentes, contratos e cronograma

Situação em 14/11/2026 (sábado, 18h). Entrega: link do repositório na tarefa do
Canvas até **17/11/2026 às 18h** (o Canvas registra 21h UTC); fora do prazo, a
entrega é nula. Na mesma noite (Aula 16) há a apresentação: um programa inédito
do professor é compilado e executado ao vivo **no PC e na placa**, seguido de
arguição individual sobre qualquer parte (código, IR, pipeline MLIR, runtime da
placa, resultados). A **Prova 2** é em 24/11.

Peso na nota: cada RA vale 5 pontos de projeto e 5 de prova, com mínimo 7 por
RA. O PjBL 3 compõe o RA3 (30% da média) e o PjBL 4 o RA4 (40%). A linguagem é a
**Macaronica**; o que o enunciado pede só para a Zolang (paridade, `ehprimo`,
`c ? a ++ b .`) não se aplica.

## 1. Organização em frentes

```
develop ──► luiz/placa (integração) ──► develop ──► main      (só no fim, com PR)
               ▲
   ┌───────────┼──────────────┬───────────────┬────────────┐
luiz/linguagem  luiz/placa-esp32  luiz/dsl-mlir  luiz/bench  luiz/qa
```

Regras:

1. Ninguém commita na `develop` nem na `main`. O PR `luiz/placa` → `develop` →
   `main` acontece só no fim, com o OK do Luiz.
2. Cada frente trabalha na sua branch, saída da `luiz/placa`, e edita **só os
   arquivos que são dela** (Seção 3). Precisou mudar arquivo de outra frente?
   Pede à integração, que repassa o pedido ou autoriza um gancho curto.
3. Só a integração faz merge na `luiz/placa`, na ordem da Seção 5 e depois do
   portão de testes (Seção 4). Merges pequenos e frequentes: cada entrega
   pronta vira merge, sem esperar a frente terminar tudo.
4. Antes de começar uma entrega que depende de outra frente, a frente traz a
   `luiz/placa` atualizada para a sua branch (`git merge luiz/placa`).
5. Commits em português, no imperativo, dizendo o porquê, sem linhas de
   coautoria. Teste antigo não é apagado nem afrouxado para passar.
6. Cada frente mantém o seu capítulo em `docs/defesa/<frente>.md`: o que fez,
   por que fez assim, onde está no código e as perguntas prováveis da arguição
   com resposta. É o material de estudo do Luiz para a defesa.

## 2. Ferramentas fixadas

| Para quê | Ferramenta | Versão |
|---|---|---|
| Compilar o compilador no Windows | TDM-GCC (`gcc`) e `mingw32-make` | gcc 10.3.0, GNU Make 3.82.90 |
| PjBL 3, Partes B e C, `make test` completo | Ubuntu 26.04 no WSL: `build-essential`, `llvm-18`, `clang-18`, `mlir-18-tools`, `libomp-18-dev`, `python3-matplotlib` (apt) | LLVM/MLIR 18.1.8 |
| Placa (rota do Luiz) | Arduino IDE / `arduino-cli`, núcleo `esp32:esp32`, placa `esp32:esp32:esp32doit-devkit-v1` | arduino-cli 1.4.1, núcleo 3.3.11 (ESP-IDF 5.5), xtensa-esp-elf-gcc 14.2.0 |
| Placa (rota do Lander) | ESP-IDF (`esp32/`, `tools/esp32.ps1`) | 5.5.2 |

Comandos:

- Windows (Git Bash, com `C:\TDM-GCC-64\bin` no PATH): `mingw32-make`,
  `mingw32-make test-frontend`, `mingw32-make test-c`.
- Ubuntu (WSL), na mesma pasta do Windows: `make test`. O Makefile acha o LLVM
  18 em `/usr/lib/llvm-18/bin`. A partir do Git Bash:
  `MSYS_NO_PATHCONV=1 wsl --cd "$(pwd -W)" -e make test`.
- O Makefile principal só tem o núcleo (build e testes do PjBL 1 e 3). Cada
  parte do PjBL 4 tem os seus alvos em `mk/<parte>.mk` (`mk/placa.mk`,
  `mk/dsl.mk`, `mk/bench.mk`), e testes novos entram no `make test` com
  `TESTES_EXTRAS += <alvo>`.

## 3. Frentes: escopo, arquivos e entregas

### 3.1 `luiz/linguagem`: PjBL 3 e correção do PjBL 1/2

Rubrica do PjBL 3 inteira (corretude da IR 25%, cobertura 20%, tipos 15%, SSA
10%, testes 10%, arquitetura 10%, lacunas 10%).

**Arquivos da frente:** `src/lexer/`, `src/parser/`, `src/semantic/` (menos os
arquivos `dsl*`), `src/common/`, `src/compiler.*`, `src/main.c`,
`src/ir/codegen_llvm.*`, `runtime/zl_runtime.h`, `runtime/zl_runtime_posix.c`,
`tests/` (menos `esp32/`, `placa/`, `dsl/`, `qa/`, `professor/` e
`run_c_tests.sh`), `examples/` (menos `esp32_*`), `docs/decisoes.md`,
`docs/decisoes_backend.md`, `docs/otimizacoes.md`, `docs/diagnosticos.md`,
`docs/gramatica.md`, `docs/guia-defesa-pjbl3.md`.

Entregas, nesta ordem:

1. `inteira a, ;`, `inteira a, 5;` e `inteira a, vet{3} c;` deixam de travar o
   analisador (ponteiro nulo em `parse_declaration_after_type`): erro
   sintático e testes inválidos.
2. Cascatas: `inteira vet{n} v;`, `b receba 2 2;` e `NAO VER` passam a gerar um
   erro cada (recuperação com sincronização).
3. Fim das 13 linhas `// debug - remover`; mensagem de depuração só com opção
   explícita.
4. **Depois do merge do `int64_t` da frente da placa:** expressões mistas
   `inteira`/`flut`/`duplocarpado` (`2.0 ^ 2`, `f + 1`). A semântica anota cada
   conversão implícita e os dois back-ends só emitem o que foi anotado (LLVM:
   `sitofp`, `fpext`, `fptrunc`; C: conversão explícita). Pode editar
   `src/codegen_c.c` só nesse ponto.
5. `/` e `//` definidos para cada combinação de tipos; `^` e `raiz()` com tipo
   de retorno definido (intrínsecos `llvm.pow`/`llvm.powi`/`llvm.sqrt` ou libm);
   um teste dedicado a cada um.
6. Testes obrigatórios do PjBL 3 (6.1) com `.esperado`: aritmética com
   precedência e associatividade, condicionais aninhadas, as duas repetições,
   recursão, vetor, registro, E/S, expressões mistas, `^` e `raiz()`, `/` × `//`.
7. Todas as lacunas da Seção 4.3 do enunciado respondidas em
   `docs/decisoes_backend.md`, inclusive `inteira` em i64 (casos do professor
   com CPF) e `duplocarpado` na placa.
8. `docs/otimizacoes.md` regenerado com o LLVM 18 (mem2reg e `phi`; para o
   Excelente, comparação com `-O2`).

### 3.2 `luiz/placa-esp32`: Parte D (15%) e rota C

**Arquivos da frente:** `src/codegen_c.*`, `src/target.*`,
`runtime/zl_runtime_esp32.c` (e variantes da placa), `esp32/`, `tools/`,
`mk/placa.mk`, `tests/esp32/`, `tests/placa/`, `tests/run_c_tests.sh`,
`placa/`, `docs/placa.md`, `examples/esp32_*`.

Entregas, nesta ordem:

1. **Primeira, até sábado 22h:** `inteira` como `int64_t` no back-end C (hoje
   falham `12_inteira_64bits` e `erros/02_divisao_overflow` no `test-c`) e fim
   da falha intermitente "Permission denied" do `test-c` no Windows. Merge
   imediato: destrava a frente da linguagem.
2. Runtime ESP32 completo: todas as funções de `zl_runtime.h` (impressão com
   o mesmo formato do PC, leitura pela serial, potência, raiz, comparação de
   textos, GPIO, espera); erro de execução com a mesma mensagem do PC e o LED
   (GPIO2) piscando sem parar.
3. Rota Arduino, na máquina do Luiz: `make placa PROG=...` gera o C com o
   compilador, monta o sketch e compila com o `arduino-cli`, deixando o
   executável em `placa/<programa>/`; `make placa-gravar PORT=COMx` e
   `make placa-monitor`. A rota ESP-IDF do Lander continua funcionando.
4. Verificação de memória do `--alvo=esp32` coerente com a pilha da tarefa que
   executa `zl_principal` (números documentados).
5. Programas da placa em `tests/placa/`, com `.esperado` idêntico ao do PC:
   laço com vetor, recursão, erro de execução, E/S física (LED ou botão) e um
   rejeitado pela verificação de memória.
6. Script que captura a serial e compara com o `.esperado` (para segunda).
7. `docs/placa.md` completo: placa, tripla e `-mcpu`, ferramentas e versões,
   comandos, decisões dependentes do alvo (`duplocarpado`, numeração dos
   pinos, maior vetor), tamanhos de flash e RAM, vídeo de até 1 minuto
   (gravado na segunda).
8. Roteiro da segunda-feira, passo a passo, para o Luiz executar com a placa.

### 3.3 `luiz/dsl-mlir`: Partes A (20%) e B (25%)

**Arquivos da frente:** `docs/dsl.md`, `src/semantic/dsl.*` (novo),
`src/codegen_mlir.*` (novo), `scripts/pipeline.sh`, `tests/dsl/`,
`mk/dsl.mk`, seção "Parte B" de `docs/relatorio_otimizacao.md`. Com gancho
curto (uma chamada para o módulo novo): `src/parser/parser.c`,
`src/semantic/semantic.c`, `src/ir/codegen_llvm.c`, `src/codegen_c.c` e
`src/main.c`. O gancho na semântica entra depois das expressões mistas.

Entregas:

1. `docs/dsl.md`: tokens novos e produções (EBNF), regras de tipo (a promoção
   do PjBL 3 vale elemento a elemento), erros novos com prefixo próprio
   (`E-DSL-nnn`) e respostas às lacunas da Seção 5.5 do enunciado.
2. Semântica das operações `c receba a + b;`, `c receba 3 * a;`, `somavet(a)`,
   `maxvet(a)` e `prodesc(a, b)`: tamanhos ou tipos incompatíveis geram erro.
   Os testes antigos continuam passando.
3. MLIR com estratégia **híbrida**: o programa segue em LLVM IR e só os núcleos
   vetoriais saem em MLIR (`func`, `arith`, `memref`, `affine`, e `vector` via
   `affine-super-vectorize`), passam por `mlir-opt` e `mlir-translate` e são
   unidos com `llvm-link`. `scripts/pipeline.sh` com um comentário por passo,
   validado no MLIR 18.
4. Back-end C (placa): as mesmas operações como laços em C, já que o alvo não
   tem SIMD.
5. `tests/dsl/`: um programa por operação, comparado com a versão escalar
   escrita à mão na própria linguagem, e 2 inválidos; `make test` os executa.
6. Relatório, Parte B: o mesmo núcleo em três níveis do pipeline, com o que
   cada passo preservou e descartou.

### 3.4 `luiz/bench`: Parte C (25%) e reprodutibilidade

**Arquivos da frente:** `scripts/bench/`, `mk/bench.mk`, `Dockerfile`,
`.dockerignore`, seção "Parte C" de `docs/relatorio_otimizacao.md`.

Entregas:

1. Começa com núcleos MLIR escritos à mão (um elemento a elemento e uma
   redução ou produto escalar, com inteiro e `flut`), N = 10⁶ e 10⁷, e um
   driver em C que mede compilação e execução separadamente (mediana de 5).
   Medições no sistema de arquivos do Linux (cópia em `/tmp`), não em `/mnt/d`.
2. Configurações C0 a C3: C1 com tripla de alvo e `-Rpass`; C2 com
   `affine-super-vectorize`; C3 com OpenMP (`convert-scf-to-openmp` e
   `--canonicalize`) em 1, 2, 4 e 8 threads.
3. Pergunta obrigatória (redução de `flut` sem `-ffast-math`) respondida com
   evidência: `-Rpass-missed`, IR com e sem `reassoc`, tempo e diferença
   numérica.
4. Tabela e gráficos gerados por script; `make bench` em um comando; máquina
   descrita (i7-7700HQ, 4 núcleos e 8 threads, AVX2, sem AVX-512).
5. `Dockerfile` com os mesmos pacotes do WSL, rodando `make test` e
   `make bench`.
6. Quando a frente DSL/MLIR entrar, troca os núcleos à mão pelos gerados pelo
   compilador.

### 3.5 `luiz/qa`: garantia para o programa inédito

**Arquivos da frente:** `tests/qa/` e `scripts/qa/` (novos) e os `.esperado`
de `tests/professor/`. Não edita `src/` nem `runtime/`: cada bug vai para a
integração com o menor programa que o reproduz.

Entregas:

1. Casos do professor (`tests/professor/*.mcn`) como testes de execução, com
   `.esperado` conferido à mão.
2. Teste diferencial: o mesmo programa por `lli`, por `clang` e pela rota C no
   PC (e na placa, na segunda) precisa dar a mesma saída.
3. Mais de 20 programas no estilo dos casos do professor (níveis 3 a 5),
   cobrindo todas as construções, a DSL e casos de borda (limites de vetor,
   divisão por zero, overflow de i64, recursão profunda, textos, registros, E/S).
4. Robustez do front-end com ASan e UBSan no WSL (`make sanitize` e entradas
   mutadas): nenhum crash e nenhum vazamento.

## 4. Portão para merge na `luiz/placa`

- Windows: `mingw32-make` sem warnings; `test-frontend` e `test-c` sem falhas.
- Ubuntu: `make test` sem falhas, assim que o LLVM 18 estiver instalado;
  `make sanitize` limpo quando a mudança mexe no front-end ou no runtime.
- Nenhum teste antigo removido ou afrouxado sem justificativa no commit.
- `docs/defesa/<frente>.md` atualizado com a entrega.

## 5. Ordem de integração e contratos

| Quando | Merge | Destrava |
|---|---|---|
| sáb 22h | placa: `int64_t` no back-end C, `test-c` estável | linguagem (expressões mistas) |
| sáb noite | linguagem: crash, cascatas, `debug - remover` | qa (base sem crash) |
| dom manhã | linguagem: expressões mistas, `/` e `//`, `^` e `raiz()` | dsl-mlir (gancho na semântica) |
| dom tarde | dsl-mlir: DSL, MLIR, `pipeline.sh`, `tests/dsl` | bench (núcleos do compilador) |
| dom tarde | placa: runtime completo, rota Arduino, `tests/placa` | dia da placa |
| dom noite | bench e qa | congelamento às 23h |

Contratos entre frentes:

1. **`inteira` é i64 em tudo:** `i64` na IR, `int64_t` no C e no runtime.
2. **Conversões implícitas:** a regra é da frente da linguagem; a semântica
   anota e os back-ends emitem. A DSL usa a mesma função de promoção.
3. **Runtime:** `runtime/zl_runtime.h` é a única interface. Função nova entra
   no cabeçalho e no runtime POSIX (linguagem) e no runtime ESP32 (placa) no
   mesmo ciclo; a integração avisa a outra frente.
4. **Mesma saída no PC e na placa:** impressão de cada tipo e mensagem de erro
   de execução idênticas nos dois runtimes. A frente da placa garante isso; o
   formato comum aos dois está em `runtime/zl_formato.h`.
5. **Núcleos MLIR:** nome e assinatura publicados em `docs/dsl.md` (atenção à
   convenção de chamada de `memref` depois do lowering, ou
   `llvm.emit_c_interface`); a frente de benchmark usa os mesmos.
6. **Diagnósticos:** códigos novos seguem `docs/diagnosticos.md`; a DSL usa
   `E-DSL-nnn` e o alvo, `E-TGT-nnn`, para não colidir com `E-SEM`.
7. **Códigos de erro de execução:** 1 a 7 são os da linguagem (`ZL_ERRO_*` em
   `runtime/zl_runtime.h`, um teste cada em `tests/execucao/erros/`); 8 é
   `ZL_ERRO_PLACA`, uso inválido de pino ou espera (`runtime/zl_placa.h`).
   Código novo começa no 9 e é pedido à integração antes de usar, para duas
   frentes não pegarem o mesmo número.

Congelamento: domingo às 23h. Depois disso, só correção de bug com teste.

## 6. Cronograma

| Quando | Fase | O que acontece |
|---|---|---|
| sáb 14/11, tarde | 0. Fundação | `luiz/placa`, fim de linha LF, Makefile por parte, este plano, LLVM/MLIR 18 no WSL |
| sáb 14/11 noite → dom 15/11 tarde | 1. Construção | as cinco frentes em paralelo, com merges frequentes |
| dom 15/11 noite | 2. Integração | últimos merges, `make test` completo no WSL, revisão de código, congelamento |
| seg 16/11 | 3. Dia da placa | gravar, comparar a serial com o `.esperado`, medir tamanhos, gravar o vídeo, preencher `placa/` |
| seg 16/11 noite | 4. Ensaio | simulação da defesa com programa inédito e perguntas de arguição |
| ter 17/11 até 18h | 5. Entrega | PR `luiz/placa` → `develop` → `main` (com OK), link no Canvas (Luiz) |

## 7. Correção do PjBL 1/2: quem resolve

| Ponto da correção | Situação em 14/11 | Frente |
|---|---|---|
| `inteira a, ;` e variantes travam o analisador | ainda trava | linguagem (1) |
| Cascatas de erro | ainda ocorrem | linguagem (2) |
| `make` gera build de debug com `[debug]` no `stderr` | **corrigido**: `make` agora gera o release | — |
| 13 linhas `// debug - remover` | ainda | linguagem (3) |
| Sem promoção: `2.0 ^ 2`, `f + 1` | atribuição já promove; expressão ainda não | linguagem (4) |
| Fim de linha CRLF | **corrigido**: `.gitattributes` com LF | — |
| Aviso de fluxo Git no README | ainda | integração (README) |
| Declaração de uso de IA fora do modelo do Plano de Ensino | ainda | integração (README) |
| Blocos sem escopo próprio (D-014) | ainda; limita o Excelente | linguagem, se sobrar tempo |
| Poucos comentários; busca linear na tabela de símbolos | ainda | cada frente comenta o que tocar |
| 100.000 `+`, registro recursivo, `make test`, E/S, binários em `build/` | corrigidos antes | — |

## 8. Riscos

| Risco | Como está coberto |
|---|---|
| Placa só na segunda | rota C testada no PC (`test-c` com GPIO simulado) e sketch compilado pelo `arduino-cli` desde sábado; na segunda falta só gravar e conferir |
| LLVM/MLIR ausentes no WSL | instalação pelo apt (senha do Luiz); até lá, testes do Windows |
| Passes do MLIR 18 com nomes diferentes dos do enunciado | dsl-mlir valida o pipeline primeiro, antes da semântica |
| Conflitos de merge | arquivos com dono, ganchos curtos, `mk/<parte>.mk`, merges pequenos |
| Programa inédito usa algo não testado | frente qa e simulação da defesa na segunda à noite |
| Docker não instalado nesta máquina | `Dockerfile` espelha os pacotes do WSL, onde tudo é testado |
