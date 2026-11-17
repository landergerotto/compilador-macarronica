# DSL vetorial da Macarônica (PjBL 4, Parte A)

Este documento é o contrato da DSL vetorial: a sintaxe, as regras de tipo, os
erros `E-DSL-nnn`, as respostas às lacunas da Seção 5.5 do enunciado e os
nomes e assinaturas dos núcleos MLIR que a frente de benchmark usa.

Código: `src/semantic/dsl.c` (semântica e nomes dos núcleos),
`src/codegen_mlir.c` (núcleos em MLIR, declarações e chamadas LLVM, laços C),
`scripts/dsl/teste.sh` (`make dsl-teste`: recusas, descida MLIR → LLVM e
execução ligada) e `tests/dsl/16_dsl_vetorial.mac`, com as recusas em
`tests/semantic/invalido_dsl_*.mac`.

## 1. Operações

| Forma | Significado (para todo `i` em `0 .. N-1`) |
|---|---|
| `c receba a + b;` | `c{i} = a{i} + b{i}` |
| `c receba a - b;` | `c{i} = a{i} - b{i}` |
| `c receba a * b;` | `c{i} = a{i} * b{i}` |
| `c receba k * a;` e `c receba a * k;` | `c{i} = k * a{i}` e `c{i} = a{i} * k` |
| `c receba k + a;`, `c receba a - k;`… | escalar com `+`, `-` e `*`, dos dois lados |
| `somavet(a)` | `0 + a{0} + a{1} + … + a{N-1}`, somado da esquerda para a direita |
| `maxvet(a)` | maior elemento de `a` |
| `prodesc(a, b)` | `0 + a{0}*b{0} + a{1}*b{1} + …`, da esquerda para a direita |

`a`, `b` e `c` são vetores declarados com `vet{N}` e elemento `inteira`,
`flut` ou `duplocarpado`. `k` é qualquer expressão escalar numérica, inclusive
uma redução: `c receba somavet(a) * b;` vale.

Como extra, além do produto por escalar pedido no enunciado, a DSL aceita o
escalar com `+` e `-` e dos dois lados do operador.

## 2. Sintaxe

Não há token novo. As formas da DSL já são frases válidas da gramática do
PjBL 1: `c receba a + b;` é uma `atribuicao` comum e `somavet(a)` é uma
`chamada`. A semântica distingue a DSL pelo tipo dos nomes: quando um operando
de `+`, `-` ou `*` é o nome de um vetor inteiro, a atribuição é vetorial.
Assim o parser e o léxico não mudam, e nenhum programa do PjBL 1 muda de
significado.

As produções abaixo descrevem as frases que a semântica aceita como DSL:

```ebnf
(* Casos particulares de atribuicao e chamada, reconhecidos pelo tipo. *)
atribuicao_vetorial = nome_vetor, "receba", operacao_vetorial, ";" ;
operacao_vetorial   = nome_vetor, op_vetorial, nome_vetor
                    | escalar, op_vetorial, nome_vetor
                    | nome_vetor, op_vetorial, escalar ;
op_vetorial         = "+" | "-" | "*" ;
escalar             = expressao ;   (* numerica, sem nome de vetor solto *)
reducao             = ( "somavet" | "maxvet" ), "(", nome_vetor, ")"
                    | "prodesc", "(", nome_vetor, ",", nome_vetor, ")" ;
nome_vetor          = IDENT ;       (* declarado com vet{N}, elemento numerico *)
```

- `somavet`, `maxvet` e `prodesc` são intrínsecas pré-declaradas no espaço de
  nomes de funções, como `imprimeInteira`. Redefinir uma delas é `E-SEM-003`.
  Uma variável pode ter esses nomes, porque valores e funções têm espaços de
  nomes separados.
- Cada comando faz uma operação. `c receba a + b + d;` é `(a + b) + d` e dá
  `E-DSL-005`, porque o operando esquerdo é outra expressão vetorial. A forma
  correta é usar dois comandos: `c receba a + b; c receba c + d;`.
- Copiar ou preencher um vetor inteiro (`c receba a;`, `c receba 0;`)
  continua fora da linguagem e dá o mesmo `E-SEM-026` de antes. Também não há
  `/` entre vetores.

## 3. Regras de tipo

Sejam `A` e `B` os tipos dos elementos (ou do escalar `K`) e `C` o tipo do
elemento do destino.

