# Plano do PjBL 3 e 4 — situação e o que falta

Situação em 14/11/2026. Entrega: link do repositório na tarefa do Canvas até
**17/11/2026 às 18h** (o Canvas registra 21h UTC); entrega fora do prazo é nula.
Na mesma noite (Aula 16) há a apresentação, com um programa inédito do professor
compilado e executado ao vivo no PC e na placa. A **Prova 2** é em 24/11 (Aula 17).

Branch de trabalho: `backend/luiz/pjbl3-4` (base: `backend/lander/LLVM-IR` +
`luiz/melhorias`). Fluxo: branch de trabalho → `develop` → `main`.

## Linguagem e peso na nota

- A linguagem é a **Macaronica**. A especificação (`Zolang_Macaronica.pdf`,
  Seção 3) destina a Macaronica a Linguagens Formais e Compiladores e a Zolang a
  Construção de Interpretadores. O enunciado do PjBL 3/4 serve às duas
  disciplinas; os itens só da Zolang (paridade, `ehprimo`) não se aplicam.
- Pelo Plano de Ensino, cada RA vale 5 pontos de projeto e 5 de prova, e é
  preciso tirar 7 em cada RA. O PjBL 3 compõe o RA3 (30% da média) e o PjBL 4,
  o RA4 (40%). A outra metade de RA3 e RA4 vem da Prova 2.

## 1. O que já temos

| Área | Estado |
|---|---|
| Front-end (PjBL 1/2) | lexer, parser, semântica; 88 testes passando (`make test-frontend`), incluindo os 5 casos do professor em `tests/professor/` |
| Back-end LLVM IR | todas as construções do escopo mínimo (4.1): locais/parâmetros com `alloca`, globais, aritmética, `icmp`/`fcmp`, `E`/`OU` com curto-circuito, `cond`/`casocontrario`, `repete`/`durante`, funções e recursão, vetores com verificação de limites, registros, textos, E/S via `zl_*` |
| Regras de IR portável (4.5) | sem triple/datalayout, tipos de largura fixa, sem variádicas, GEP com índices `i32`, `nounwind`, `@zl_principal` |
| Runtime | `runtime/zl_runtime.h` + `zl_runtime_posix.c`, guardas de execução com códigos 1–7 |
| Testes de execução | 12 programas em `tests/execucao/` + 7 de erro em `tests/execucao/erros/` |
| Documentação | `decisoes.md`, `decisoes_backend.md`, `otimizacoes.md` (mem2reg/phi), `guia-defesa-pjbl3.md` |

Feito nesta branch (03/11):

- `inteira` passou a **64 bits** (`i64`): os casos `nivel3`–`nivel5` do professor
  guardam CPFs de 11 dígitos e eram rejeitados (E-SEM-037). Semântica, IR,
  runtime, testes e documentação atualizados (D-021 revisada).
- **IR legível**, como pede a seção 4.4: `%n.addr`, `%contador.N`, `%fatorial.7`,
  `@chamadas`, um `; linha N` por linha de origem; globais `internal`.
- O compilador aceita **`.mcn`** (extensão dos casos do professor) além de `.mac`.
- `build/` saiu do índice do Git (já estava no `.gitignore`).

## 2. Correção do PjBL 1/2

O enunciado (Seção 1) manda corrigir os erros de front-end apontados na correção
do PjBL 1/2 antes de gerar código. Situação de cada ponto, conferida em 14/11
com o build de release:

| Ponto da correção | Situação em 14/11 |
|---|---|
| `inteira a, ;`, `inteira a, 5;` e `inteira a, vet{3} c;` derrubam o analisador (ponteiro nulo em `parse_declaration_after_type`) | **ainda trava** (segmentation fault) |
| 100.000 `+` encadeados estouram a pilha | corrigido (E-SIN-009) |
| Registro que contém a si mesmo é aceito | corrigido (E-SEM-039) |
| Não há `make test` nem resultado esperado dos testes | corrigido (`make test`, 88 testes) |
| Não há entrada e saída | corrigido (intrínsecas da Seção 8.1 da gramática, D-017) |
| Sem promoção: `flut f receba 2;` e `2.0 ^ 2` são erro | atribuição corrigida; `2.0 ^ 2` e `f + 1` ainda dão E-SEM-017 (item 5 da Seção 3) |
| Cascatas: `inteira vet{n} v;` gera 5 erros; `b receba 2 2;` e `NAO VER`, 2 cada | ainda ocorrem |
| `make` gera o build de debug, com linhas `[debug]` no `stderr` | ainda |
| 13 linhas marcadas `// debug - remover` em `src/` | ainda |
| Aviso de fluxo Git no README | ainda (seção "Fluxo de trabalho") |
| Declaração de uso de IA | existe, mas não segue o modelo do Plano de Ensino (6.1) nem cita todas as ferramentas usadas |
| Blocos não criam escopo (D-014) | ainda; limita o nível Excelente em escopo |
| `.c` quase sem comentários; busca linear na tabela de símbolos | ainda |
| Binários versionados em `build/` | corrigido |
| Fim de linha CRLF em todos os arquivos | ainda; scripts `.sh` com CRLF não rodam no Linux, falta um `.gitattributes` |

## 3. O que falta no PjBL 3

