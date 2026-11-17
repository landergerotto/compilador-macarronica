# Compilador Macaronica

Este projeto implementa, em C99, o compilador da linguagem Macaronica. O
executável recebe um arquivo `.mac`, produz os tokens, monta a árvore sintática
abstrata (AST) e faz a análise semântica. Nessa última fase, confere nomes,
tipos, escopos, chamadas e retornos.

A etapa PjBL 3 gera LLVM IR textual diretamente da AST semanticamente
validada, com runtime C separado. O pipeline é fonte → lexer → parser/AST →
semântica → LLVM IR → ferramentas LLVM → execução no hospedeiro. A rota da
ESP32 (PjBL 4, parte D) reutiliza a AST validada para gerar C99 e firmware
ESP-IDF. MLIR, DSL vetorial e benchmarks ficam fora desta etapa.

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
| `make` | `mingw32-make` | Compila a versão de release (padrão), sem os rastros de debug |
| `make debug` | `mingw32-make debug` | Compila a versão de debug |
| `make release` | `mingw32-make release` | Compila a versão otimizada, sem os rastros de debug |
| `make test` | `mingw32-make test` | Compila e executa todos os testes: PjBL 1, LLVM IR (exige LLVM 18), rota C e os testes de `mk/*.mk` |
| `make test-frontend` | `mingw32-make test-frontend` | Executa somente a regressão do front-end, sem exigir LLVM |
| `make test-c` | `mingw32-make test-c` | Executa a rota C da placa (`--alvo=esp32`) no PC, com o runtime POSIX |
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
A suíte PjBL 1/3 passou 119/119 verificações, incluindo 82 regressões do
front-end, onze programas positivos, sete falhas de execução, entrada com
NUL e análise de otimizações. A rota C acrescenta 21/21 verificações, também
executadas por make test. A execução em Linux ainda não foi realizada
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

## ESP32: LED azul integrado, GPIO 2

O programa [examples/esp32_blink.mac](examples/esp32_blink.mac) configura o
GPIO 2 e alterna `VER`/`FAL`, com `espera(1000)` entre as mudanças. Usa o LED
azul programável da DOIT DevKit V1; o LED de alimentação não é controlado.

No PowerShell, na raiz do repositório:

```powershell
# Gera C e constrói o firmware (ESP-IDF instalado).
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action build
# Repete a geração/build e grava pela porta da placa.
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action flash -Port COM4
# Monitor serial: sair com Ctrl+].
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action monitor -Port COM4
```

Equivalentes com Make: `mingw32-make placa`,
`mingw32-make placa-gravar PORT=COM4` e
`mingw32-make placa-monitor PORT=COM4`. Para outra fonte, passe
`-Program caminho/programa.mac` ou `PROG=caminho/programa.mac`.

O script procura `IDF_PATH` ou `C:/Espressif/frameworks/esp-idf-v5.5.2`;
aceita `-IdfPath` para outra instalação. Gera
`build/esp32-generated/prog_gerado.c` e registra hashes da fonte, do compilador
e do C em `proveniencia.json`. O ESP-IDF compila esse C junto ao runtime;
`app_main` chama `zl_principal`. O build fica em `build/esp32`.
O caminho curto Windows 8.3 permite usar este repositório com espaços.

Para inspecionar apenas o C: `-Action generate` ou `mingw32-make placa-gerar`.
`mingw32-make test-c` verifica a rota C e simula ciclos do blink no host;
essa simulação não comprova que o LED físico acendeu.
ESP-IDF 5.5.2: firmware construído e gravado pela COM4; captura UART com oito
eventos alternados a aproximadamente 1 s. A luz do LED requer confirmação
visual. O runtime ESP32 desta etapa atende GPIO/espera e texto/nova linha;
serviços numéricos e de entrada ainda dependem do runtime POSIX.
Versões, evidências e limitações estão em [docs/placa.md](docs/placa.md).

## Declaração de uso de IA

Declaração padrão do Plano de Ensino (seção 6.1, Resolução nº 274/2024 CONSUN):

> Durante a preparação deste compilador (código, testes e documentação dos
> PjBL 1 a 4), os autores usaram Codex (OpenAI, versão [VERSÃO DO CODEX]),
> nas contribuições de Lander Furusho Gerotto, e Claude Code (Anthropic,
> modelo Claude Opus 5.5), nas contribuições de Luiz Toledo, para
> diagnosticar defeitos, implementar e revisar partes do código, escrever
> testes e documentação e executar as baterias de teste. Após usar essas
> ferramentas, os autores revisaram e editaram o conteúdo conforme necessário
> e assumem total responsabilidade pelo conteúdo.

Toda proposta das ferramentas passou pela revisão do grupo e pelas baterias de
teste antes de entrar no projeto. Os resultados de teste citados valem para as
ferramentas que de fato foram executadas e não são prova formal de correção.

### Apoio do Claude Code nas contribuições de Luiz Toledo: branch `luiz/placa` (PjBL 3 e 4)

| Parte do projeto | Em que consistiu o apoio |
|---|---|
| [`src/semantic/semantic.c`](src/semantic/semantic.c), [`src/semantic/type.c`](src/semantic/type.c) e os dois geradores | Expressões mistas `inteira`/`flut`/`duplocarpado`: tipo comum anotado na AST e conversões emitidas pelos back-ends |
| [`src/codegen_c.c`](src/codegen_c.c), [`src/target.c`](src/target.c), [`runtime/`](runtime) | Back-end C99 para a ESP32, verificações do alvo (pino e RAM) e runtime da placa |
| [`tools/placa.sh`](tools/placa.sh), [`tools/serial.ps1`](tools/serial.ps1), [`mk/placa.mk`](mk/placa.mk) | Gravação pela rota Arduino e `make placa-testar`, que compara a serial com o `.esperado` |
| [`src/compiler.c`](src/compiler.c) | Saída escrita num arquivo temporário ao lado do destino, que só substitui o arquivo final se a geração terminar |
| [`scripts/qa/`](scripts/qa), [`tests/qa/`](tests/qa), [`tests/execucao/`](tests/execucao) | Teste diferencial entre rotas, testes de mutação e os programas da Seção 6.1 |

### Apoio do Claude Code nas contribuições de Luiz Toledo: branch `luiz/melhorias`

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

## Documentação

| Documento | Conteúdo |
|---|---|
| [`docs/comandos.md`](docs/comandos.md) | Comandos PowerShell para executar no PC e gerar/gravar firmware na ESP32. |
| [`docs/decisoes.md`](docs/decisoes.md) | Semântica da Macaronica e extensões explícitas do projeto. |
| [`docs/gramatica.md`](docs/gramatica.md) | Gramática operacional. |
| [`docs/decisoes_backend.md`](docs/decisoes_backend.md) | Lacunas da Seção 4.3, expressões mistas, tipos LLVM, runtime e execução do PjBL 3. |
| [`docs/otimizacoes.md`](docs/otimizacoes.md) | Evidências de SSA, `mem2reg`, `phi`, `-O2` e equivalência. |
| [`docs/guia-defesa-pjbl3.md`](docs/guia-defesa-pjbl3.md) | Guia de código, conceitos das aulas e perguntas de arguição. |
| [`docs/diagnosticos.md`](docs/diagnosticos.md) | Catálogo dos códigos de erro léxico, sintático e semântico. |
| [`docs/plano-pjbl3-4.md`](docs/plano-pjbl3-4.md) | Situação, pendências e cronograma do PjBL 3 e 4 até 17/11. |
| [`docs/fluxo-git.md`](docs/fluxo-git.md) | Fluxo de branches do grupo (`develop` e `main`). |