1. Todos os vetores da operação precisam ter elemento numérico (`inteira`,
   `flut` ou `duplocarpado`). Senão é `E-DSL-002`. O escalar também precisa
   ser numérico, ou é `E-DSL-006`.
2. A conta é feita no tipo `R = promove(A, B)`, aplicado elemento a elemento.
   É a mesma promoção das expressões mistas do PjBL 3: vence o tipo mais
   largo na ordem `inteira < flut < duplocarpado`. A semântica da DSL chama a
   função de promoção da frente da linguagem, então a regra não se duplica.
3. O resultado `R` precisa caber no elemento do destino pela regra de
   atribuição (D-012): identidade, `inteira→flut`, `inteira→duplocarpado` ou
   `flut→duplocarpado`. Senão é `E-DSL-004`. Por isso
   `inteira vet{4} c; c receba f + g;` com `f` e `g` `flut` é erro, e
   `duplocarpado vet{4} d; d receba i + f;` com `i` `inteira` e `f` `flut`
   vale: a conta é em `flut` e o resultado é estendido para `duplocarpado`.
4. Todos os vetores da operação têm o mesmo tamanho `N`, ou é `E-DSL-003`.
5. `somavet(a)` e `maxvet(a)` têm o tipo do elemento de `a`. `prodesc(a, b)`
   tem o tipo `promove(A, B)`. O resultado é um valor escalar comum e segue as
   regras normais: `inteira s receba somavet(f);` com `f` `flut` é `E-SEM-020`.
6. Uma redução usada sozinha como comando (`somavet(a);`) é `E-SEM-027`, como
   qualquer chamada que retorna valor.

A aritmética é a da linguagem (D-021). `inteira` é módulo 2^64: o estouro dá
a volta nos três caminhos (LLVM, MLIR e C). `flut` e `duplocarpado` são
IEEE 754 sem fast-math, sem reassociação e sem fusão em FMA. Uma comparação
com NaN é falsa.

## 4. Erros

| Código | Quando | Exemplo |
|---|---|---|
| `E-DSL-001` | O destino de uma operação vetorial não é um vetor | `x receba a + b;` com `x` `inteira` |
| `E-DSL-002` | Vetor da operação com elemento não numérico | `p receba q + r;` com vetores `logica` |
| `E-DSL-003` | Vetores de tamanhos diferentes | `vet{4}` mais `vet{5}` |
| `E-DSL-004` | O tipo do resultado não cabe no elemento do destino | `inteira` recebe soma de `flut` |
| `E-DSL-005` | Forma vetorial não suportada | `c receba a + b + d;` ou `c receba 1 + 2;` |
| `E-DSL-006` | Escalar não numérico | `c receba verdadeiro * a;` |
| `E-DSL-007` | Argumento de redução não é o nome de um vetor | `somavet(x)` ou `somavet(a + b)` |
| `E-DSL-008` | Quantidade de argumentos errada na redução | `prodesc(a)` |

Os erros saem na fase semântica, no formato de sempre
(`arquivo.mac:linha:coluna: semantico E-DSL-003: ...`), e o compilador
devolve o código 1, como nos demais erros de fonte.

## 5. Respostas às lacunas da Seção 5.5

**Tamanho só conhecido em execução.** Hoje isso não acontece. O tamanho de
um vetor é sempre um literal (`vet{N}`), e um vetor não pode ser parâmetro,
retorno nem valor. Então a checagem de tamanho é sempre estática
(`E-DSL-003`). Mesmo assim, os núcleos recebem `memref<?xT>` e leem o tamanho
do descritor, ou seja, não dependem de `N` fixo. A frente de benchmark usa o
mesmo núcleo para qualquer `N`. Se a linguagem um dia tiver vetor de tamanho
dinâmico, a checagem vira um teste antes da chamada, com um novo código de
erro de execução no runtime (`zl_erro_execucao`).

**Destino igual a um operando.** É permitido: `a receba a * a;` e
`a receba 2 * a;`. A semântica é de valor, isto é, o resultado é o mesmo que
calcular o lado direito inteiro antes de gravar. A implementação não precisa
de vetor temporário por dois motivos:

- O elemento `i` do resultado só depende do elemento `i` de cada operando, que
  é lido antes de `c{i}` ser gravado. Não há dependência entre iterações.
- Na Macarônica um vetor é um objeto inteiro. Dois nomes são o mesmo vetor ou
  vetores disjuntos, porque não há fatia nem ponteiro. Logo não existe
  sobreposição parcial.

