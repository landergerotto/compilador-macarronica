# Decisões de backend — PjBL 3

Este contrato complementa as decisões D-012 a D-023 de `decisoes.md`.
As escolhas que preenchem lacunas são decisões da Macaronica deste projeto.
O código C99 gera LLVM IR textual diretamente da AST semanticamente aceita.
Não existe uma representação intermediária própria de três endereços.

## Lacunas da Seção 4.3 (resumo)

| Pergunta do enunciado | Decisão | Detalhe |
|---|---|---|
| Promoção entre `inteira`, `flut` e `duplocarpado`; `/` e `//` em cada combinação | Só alarga, na ordem `inteira < flut < duplocarpado`: em atribuição, argumento, retorno e expressão mista. `/` é sempre real (`inteira / inteira` é feita em `duplocarpado`); `//` só aceita `inteira`. Nunca estreita, então não há `fptrunc` | Expressões mistas; D-011 e D-012 |
| `E` e `OU` usam curto-circuito? Como sai o `XOR`? | Sim, com blocos e `phi`. `XOR` avalia os dois lados e vira `xor i1` | Operações e controle |
| O que é `respost` em `principal`? | Código de saída do processo, reduzido a 32 bits; `vazio principal` sai com 0 | Entrada e retorno |
| Divisão por zero, overflow inteiro, vetor fora dos limites | Divisão e índice são verificados antes da operação e abortam com status de falha e `linha N: erro de execucao C` na saída de erro (na placa, mensagem na serial e LED piscando). `+`, `-`, `*` e `^` em `inteira` são módulo 2^64, sem comportamento indefinido | Validação e execução |
| Formato de impressão | `inteira` em decimal; `flut` com `%.9g`; `duplocarpado` com `%.17g`; `bool` como `VER`/`FAL`; só `imprimeNl` quebra a linha | Validação e execução |
| Largura dos tipos e `duplocarpado` sem `double` de 64 bits | `inteira` é i64 e `duplocarpado` é binary64 em qualquer alvo. Na ESP32, cuja FPU só faz 32 bits, o `double` roda em software (libgcc) e nunca vira `float`. Não há alvo AVR; num alvo sem `double` de 64 bits, a regra seria recusar `duplocarpado` no alvo, como já se recusam GPIO fora da placa (E-TGT-001) e falta de RAM (E-TGT-002); pino inválido é o erro de execução 8 | Tipos e portabilidade; Rota C para ESP32 |

## Tipos e portabilidade

| Macaronica | LLVM | Contrato |
|---|---|---|
| `bool` | `i1` | `0` é FAL e `1` é VER |
| `inteira` | `i64` | inteiro assinado de 64 bits (D-021, revisão do PjBL 3/4) |
| `flut` | `float` | IEEE 754 binary32 |
| `duplocarpado` | `double` | IEEE 754 binary64 |
| `palavra` | `ptr` | texto UTF-8 terminado em zero |
| `vazio` | `void` | ausência de valor de retorno |
| vetor | `[N x T]` | comprimento estático, índices começando em zero |
| registro | `%struct.nome` | campos por valor na ordem da declaração |

O módulo gerado não contém `target triple` nem `target datalayout`. Endereços
de vetores/campos usam GEP com índices `i32`, sem offsets manuais: o índice
`inteira` é verificado em `i64` e só depois reduzido com `trunc`. Funções
geradas e declarações de runtime usam `nounwind`. Não há chamadas variádicas
nem chamadas diretas a `printf`/`scanf` na IR do programa.

O bitcode obtido ao compilar o runtime C pode conter triple e datalayout do
hospedeiro. Essa especialização ocorre somente ao ligar/executar o programa;
o `.ll` original permanece independente da plataforma. O PjBL 4 deve preservar
o significado de `double`, mesmo quando uma placa exigir implementação em
software. A placa do PjBL 4 (ESP32, Xtensa LX6) está em "Rota C para ESP32".

## Nomes na IR

