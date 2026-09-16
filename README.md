# Compilador Macaronica

Front-end didatico da linguagem **Macaronica**, escrito em C99. Nesta etapa o
programa le um arquivo `.mac` e executa, nesta ordem, os analisadores lexico,
sintatico e semantico. O projeto nao implementa Zolang/Zoolang, interpretacao,
IR, otimizacao ou geracao de assembly.

## Preparacao no Windows

Use o terminal **MSYS2 UCRT64** e instale GCC e Make, se necessario:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make
```

Os executaveis esperados sao `C:\msys64\ucrt64\bin\gcc.exe` e
`C:\msys64\ucrt64\bin\mingw32-make.exe`. No terminal UCRT64:

```sh
mingw32-make
mingw32-make test
mingw32-make release
mingw32-make clean
```

Em Linux, os mesmos alvos podem ser usados com `make`. Toda compilacao usa
`-std=c99 -Wall -Wextra -pedantic`. O alvo padrao mantem os rastros temporarios
de debug desta primeira etapa; `release` desliga esses rastros.

## Uso

```text
build/debug/macaronica [--dump-tokens] [--dump-ast] [--dump-symbols] arquivo.mac
build/debug/macaronica --help
build/debug/macaronica --version
```

Sem opcoes de dump, uma fonte aceita produz uma confirmacao. Diagnosticos e
rastros de debug sao escritos em `stderr`; dumps e a confirmacao ficam em
`stdout`. Os dumps combinados aparecem na ordem tokens, AST e simbolos.

Codigos de saida:

| Codigo | Significado |
|---:|---|
| 0 | fonte aceita |
| 1 | erro lexico, sintatico ou semantico na fonte |
| 2 | uso da CLI, opcao ou extensao invalida |
| 3 | falha ao abrir ou ler o arquivo |
| 4 | falha interna, inclusive alocacao |

Exemplo:

```sh
build/debug/macaronica --dump-tokens examples/algo.mac
```

## Testes

`make test` percorre `tests/manifest.tsv`, confere aceitacao/rejeicao e a fase
dos diagnosticos, executa entradas truncadas, casos semanticos adicionais e
confere que os dumps sao deterministicos. O runner usa `timeout` quando
disponivel. `make sanitize` recompila com AddressSanitizer e
UndefinedBehaviorSanitizer e roda a mesma suite; esse alvo e ferramenta de
desenvolvimento e depende do suporte dos sanitizers no host.

A sintaxe e as lacunas resolvidas estao em `docs/gramatica.md` e
`docs/decisoes.md`. O ownership entre fases esta em `docs/pipeline.md`.

## Uso de inteligencia artificial

Este projeto foi desenvolvido com apoio de inteligencia artificial generativa
para estruturar o codigo, revisar contratos e ampliar os testes. As decisoes de
linguagem foram conferidas contra o enunciado da Macaronica; regras de
Zolang/Zoolang nao foram importadas.
