# Decisões de backend — PjBL 3

Este contrato complementa as decisões D-012 a D-023 de `decisoes.md`.
As escolhas que preenchem lacunas são decisões da Macaronica deste projeto.
O código C99 gera LLVM IR textual diretamente da AST semanticamente aceita.
Não existe uma representação intermediária própria de três endereços.

## Tipos e portabilidade

| Macaronica | LLVM | Contrato |
|---|---|---|
| `bool` | `i1` | `0` é FAL e `1` é VER |
| `inteira` | `i32` | inteiro assinado de 32 bits |
| `flut` | `float` | IEEE 754 binary32 |
| `duplocarpado` | `double` | IEEE 754 binary64 |
| `palavra` | `ptr` | texto UTF-8 terminado em zero |
| `vazio` | `void` | ausência de valor de retorno |
| vetor | `[N x T]` | comprimento estático, índices começando em zero |
| registro | `%struct.nome` | campos por valor na ordem da declaração |

O módulo gerado não contém `target triple` nem `target datalayout`. Endereços
de vetores/campos usam GEP com índices `i32`, sem offsets manuais. Funções
geradas e declarações de runtime usam `nounwind`. Não há chamadas variádicas
nem chamadas diretas a `printf`/`scanf` na IR do programa.

O bitcode obtido ao compilar o runtime C pode conter triple e datalayout do
hospedeiro. Essa especialização ocorre somente ao ligar/executar o programa;
o `.ll` original permanece independente da plataforma. O PjBL 4 deve preservar
o significado de `double`, mesmo quando uma placa exigir implementação em
software. Nenhuma placa, ISA ou ABI de placa é escolhida nesta etapa.

## Memória, valores e conversões

Parâmetros e locais têm `alloca` no bloco `entry`, leitura por `load` e escrita
por `store`. O runtime não administra esses objetos. Todos os locais são
inicializados uma vez por chamada, antes do corpo; declarações em laços não
reinicializam armazenamento. Globais usam inicializador LLVM. Zero/FAL são
os valores padrão numéricos/lógicos; `ptr null` é a representação interna de
palavra vazia. Agregados podem usar `zeroinitializer`, pois impressão e
comparação textual tratam ponteiro nulo como texto vazio.

Literais de palavra viram constantes globais `[N x i8]` incluindo `\00`.
Comparação de palavras usa conteúdo (`zl_texto_igual`), nunca identidade do
ponteiro. Registros têm cópia, argumento e retorno por valor, com tipos
nominais e sem ciclos por valor; vetores completos não são valores de fonte.

As únicas promoções são `inteira -> flut`, `inteira -> duplocarpado` e
`flut -> duplocarpado`, em atribuições, argumentos e retornos. O codegen emite
`sitofp i32 ... to float`, `sitofp i32 ... to double` e `fpext float ... to
double`, respectivamente. A conversão para double a partir de inteira é
direta. Não há narrowing, cast de fonte nem promoção de operandos binários.

## Operações e controle

Operandos, argumentos e o destino de atribuição são avaliados conforme D-020.
`E` e `OU` usam curto-circuito com blocos e saltos; `XOR` avalia ambos os lados
e usa `xor i1`. Condicionais e laços formam um CFG com um terminador por bloco.
Depois, `mem2reg` pode promover os locais elegíveis para SSA e inserir `phi`.

`/` aceita somente dois `flut` ou dois `duplocarpado`, e emite `fdiv` após
guarda para divisor zero, inclusive zero negativo. `//` emite `sdiv i32`,
truncando em direção a zero, após guardas contra zero e `INT32_MIN / -1`.
Soma, subtração, multiplicação, menos unário e potência inteira são modulares
em 32 bits; a IR não usa `nsw`/`nuw`. Potência inteira usa exponenciação por
quadrados no runtime e rejeita expoente negativo. `0 ^ 0` é 1.