A IR é escrita para ser lida. Os nomes derivam da fonte e não colidem entre si,
porque um `IDENT` da Macaronica não contém ponto:

| Forma | Uso | Exemplo |
|---|---|---|
| `@nome` | global (`internal`, não conflita com símbolos do runtime ou da placa) | `@chamadas` |
| `@mac.nome` | função do programa | `@mac.fatorial` |
| `%nome.addr` | `alloca` de local ou parâmetro | `%contador.addr` |
| `%nome` | valor recebido pelo parâmetro | `%n` |
| `%nome.N` | `load` de variável ou resultado de chamada | `%n.2`, `%fatorial.7` |
| `%t.N` | demais temporários | `%t.3 = icmp slt i64 %n.2, 2` |
| `a.b.N` | rótulos de bloco (`if.then.1`, `guarda.erro.0`) | `loop.body.4` |

`N` vem de um único contador por função. Um parâmetro chamado `entry` recebe o
sufixo `.arg`, pois esse é o único nome simples de rótulo. Antes do primeiro
comando de cada linha de origem, o gerador escreve `; linha N` uma vez.

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
`flut -> duplocarpado`, em atribuições, argumentos, retornos e nos operandos
de expressões mistas. O codegen emite `sitofp i64 ... to float`,
`sitofp i64 ... to double` e `fpext float ... to double`, respectivamente
(no C99, `(float)` e `(double)`). A conversão para double a partir de inteira
é direta. Não há estreitamento nem cast de fonte, então `fptrunc` e `fptosi`
nunca aparecem.

### Expressões mistas (D-011, D-012)

Num operador binário, o tipo comum é o mais largo dos dois operandos na ordem
`inteira < flut < duplocarpado`. A semântica grava esse tipo no nó do operador
(`semantic.conversion_type_id`, que o dump da AST mostra como `converte=N`) só
quando algum operando precisa mudar de tipo. Os dois back-ends convertem
exatamente o que foi anotado e não decidem tipos por conta própria.

| Operação | Operandos | Conta feita em | Resultado |
|---|---|---|---|
| `+`, `-`, `*` | dois numéricos | tipo comum | tipo comum |
| `^` | dois numéricos | tipo comum | tipo comum (`inteira ^ inteira` é `inteira`) |
| `/` | dois numéricos | tipo comum; `duplocarpado` se os dois forem `inteira` | real |
| `//` | dois `inteira` | `inteira` (`sdiv i64`) | `inteira` |
| `//` | algum real | erro semântico E-SEM-017 | — |
| `raiz(x)` | `flut` ou `duplocarpado` | o próprio tipo | o próprio tipo |
| `raiz(x)` | `inteira` | `duplocarpado` | `duplocarpado` |
| `<`, `>`, `==`, `!` | dois numéricos | tipo comum | `bool` |

Exemplos: `f + 1` emite `sitofp i64 1 to float` e soma em `float`; `2.0 ^ 2`
chama `zl_potencia_flut(2.0, 2.0)` e dá o `flut` 4; `7 / 2` converte os dois
lados para `double` e dá 3.5; `7 // 2` dá 3; `flut m; m receba 7 / 2;` é
recusado com E-SEM-020, porque `duplocarpado` não estreita para `flut`.

Justificativas:
- `/` é sempre divisão real porque a linguagem já tem `//` para a divisão
  inteira, o mesmo par de Python 3. Entre dois `inteira`, o real escolhido é
  `duplocarpado`: ele representa exatamente todo inteiro até 2^53, enquanto
  `flut` erra a partir de 2^24 (em `02_conversoes`, 16777217 vira 16777216).
- `//` com real seria uma segunda divisão real, com a dúvida entre truncar e
  arredondar para baixo. Para reais o programa usa `/`.
- `inteira ^ inteira` continua `inteira` para ser exato: exponenciação por
  quadrados no runtime, módulo 2^64. `pow` em `double` perderia precisão acima
  de 2^53, e `llvm.powi` só serve para base real com expoente `i32`.