O escalar é avaliado uma única vez, antes do laço. Em `a receba a{0} * a;`,
todos os elementos são multiplicados pelo `a{0}` original. A vetorização
continua legal, porque cada bloco de 8 elementos é lido antes de ser gravado
no mesmo lugar.

**Redução de vetor vazio ou unitário.** Vetor vazio não existe: `vet{0}` é
`E-SEM-005`. Com um elemento, `somavet(a)` vale `0 + a{0}`, `maxvet(a)` vale
`a{0}` e `prodesc(a, b)` vale `0 + a{0}*b{0}`.

- `somavet` começa em zero para coincidir com o laço escrito à mão
  (`s receba 0.0; ... s receba s + a{i};`). Para reais, a única diferença em
  relação a "começar em `a{0}`" é o `-0.0`, porque `0.0 + -0.0` dá `+0.0`.
- `maxvet` começa em `a{0}` e compara com `>` a partir de `a{1}`. Um NaN no
  meio é ignorado, porque a comparação é falsa. Se `a{0}` é NaN, o resultado é
  NaN. É exatamente o comportamento do laço escalar com
  `se (a{i} > m) entao m receba a{i};`.

**Alvo sem SIMD.** O significado não depende de SIMD.

- No PC, o MLIR gera `vector<8xT>`, e o LLVM legaliza para o que a CPU tem.
  Com SSE2, um `vector<8xf32>` vira dois `<4 x float>`. Sem SIMD, o LLVM
  escalariza.
- No ESP32 (Xtensa LX6, sem SIMD para esses tipos), o back-end C gera os
  mesmos núcleos como laços escalares simples, com os mesmos nomes.

O resultado é bit a bit igual em todos os alvos por dois motivos:

- As operações elemento a elemento não reassociam nada.
- As reduções de reais são sequenciais em todos os caminhos. Só as reduções
  inteiras podem ser reordenadas, porque a soma módulo 2^64 e o máximo são
  associativos.

## 6. Contrato dos núcleos (frente de benchmark)

A estratégia é híbrida. O programa continua em LLVM IR, gerado pela AST
anotada como no PjBL 3. Só as operações vetoriais viram funções MLIR
(`func`, `arith`, `memref`, `affine` e, depois da vetorização, `vector` e
`scf`). Essas funções são descidas para LLVM IR e ligadas ao `.ll` do
programa com `llvm-link`. A justificativa está no relatório
(`docs/relatorio_otimizacao.md`, Parte B).

Os núcleos são gerados sob demanda: cada combinação de operação e tipos que o
programa usa vira uma função, uma vez só.

### 6.1 Nomes

Marcas de tipo: `i64` (`inteira`), `f32` (`flut`), `f64` (`duplocarpado`).
Marcas de operação: `soma` (`+`), `sub` (`-`), `mul` (`*`). `R` é
`promove(...)`.

| Forma | Núcleo | Assinatura MLIR |
|---|---|---|
| `c receba a op b` | `mcn_vv_<op>_<A>_<B>_<C>` | `(memref<?xA>, memref<?xB>, memref<?xC>) -> ()` |
| `c receba k op a` | `mcn_ev_<op>_<R>_<A>_<C>` | `(R, memref<?xA>, memref<?xC>) -> ()` |
| `c receba a op k` | `mcn_ve_<op>_<A>_<R>_<C>` | `(memref<?xA>, R, memref<?xC>) -> ()` |
| `somavet(a)` | `mcn_somavet_<A>` | `(memref<?xA>) -> A` |
| `maxvet(a)` | `mcn_maxvet_<A>` | `(memref<?xA>) -> A` |
| `prodesc(a, b)` | `mcn_prodesc_<A>_<B>` | `(memref<?xA>, memref<?xB>) -> R` |

Exemplos: `mcn_vv_soma_i64_i64_i64`, `mcn_ev_mul_f32_i64_f32` (`flut` vezes
vetor `inteira`, resultado `flut`), `mcn_prodesc_f64_f64`.

- O escalar chega já convertido para `R`.
- `ev` e `ve` são núcleos separados porque `-` não é comutativo e porque a
  ordem dos operandos é preservada (D-020).
- Os nomes têm `_`, que um identificador da Macarônica não pode ter, então
  nunca colidem com funções ou globais do programa.

### 6.2 Convenção de chamada depois da descida