Potências reais usam `powf`/`pow`; raízes usam `sqrtf`/`sqrt` via runtime.
Raiz negativa e domínio inválido de potência são erros. Outros resultados
reais seguem IEEE 754, incluindo arredondamento, infinito e NaN; não é usado
`fast-math`. Comparações reais `<`, `>`, `==` são ordenadas; `!` é verdadeira
também quando houver NaN. O frontend já exige operandos de tipos iguais.

## ABI do runtime

`runtime/zl_runtime.h` define assinaturas fixas. A correspondência é:

| Função Macaronica | Símbolo LLVM/C | Assinatura LLVM |
|---|---|---|
| `imprimeInteira` | `zl_imprime_int` | `void (i32)` |
| `imprimeFlut` | `zl_imprime_flut` | `void (float)` |
| `imprimeDuplocarpado` | `zl_imprime_duplocarpado` | `void (double)` |
| `imprimePalavra` | `zl_imprime_texto` | `void (ptr)` |
| `imprimeBool` | `zl_imprime_bool` | `void (i32)` |
| `imprimeNl` | `zl_imprime_nl` | `void ()` |
| `leInteira` | `zl_le_int` | `i32 (i32 linha)` |
| `leFlut` | `zl_le_flut` | `float (i32 linha)` |
| `leDuplocarpado` | `zl_le_duplocarpado` | `double (i32 linha)` |

A linha é um argumento oculto emitido pelo compilador; as funções de leitura
não recebem argumentos na fonte. A impressão de bool usa `zext i1 ... to i32`
antes da chamada, evitando depender da ABI C de `_Bool` e atributos como
`zeroext`. O helper `i32 @zl_texto_igual(ptr, ptr)` retorna 0 ou 1; o codegen
transforma seu resultado em `i1` com comparação, invertendo para `!`.

Os demais auxiliares são:

```llvm
declare void @zl_erro_execucao(i32, i32) noreturn nounwind
declare i32 @zl_potencia_int(i32, i32, i32) nounwind
declare float @zl_potencia_flut(float, float, i32) nounwind
declare double @zl_potencia_duplocarpado(double, double, i32) nounwind
declare float @zl_raiz_flut(float, i32) nounwind
declare double @zl_raiz_duplocarpado(double, i32) nounwind
```

O último argumento matemático é a linha. O runtime não expõe parâmetros
variádicos. A função de erro termina o processo; o bloco LLVM que a chama
termina com `unreachable`.

| Código | Erro |
|---:|---|
| 1 | divisão por zero |
| 2 | overflow de divisão inteira |
| 3 | índice fora dos limites |
| 4 | raiz negativa |
| 5 | expoente inteiro negativo |
| 6 | entrada inválida, ausente ou fora da faixa |
| 7 | domínio inválido de potência real |

O diagnóstico usa `linha N: erro de execucao C: mensagem` em `stderr` e status
de falha. O gerador faz as guardas de divisão e índice antes da operação.

Inteiros são impressos em decimal assinado; `flut` com `%.9g`,
`duplocarpado` com `%.17g`, bool como `VER`/`FAL` e texto sem transformação.
Somente `imprimeNl` acrescenta nova linha. O processo começa com a locale C,
portanto usa ponto decimal. A leitura consome tokens separados por espaços;
reais de entrada admitem notação decimal com expoente opcional, sem hexadecimal,
NaN ou infinito. Tokens maiores que 255 bytes são rejeitados como entrada
inválida. Underflow real pode arredondar para subnormal ou zero; overflow é
rejeitado. Essas regras de `stdin` não ampliam os literais da linguagem.

## Entrada e retorno

`inteira principal()` e `vazio principal()` são normalizadas para
`define i32 @zl_principal() nounwind`. A primeira devolve o valor de `respost`;
a segunda devolve zero ao executar `respost;` ou chegar ao fim. Demais funções
`vazio` usam `ret void`. `main` pertence ao runtime POSIX, chama `zl_principal`
e devolve seu resultado. O sistema hospedeiro determina quantos bits do status
ficam observáveis pelo shell.

