# Compilador Macaronica

Este projeto implementa, em C99, o compilador da linguagem Macaronica. O
executável recebe um arquivo `.mac`, produz os tokens, monta a árvore sintática
abstrata (AST) e faz a análise semântica. Nessa última fase, confere nomes,
tipos, escopos, chamadas e retornos.

A etapa PjBL 3 gera LLVM IR textual diretamente da AST semanticamente
validada, com runtime C separado. O pipeline é fonte → lexer → parser/AST →
semântica → LLVM IR → ferramentas LLVM → execução no hospedeiro. O PjBL 4
(placa, ISA, MLIR e otimizações próprias) permanece uma etapa futura.

## Requisitos

- GCC
- GNU Make ou `mingw32-make`
- Bash para executar os testes
- LLVM 15 ou superior com ponteiros opacos: `clang`, `llvm-as`, `llvm-link`,
  `opt` e `lli`, todos da mesma versão principal

A compilação usa C99 com `-Wall -Wextra -pedantic`.

## Preparação no Windows

O ambiente usado no desenvolvimento é o
[MSYS2](https://www.msys2.org/) com o terminal UCRT64. Instale as ferramentas
por esse terminal:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make
```

O Makefile procura o Bash em `C:\msys64\usr\bin\bash.exe`. Se o MSYS2 estiver
em outro diretório, ajuste a variável `BASH` ao chamar o Make.

No PowerShell, inclua as ferramentas no `PATH` da sessão:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
```

Depois, compile e execute o exemplo:

```powershell
mingw32-make debug
.\build\debug\macaronica.exe .\examples\algo.mac
```

Essa alteração do `PATH` vale somente para a janela atual do PowerShell.

No terminal MSYS2 UCRT64, use:

```sh
mingw32-make debug
./build/debug/macaronica.exe examples/algo.mac
```

## Compilação no Linux

Com GCC, Make e Bash instalados:

```sh
make debug
./build/debug/macaronica examples/algo.mac
```

## Alvos do Makefile

| Linux | Windows | Resultado |
|---|---|---|
| `make` | `mingw32-make` | Compila a versão de debug |
| `make debug` | `mingw32-make debug` | Compila a versão de debug |
| `make release` | `mingw32-make release` | Compila a versão otimizada, sem os rastros de debug |
| `make test` | `mingw32-make test` | Compila e executa os testes |
| `make test-frontend` | `mingw32-make test-frontend` | Executa somente a regressão do front-end, sem exigir LLVM |
| `make sanitize` | `mingw32-make sanitize` | Executa os testes com AddressSanitizer e UndefinedBehaviorSanitizer, quando disponíveis |
| `make clean` | `mingw32-make clean` | Remove o diretório `build/` |

Os executáveis ficam em `build/debug/`, `build/release/` ou
`build/sanitize/`, conforme o alvo. No Windows, o nome recebe a extensão
`.exe`.

O build de debug escreve mensagens iniciadas por `[debug]` em `stderr`. O build
de release desativa essas mensagens. Para recompilar tudo, use `make -B debug`
ou `mingw32-make -B debug`.

## Linha de comando

```text
macaronica [--dump-tokens] [--dump-ast] [--dump-symbols] arquivo.mac [-o saida.ll]
macaronica --help
macaronica --version
```

| Opção | Saída |
|---|---|
| `--dump-tokens` | Tokens produzidos pelo lexer |
| `--dump-ast` | AST produzida pelo parser |
| `--dump-symbols` | Tipos, escopos e símbolos produzidos pela análise semântica |
| `-o saida.ll` | Gera LLVM IR depois de aceitar a fonte |
| `--help` | Sintaxe da linha de comando |
| `--version` | Versão do compilador |

As opções de dump podem ser combinadas:

```powershell
.\build\debug\macaronica.exe --dump-tokens --dump-ast --dump-symbols .\examples\algo.mac
```

Os dumps seguem a ordem do pipeline: tokens depois do lexer, AST depois do
parser e símbolos depois da análise semântica. O dump da AST acontece antes das
anotações semânticas. Se a semântica rejeitar a fonte, `--dump-symbols` ainda
pode mostrar as tabelas parciais construídas até o erro.

Sem dumps, uma fonte aceita produz uma mensagem de confirmação em `stdout`.
Os dumps também usam `stdout`. Diagnósticos e mensagens de debug usam
`stderr`.

### Códigos de saída

| Código | Significado |
|---:|---|
| 0 | Fonte aceita |
| 1 | Erro léxico, sintático ou semântico |
| 2 | Uso incorreto da CLI, opção desconhecida ou extensão inválida |
| 3 | Falha ao ler a fonte ou escrever a saída |
| 4 | Falha interna, inclusive erro de alocação |

## Testes

No Windows:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
mingw32-make test
```

No Linux:

```sh
make test
```

A suíte percorre programas válidos e inválidos, confere a fase dos
diagnósticos, exercita a CLI e compara duas execuções de cada dump. Também há
casos de entrada truncada, profundidade excessiva, regras semânticas e retorno
em todos os caminhos de funções não vazias. O script usa um limite de dez
segundos por execução quando o comando `timeout` está disponível.

O teste completo também gera IR para os programas em `tests/execucao`,
valida com `llvm-as` e `opt -passes=verify`, liga o runtime com `llvm-link`,
executa com `lli` e compila/executa nativamente com Clang. Saída, entrada e
status são comparados com os arquivos de referência. Dependências ausentes
causam falha no teste completo; use `test-frontend` para testar só o front-end.

## Gerar e executar LLVM IR

```sh
./build/debug/macaronica tests/execucao/05_lacos.mac -o build/programa.ll
llvm-as build/programa.ll -o build/programa.bc
opt -passes=verify build/programa.ll -disable-output
clang -std=c99 -O0 -emit-llvm -c runtime/zl_runtime_posix.c -o build/runtime.bc
llvm-link build/programa.bc build/runtime.bc -o build/programa_ligado.bc
lli build/programa_ligado.bc
clang build/programa.ll runtime/zl_runtime_posix.c -lm -o build/programa
./build/programa
```

No Windows, acrescente `.exe` aos executáveis gerados. O LLVM/Clang do
MSYS2 UCRT64 usa a toolchain GNU instalada junto com ele. Para instalar os
componentes necessários no terminal MSYS2:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-llvm-tools
export PATH=/ucrt64/bin:$PATH
```

O instalador Windows do Clang pode não incluir `opt`, `lli` e `llvm-as`.
Confirme cada ferramenta com `--version`. Não misture bitcode de versões
principais diferentes. O módulo da Macaronica não fixa triple nem datalayout;
a compilação do runtime e a ligação escolhem o hospedeiro. O ponto de entrada
gerado é `@zl_principal`; `main` pertence ao runtime.

No PowerShell, use as ferramentas MSYS2 na sessão:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
mingw32-make test
```

O runner aceita `LLVM_BINDIR` ou os caminhos individuais `LLVM_AS`,
`LLVM_OPT`, `LLVM_LLI`, `LLVM_CLANG` e `LLVM_LINK`. No Windows, ele executa
`lli` com o runtime em DLL, pois o JIT UCRT64 testado apresentou problemas
de resolução do CRT no runtime em bitcode. O modo POSIX usa `llvm-link` e
`lli` sobre os módulos ligados. Os comandos manuais Windows estão em
[`docs/decisoes_backend.md`](docs/decisoes_backend.md).

Versões usadas nesta validação: GCC 16.2.0, LLVM/Clang 22.1.8, Windows x86_64
com MSYS2 UCRT64. As versões indicam o ambiente testado; LLVM 15+ é o mínimo
do formato de IR com `ptr`, não uma afirmação de teste de cada versão.
O teste completo passou 119/119 verificações, incluindo 82 regressões do
front-end, onze programas positivos, sete falhas de execução, entrada com
NUL e análise de otimizações. A execução em Linux ainda não foi realizada
neste ambiente; o runner mantém o fluxo POSIX por bitcode.

Os tipos são `bool → i1`, `inteira → i32`, `flut → float`,
`duplocarpado → double`, `palavra → ptr`, registros nominais e vetores de
comprimento fixo. As promoções numéricas autorizadas geram `sitofp`/`fpext`;
a fonte não tem casts. `E`/`OU` usam curto-circuito, `XOR` avalia ambos os
operandos, `/` divide reais e `//` divide inteiros com truncamento para zero.
Os detalhes de avaliação, inicialização, limites e erros estão em
[`docs/decisoes_backend.md`](docs/decisoes_backend.md).

## Otimizações e defesa

```sh
opt -S -passes=mem2reg build/programa.ll -o build/programa_mem2reg.ll
opt -S -O2 build/programa.ll -o build/programa_O2.ll
```

A IR inicial usa `alloca` no bloco `entry`, `load` e `store`. `mem2reg`
promove os locais elegíveis e cria `phi` nos encontros do CFG. Vetores,
registros e objetos cujo endereço precisa ser preservado podem permanecer
em memória. A análise deve comparar a mesma saída antes/depois e identificar
os predecessores de cada `phi`, sem atribuir transformações a passes não
observados. Não há uma IR própria de três endereços nem otimizador próprio.

## Uso de IA

O desenvolvimento usa Codex para diagnóstico, implementação, revisão,
documentação e execução de testes, com responsabilidades de arquitetura,
linguagem, IR, runtime, testes, otimização e revisão pedagógica. Propostas de
IA exigem revisão do grupo e validação pelas ferramentas. O grupo deve saber
explicar a AST anotada, os tipos, os blocos/terminadores, as guardas de runtime,
as conversões e o efeito de `mem2reg`/`phi`. Resultados de testes relatados
referem-se apenas às ferramentas efetivamente executadas, não a uma prova
formal de correção.

### Apoio de IA nas contribuições da branch `luiz/melhorias`

| Parte do projeto | Em que consistiu o apoio |
|---|---|
| [`src/semantic/semantic.c`](src/semantic/semantic.c) — `statement_always_returns` e `block_always_returns` | Identificação de que a verificação anterior apenas contava a existência de um `respost`, e implementação da análise de fluxo de controle que exige retorno em todos os caminhos |
| [`src/parser/parser.c`](src/parser/parser.c) — limite em cadeias de operadores | Identificação de um estouro de pilha com expressões muito longas e aplicação do limite de profundidade que já existia no projeto |
| [`src/lexer/token.c`](src/lexer/token.c), [`src/lexer/token.h`](src/lexer/token.h) e [`src/compiler.c`](src/compiler.c) — `token_print` | Correção do prefixo fixo que aparecia no lugar do caminho do arquivo em `--dump-tokens` |
| [`tests/run_tests.sh`](tests/run_tests.sh) e [`tests/semantic/invalido_retorno_parcial.mac`](tests/semantic/invalido_retorno_parcial.mac) | Casos de regressão correspondentes às correções acima |
| Revisão de código | Leitura da base em busca de defeitos, com verificação manual de cada achado antes de qualquer alteração |

Esta tabela cobre o apoio de IA nas contribuições da branch `luiz/melhorias`.
Cada integrante complementa a lista com o próprio uso nas demais partes do
projeto.

Todo o código entregue foi revisado pelos integrantes, que respondem por
qualquer trecho da implementação.

## Fluxo de trabalho

```text
develop -> branch-de-trabalho -> develop -> main
```

**Nunca desenvolver ou commitar diretamente na `main`.**

## Documentação

| Documento | Conteúdo |
|---|---|
| [`docs/decisoes.md`](docs/decisoes.md) | Semântica da Macaronica e extensões explícitas do projeto. |
| [`docs/gramatica.md`](docs/gramatica.md) | Gramática operacional. |
| [`docs/decisoes_backend.md`](docs/decisoes_backend.md) | Tipos LLVM, runtime, portabilidade e execução do PjBL 3. |
| [`docs/otimizacoes.md`](docs/otimizacoes.md) | Evidências de SSA, `mem2reg`, `phi`, `-O2` e equivalência. |
| [`docs/guia-defesa-pjbl3.md`](docs/guia-defesa-pjbl3.md) | Guia de código, conceitos das aulas e perguntas de arguição. |
| [`docs/diagnosticos.md`](docs/diagnosticos.md) | Catálogo dos códigos de erro léxico, sintático e semântico. |