Os núcleos não usam `llvm.emit_c_interface` nem `bare-ptr`, porque `bare-ptr`
exige forma estática. Depois de `--finalize-memref-to-llvm` e
`--convert-func-to-llvm`, cada `memref<?xT>` de uma dimensão vira cinco
parâmetros:

```
ptr alocado, ptr alinhado, i64 deslocamento, i64 tamanho, i64 passo
```

O `.ll` do programa declara e chama os núcleos assim:

```llvm
declare void @mcn_vv_soma_i64_i64_i64(ptr, ptr, i64, i64, i64, ptr, ptr, i64, i64, i64, ptr, ptr, i64, i64, i64)
declare i64 @mcn_somavet_i64(ptr, ptr, i64, i64, i64)

  call void @mcn_vv_soma_i64_i64_i64(ptr @a, ptr @a, i64 0, i64 8, i64 1, ptr @b, ptr @b, i64 0, i64 8, i64 1, ptr @c, ptr @c, i64 0, i64 8, i64 1)
  %t.3 = call i64 @mcn_somavet_i64(ptr @a, ptr @a, i64 0, i64 8, i64 1)
```

O vetor é o próprio global (`@a`) ou o `alloca` do local (`%a.addr`).
Deslocamento é 0 e passo é 1. Um escalar entra na posição do seu parâmetro,
como `float` ou `double` ou `i64` comum.

No back-end C (placa), os mesmos nomes viram funções `static` no `.c`
gerado, com ponteiro e tamanho:

```c
static void mcn_vv_soma_i64_i64_i64(const int64_t *a, const int64_t *b, int64_t *c, int64_t n);
static void mcn_ev_mul_f32_i64_f32(float k, const int64_t *a, float *c, int64_t n);
static int64_t mcn_somavet_i64(const int64_t *a, int64_t n);
```

### 6.3 Como cada núcleo é escrito

- Os núcleos elemento a elemento usam `affine.for` de `0` até
  `memref.dim`, com `affine.load` e `affine.store`. É o formato que o
  `--affine-super-vectorize` entende.
- As reduções inteiras também usam `affine.for` com `iter_args`.
- As reduções de reais (`somavet`, `prodesc` com resultado real e `maxvet`
  real) usam `scf.for` com `memref.load`. Assim o vetorizador afim não as
  toca, e a ordem de arredondamento fica a sequencial do laço escalar.
- `maxvet` inteiro usa `arith.maxsi`. `maxvet` real usa `arith.cmpf ogt` com
  `arith.select`, que é o `se (x > m)` do laço escalar.

Para ver o MLIR de um programa, compile-o com `-o prog.ll`. Se ele usa a DSL,
o compilador grava `prog.mlir` ao lado. O `.ll` chama os núcleos `mcn_*`, que
só existem no `.mlir`. Para rodar, desça o `.mlir` e ligue os dois, como faz o
`make dsl-teste` (`scripts/dsl/teste.sh`):

```bash
B=/usr/lib/llvm-18/bin
build/release/macaronica prog.mac -o prog.ll   # grava prog.ll e prog.mlir
$B/mlir-opt prog.mlir --lower-affine --convert-scf-to-cf \
    --convert-arith-to-llvm --finalize-memref-to-llvm --convert-func-to-llvm \
    --convert-cf-to-llvm --reconcile-unrealized-casts -o prog_llvm.mlir
$B/mlir-translate --mlir-to-llvmir prog_llvm.mlir -o prog_nucleos.ll
$B/clang -Wno-override-module prog.ll prog_nucleos.ll \
    runtime/zl_runtime_posix.c -lm -o prog
./prog
```

A rota C99 (`--emit-c` e a placa, com `--alvo=esp32`) não usa o MLIR: nela a
DSL sai como laços inline no C.

## 7. Informação que a semântica deixa para os back-ends

A semântica marca os nós da DSL com o bit `DSL_SEMANTIC_FLAG` (`0x8000`,
longe dos `AST_SEMANTIC_*`) em `node->semantic.flags`:

- a atribuição vetorial, com `type_id` igual ao tipo do vetor destino;
- a chamada de redução, com `type_id` igual a `R`.

Os back-ends não refazem a análise. Eles chamam `dsl_describe()`
(`src/semantic/dsl.h`), que devolve:

- a forma;
- o operador;
- `N`;
- os símbolos dos vetores;
- o nó do escalar;
- os tipos `A`, `B`, `R` e `C`;
- o nome do núcleo.
