# Compilador Macaronica

Este projeto implementa, em C99, o front-end da linguagem Macaronica. O
executável recebe um arquivo `.mac`, produz os tokens, monta a árvore sintática
abstrata (AST) e faz a análise semântica. Nessa última fase, confere nomes,
tipos, escopos, chamadas e retornos.

A versão atual valida o código-fonte. Geração de IR, otimizações e código para
a placa ainda não foram implementadas.

## Requisitos

- GCC
- GNU Make ou `mingw32-make`
- Bash para executar os testes

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
macaronica [--dump-tokens] [--dump-ast] [--dump-symbols] arquivo.mac
macaronica --help
macaronica --version
```

| Opção | Saída |
|---|---|
| `--dump-tokens` | Tokens produzidos pelo lexer |
| `--dump-ast` | AST produzida pelo parser |
| `--dump-symbols` | Tipos, escopos e símbolos produzidos pela análise semântica |
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
| 3 | Falha ao abrir ou ler o arquivo |
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

O resumo final tem este formato:

```text
develop -> branch-de-trabalho -> develop -> main
```

**Nunca desenvolver ou commitar diretamente na `main`.**

## Documentação

| Documento | Conteúdo |
|---|---|
| [`docs/guia-defesa-autoria.md`](docs/guia-defesa-autoria.md) | Guia para compreender e apresentar o front-end. |
| [`docs/diagnosticos.md`](docs/diagnosticos.md) | Catálogo dos códigos de erro léxico, sintático e semântico. |