| # | Item | Situação |
|---|---|---|
| 1 | Validar no Linux com **LLVM/MLIR 18**: `llvm-as`, `opt -passes=verify`, `lli` e `clang saida.ll zl_runtime_posix.c` | LLVM/MLIR 18, gcc e make ainda não estão instalados no WSL |
| 2 | Regerar os exemplos de `otimizacoes.md` (nomes novos e `i64`) | depende do item 1 |
| 3 | Fixar a versão no README e criar `Dockerfile` (LLVM/MLIR 18) | a fazer |
| 4 | Casos do professor como testes de **execução** (`.esperado`/`.status`) | hoje só compilam; depende do item 1 |
| 5 | **Expressões mistas** inteira/flut/duplocarpado (4.2 e 6.1) | hoje só há promoção em atribuição, argumento e retorno. **Recomendado fazer**: o enunciado pede, e a correção do PjBL 1/2 já apontou a tipagem estrita como rígida |
| 6 | Teste dedicado à diferença entre `/` e `//` (ex.: `7.0 / 2.0` vs `7 // 2`) e conferir se há condicional aninhada | o 01 mostra os dois operadores com operandos diferentes |

## 4. O que falta no PjBL 4

**Parte A — DSL vetorial (20%).** Sintaxe sugerida pelo enunciado:
`c receba a + b;`, `c receba 3 * a;`, `somavet(a)`, `maxvet(a)`, `prodesc(a, b)`.
Falta: tokens e produções em `docs/dsl.md`; semântica (tamanhos e tipos
incompatíveis são erro; promoções do PjBL 3 valem para operações mistas);
respostas às lacunas 5.5 (vetor de tamanho só conhecido em execução, destino
igual a um operando, redução de vetor vazio ou unitário); `tests/dsl/` com um
programa por operação comparado com a versão escalar e 2 inválidos.

**Parte B — MLIR (25%).** Estratégia recomendada: **híbrida** (o programa segue
em LLVM IR; só os núcleos vetoriais saem em MLIR com `func`/`arith`/`memref`/
`affine`, viram LLVM IR com `mlir-translate --mlir-to-llvmir` e são unidos com
`llvm-link`). Falta: `src/codegen_mlir.c`, `scripts/pipeline.sh` com um
comentário por passo, e o mesmo núcleo mostrado em 3 níveis do pipeline.

**Parte C — estudo de otimização (25%).** Dois núcleos (um elemento a elemento
e uma redução/produto escalar), vetores de 10⁶ e 10⁷, configurações C0–C3
(C3 com OpenMP: `convert-scf-to-openmp` + `--canonicalize` no MLIR 18), mediana
de 5 execuções separando compilação e execução, máquina descrita (i7-7700HQ,
4 núcleos/8 threads, AVX2), tabela e gráfico gerados por script em
`scripts/bench/`, `make bench`, e a pergunta obrigatória sobre a redução de
`flut` sem `-ffast-math`, com evidência (`-Rpass`, IR ou medição). Relatório
em `docs/relatorio_otimizacao.md`.

**Parte D — placa ESP32 clássico, Xtensa (15%).** Rota planejada: o LLVM da
Espressif compila o mesmo `.ll` para Xtensa e o link usa o núcleo Arduino-ESP32;
alternativa aceita pelo enunciado: um back-end C. Falta: `zl_runtime_esp32.c`
(saída pela serial e LED piscando em erro), funções embutidas `pino_modo`,
`pino_escreve`, `pino_le` e `espera`, `--alvo=esp32` com erro semântico quando
os locais de uma função excedem a RAM (pilha de 8 KB da tarefa do Arduino),
`make placa PROG=...`, executáveis em `placa/`, `docs/placa.md` com vídeo de até
1 minuto, 3 programas iguais aos do PC (laço+vetor, recursão, erro de execução),
1 com E/S física e 1 rejeitado pela verificação de memória. **Avisar o professor
da placa escolhida.**

**Infraestrutura (15% do PjBL 4).** `make test` passando a incluir `tests/dsl/`,
`make bench`, `make placa`, `Dockerfile`, README com versões e uso de IA.

## 5. Prioridades até 17/11

Com três dias, a ordem abaixo segue o peso na nota e as dependências:

1. **14/11:** instalar LLVM/MLIR 18 no WSL; corrigir o crash do parser e os
   itens simples da Seção 2 (build padrão, `debug - remover`, README,
   `.gitattributes`); expressões mistas.
2. **15/11:** validar o PjBL 3 no Linux (`make test`, casos do professor como
   testes de execução, `Dockerfile`); Parte A (DSL).
3. **16/11:** Parte B (MLIR e `pipeline.sh`) e Parte C (medições, gráficos e
   relatório). Parte D em paralelo, com quem estiver com o ESP32.
4. **17/11:** ensaio da demo, PR para `develop` e depois `main`, link no Canvas
   antes das 18h, apresentação.
5. **24/11:** Prova 2.

As partes A–D do PjBL 4 só cabem no prazo se forem divididas entre os quatro
integrantes.

## 6. Decisões em aberto

1. Expressões mistas: recomendado fazer (Seção 3, item 5).
2. Estratégia MLIR: híbrida (recomendada) ou programa inteiro em MLIR.
3. ESP32: LLVM da Espressif (mesmo `.ll`) ou back-end C.
4. Divisão das partes entre os integrantes.
5. O `plataformas.zip` das Aulas 14/15 não foi encontrado no Canvas nem no Notion.