## Validação e execução

Utilize ferramentas da mesma versão principal do LLVM, com suporte a `ptr`
opaco (LLVM 15 ou superior). O fluxo abaixo liga o runtime somente depois de
validar o `.ll` gerado:

```sh
llvm-as programa.ll -o programa.bc
opt -passes=verify programa.ll -disable-output
clang -std=c99 -O0 -emit-llvm -c runtime/zl_runtime_posix.c -o runtime.bc
llvm-link programa.bc runtime.bc -o programa_com_runtime.bc
lli programa_com_runtime.bc
clang programa.ll runtime/zl_runtime_posix.c -lm -o programa
./programa
```

`llvm-link` reúne os módulos em bitcode e `lli` executa no próprio hospedeiro;
ele não emula uma placa. Consulte os manuais oficiais de
[llvm-link](https://llvm.org/docs/CommandGuide/llvm-link.html) e
[lli](https://llvm.org/docs/CommandGuide/lli.html). A biblioteca matemática do
hospedeiro precisa estar acessível ao JIT para os helpers reais; a execução
nativa liga explicitamente `-lm`. O comando de validação é um procedimento,
não uma afirmação de que todas as ferramentas já foram executadas no ambiente.

Para a análise didática, conserve a IR original e as saídas de:

```sh
opt -S -passes=mem2reg programa.ll -o programa_mem2reg.ll
opt -S -O2 programa.ll -o programa_O2.ll
```

A prova de equivalência prática compara a saída dos programas original,
promovido e otimizado com as mesmas entradas. A documentação de otimizações
deve explicar os predecessores de cada `phi`, relacionando-os ao CFG.

### Execução `lli` no Windows UCRT64

Na validação com LLVM/Clang MSYS2 UCRT64 22.1.8, o runtime em bitcode ligado
foi aceito por `llvm-link`, mas o JIT não resolveu de forma consistente as
funções matemáticas e de E/S do CRT. O teste Windows carrega o mesmo runtime
compilado como DLL nativa e chama diretamente a entrada do módulo:

```sh
clang -std=c99 -shared -DZL_RUNTIME_NO_MAIN runtime/zl_runtime_posix.c \
  -lm -Wl,--export-all-symbols -o build/zl_runtime_posix.dll
lli --entry-function=zl_principal \
  --dlopen=C:/caminho/absoluto/build/zl_runtime_posix.dll programa.ll
```

`ZL_RUNTIME_NO_MAIN` omite somente o `main` do runtime compartilhado;
compilação nativa e bitcode completo preservam seu `main` normalmente.
O teste ainda valida a montagem, o módulo e a ligação do bitcode no Windows.
Em POSIX, o roteiro `llvm-link` + `lli` acima continua sendo o modo padrão.
Os dois modos deixam o `.ll` gerado intacto, com os mesmos símbolos `zl_*`.
Isso é uma adaptação de carregamento do runtime no hospedeiro, sem seleção de
placa ou mudança na semântica Macaronica.

A análise com trechos reais e resultados de execução está em
[otimizacoes.md](otimizacoes.md); a preparação para arguição está em
[guia-defesa-pjbl3.md](guia-defesa-pjbl3.md).


## Rota C para ESP32 (PjBL 4, parte D)

`--emit-c` usa a mesma AST anotada da análise semântica e emite C99 diretamente,
sem passar pelo LLVM IR nem por uma IR própria. `--alvo=esp32` seleciona essa
saída e habilita GPIO/espera; o padrão continua gerando LLVM IR portátil.
O programa C define `int32_t zl_principal(void)`; o runtime ESP-IDF define
`app_main`, adapta os serviços `zl_*` e mantém detalhes de FreeRTOS/GPIO fora
do lexer, parser e semântica comum. Temporários C explicitam a ordem de
avaliação, e helpers usam aritmética unsigned para preservar wrap de i32.
Ver [placa.md](placa.md) para o blink do LED integrado e construção do firmware.