- `raiz` de `inteira` segue a regra do `/` e dá `duplocarpado`.

Testes: `tests/execucao/13_expressoes_mistas`, `14_potencia_raiz` e
`15_divisao_real_inteira`; recusas em
`tests/semantic/invalido_operadores_mistos.mac` (`7.0 // 2`) e
`invalido_divisao_real_para_inteira.mac` (`inteira` recebendo `7 / 2`).

## Operações e controle

Operandos, argumentos e o destino de atribuição são avaliados conforme D-020.
`E` e `OU` usam curto-circuito com blocos e saltos; `XOR` avalia ambos os lados
e usa `xor i1`. Condicionais e laços formam um CFG com um terminador por bloco.
Depois, `mem2reg` pode promover os locais elegíveis para SSA e inserir `phi`.

`/` converte os operandos para o tipo real da conta (tabela acima) e emite
`fdiv` após guarda para divisor zero, inclusive zero negativo. `//` emite `sdiv i64`,
truncando em direção a zero, após guardas contra zero e `INT64_MIN / -1`.
Soma, subtração, multiplicação, menos unário e potência inteira são modulares
em 64 bits; a IR não usa `nsw`/`nuw`. Potência inteira usa exponenciação por
quadrados no runtime e rejeita expoente negativo. `0 ^ 0` é 1.

Potências reais usam `powf`/`pow`; raízes usam `sqrtf`/`sqrt` via runtime.
Raiz negativa e domínio inválido de potência são erros. Outros resultados
reais seguem IEEE 754, incluindo arredondamento, infinito e NaN; não é usado
`fast-math`. Comparações reais `<`, `>`, `==` são ordenadas; `!` é verdadeira
também quando houver NaN. Numa comparação mista, os dois lados já chegam convertidos para o tipo comum.

## ABI do runtime

`runtime/zl_runtime.h` define assinaturas fixas. A correspondência é:

| Função Macaronica | Símbolo LLVM/C | Assinatura LLVM |
|---|---|---|
| `imprimeInteira` | `zl_imprime_int` | `void (i64)` |
| `imprimeFlut` | `zl_imprime_flut` | `void (float)` |
| `imprimeDuplocarpado` | `zl_imprime_duplocarpado` | `void (double)` |
| `imprimePalavra` | `zl_imprime_texto` | `void (ptr)` |
| `imprimeBool` | `zl_imprime_bool` | `void (i32)` |
| `imprimeNl` | `zl_imprime_nl` | `void ()` |
| `leInteira` | `zl_le_int` | `i64 (i32 linha)` |
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
declare i64 @zl_potencia_int(i64, i64, i32) nounwind
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
| 8 | uso inválido de pino ou de `espera` (só na rota da placa, `ZL_ERRO_PLACA`) |

O diagnóstico usa `linha N: erro de execucao C: mensagem` em `stderr` e status
de falha. O gerador faz as guardas de divisão e índice antes da operação.

Inteiros são impressos em decimal assinado, por conversão manual (sem
depender de `%lld`/`PRId64` na libc do alvo); `flut` com `%.9g`,
`duplocarpado` com `%.17g`, bool como `VER`/`FAL` e texto sem transformação.
Somente `imprimeNl` acrescenta nova linha. O processo começa com a locale C,
portanto usa ponto decimal. A leitura consome tokens separados por espaços;
reais de entrada admitem notação decimal com expoente opcional, sem hexadecimal,
NaN ou infinito. Tokens maiores que 255 bytes são rejeitados como entrada
inválida. Underflow real pode arredondar para subnormal ou zero; overflow é
rejeitado. Essas regras de `stdin` não ampliam os literais da linguagem.

## Entrada e retorno

`inteira principal()` e `vazio principal()` são normalizadas para
`define i32 @zl_principal() nounwind`. A primeira devolve o valor de `respost`
reduzido a 32 bits (`trunc i64 ... to i32`);
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
