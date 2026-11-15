# Catálogo de diagnósticos da Macaronica

Este documento cataloga os códigos emitidos pelo front-end na versão atual.
O código-fonte é a referência definitiva para as mensagens concretas: o lexer
está em [`src/lexer/lexer.c`](../src/lexer/lexer.c), o parser em
[`src/parser/parser.c`](../src/parser/parser.c) e a análise semântica em
[`src/semantic/semantic.c`](../src/semantic/semantic.c).

Este catálogo descreve apenas a versão atual e não promete que códigos ou
mensagens permaneçam iguais em versões futuras.

## Como ler um código

O formato é `E-FASE-NNN`:

- `E` indica erro;
- `LEX`, `SIN` ou `SEM` identifica a fase léxica, sintática ou semântica;
- `NNN` é um identificador atribuído manualmente à categoria do erro.

O número não é a linha da fonte, uma contagem de erros nem a ordem em que eles
aconteceram. Por exemplo, `E-SEM-007` sempre identifica o uso de um nome de
valor não declarado ou usado antes da declaração, independentemente da linha.
Um mesmo código pode ter mensagens variantes quando elas representam a mesma
regra. A linha e a coluna são informadas separadamente no diagnóstico.

## Erros léxicos

| Código | Significado | Quando ocorre | Exemplo curto |
|---|---|---|---|
| `E-LEX-001` | Caractere fora da linguagem | O lexer encontra um caractere não aceito. Inclui a variante específica de `=` isolado, pois igualdade usa `==`. | `a = 1;` |
| `E-LEX-002` | Literal de `palavra` não terminado | Uma cadeia iniciada por `"` chega a uma quebra de linha ou ao fim do arquivo sem aspas de fechamento. | `"texto` |
| `E-LEX-003` | Escape desconhecido | Um literal de `palavra` contém uma sequência de escape diferente de `\\`, `\"`, `\n`, `\r` ou `\t`. | `"a\q"` |
| `E-LEX-004` | Literal real incompleto | Há um ponto depois dos dígitos, mas nenhum dígito depois do ponto. | `12.` |
| `E-LEX-005` | UTF-8 inválido | A entrada contém uma sequência de bytes UTF-8 inválida, dentro ou fora de um literal de `palavra`. | Entrada com bytes inválidos |
| `E-LEX-006` | Literal numérico fora do intervalo | Um literal inteiro ultrapassa `long long` ou a parte inteira de um literal real ultrapassa o intervalo representável por `double`. | Número excessivamente grande |
| `E-LEX-007` | Negação lógica mal grafada | Aparece `NAO` (sem til) ou `não`, `Não`, `NãO` etc. como palavra inteira. A negação é somente `NÃO` (D-001). O lexer relata o erro e segue como se fosse `NÃO`, para não gerar erros sintáticos em cascata. `nao` e `Nao` continuam nomes válidos. | `b receba NAO VER;` |

`E-LEX-002`, `E-LEX-005` e `E-LEX-006` possuem mais de uma mensagem concreta,
mas cada grupo representa uma única categoria de erro.

## Erros sintáticos

| Código | Significado | Quando ocorre | Exemplo curto |
|---|---|---|---|
| `E-SIN-001` | Construção sintática esperada | Falta uma expressão, um tipo, um comando, uma declaração de campo ou uma declaração global válida. A mensagem especifica o que era esperado. Uma variante aponta comando fora de função logo depois de um `]` a mais. | `x receba ;` |
| `E-SIN-002` | Token obrigatório ausente | `expect` não encontra o delimitador ou palavra exigida pela produção; também cobre `[` ou `]` ausente em bloco ou registro, tamanho de vetor que não é um único literal inteiro (`vet{n}`, `vet{2 + 1}`), `vet` depois de `,` e função declarada sem `outrafuncao`. | `respost 1` sem `;` |
| `E-SIN-003` | Falta de memória durante o parsing | O parser não consegue alocar um nó, texto ou item de lista. É um diagnóstico interno, não um erro de escrita do programa. | — |
| `E-SIN-004` | Palavra `func` sem produção | `func` aparece, embora a gramática aceite `outrafuncao` ou `principal`. | `func inteira soma()` |
| `E-SIN-005` | Comparadores encadeados | Mais de um comparador aparece na mesma comparação sem composição explícita. | `a < b < c` |
| `E-SIN-007` | `casocontrario` fora de posição | `casocontrario` aparece isolado ou depois do ramo final de uma cadeia condicional. | `casocontrario edai [ ]` sem `cond` anterior |
| `E-SIN-008` | Uso sintático inválido de `vazio` | `vazio` é usado onde não é o tipo de retorno de uma função. | `vazio x;` |
| `E-SIN-009` | Profundidade máxima excedida | Uma expressão ou sequência de blocos excede o limite defensivo de 512 níveis. É uma proteção interna contra recursão excessiva. | 513 blocos aninhados |

`E-SIN-006` está reservado e não é emitido pela implementação atual. Nenhum
significado deve ser atribuído a ele até que uma decisão do projeto o defina.
`E-SIN-001`, `E-SIN-002` e `E-SIN-009` têm mensagens variantes relacionadas.

## Erros semânticos

| Código | Significado | Quando ocorre | Exemplo curto |
|---|---|---|---|
| `E-SEM-000` | Falta de memória na análise semântica | Não é possível alocar tipos, símbolos, escopos ou estruturas auxiliares. É um diagnóstico interno. | — |
| `E-SEM-001` | Tipo não declarado | Uma referência de tipo não corresponde a um tipo nativo nem a um `registro` declarado. | `Pessoa p;` sem `registro Pessoa` |
| `E-SEM-002` | `vazio` em contexto de valor | O tipo `vazio` aparece em variável, vetor, parâmetro, campo ou outro contexto que exige valor. | `vazio x;` se esse nó chegar à semântica |
| `E-SEM-003` | Redefinição no mesmo escopo | Um nome de valor, função, campo ou tipo é declarado novamente no mesmo espaço de nomes e escopo. Possui variantes de mensagem para valores e tipos. | `inteira x, x;` |
| `E-SEM-004` | Falta `principal` | O programa não declara a função `principal`. Uma segunda `principal` não gera este código: ela já é relatada como `E-SEM-003` (redefinição), no ponto exato. | Programa sem `principal` |
| `E-SEM-005` | Tamanho inválido de vetor | O tamanho declarado é menor ou igual a zero. | `inteira vet{0} itens;` |
| `E-SEM-006` | Função usada sem chamada | O nome de uma função é usado como se fosse um valor. | `x receba soma;` |
| `E-SEM-007` | Identificador de valor não declarado | Uma variável ou outro valor é usado antes da declaração ou nunca foi declarado. Também se aplica à base não declarada de um acesso de vetor. | `x receba y;` sem declarar `y` |
| `E-SEM-008` | Valor chamado como função | O nome antes dos parênteses resolve para variável ou outro valor, não para função. | `x();` quando `x` é variável |
| `E-SEM-009` | Função não declarada | Uma chamada referencia um nome que não corresponde a função nem a valor visível. | `soma();` sem declarar `soma` |
| `E-SEM-010` | Chamada direta de `principal` | `principal` é chamada como função comum. | `principal();` |
| `E-SEM-011` | Tipo de argumento incompatível | O tipo de um argumento não é atribuível ao tipo do parâmetro correspondente. | Passar `VER` a parâmetro `inteira` |
| `E-SEM-012` | Quantidade de argumentos incompatível | A chamada fornece quantidade diferente da assinatura da função. | Chamar `soma(a, b)` como `soma(1)` |
| `E-SEM-013` | Retorno `vazio` usado em expressão | Uma chamada de função `vazio` é usada onde deveria produzir um valor. | `x receba imprime();` |
| `E-SEM-014` | Índice de vetor não inteiro | A expressão entre `{` e `}` não tem tipo `inteira`. | `itens{1.5}` |
| `E-SEM-015` | Indexação de algo que não é vetor | A base do acesso indexado possui outro tipo. | `x{0}` quando `x` é `inteira` |
| `E-SEM-016` | Índice constante fora dos limites | Um índice inteiro constante é negativo ou maior ou igual ao tamanho do vetor. | `itens{5}` em `vet{5}` |
| `E-SEM-017` | Operador incompatível com os operandos | Um operador unário ou binário recebe tipos que não aceita. A mensagem distingue um operando de dois operandos. | `VER + FAL` |
| `E-SEM-018` | Nó inválido como expressão | A AST contém, em posição de expressão, um tipo de nó que não representa expressão. É uma defesa interna da AST. | — |
| `E-SEM-019` | Alvo não gravável | O lado esquerdo de `receba` não está marcado como `lvalue`. | AST com literal como destino |
| `E-SEM-020` | Tipo incompatível na atribuição | O valor não é atribuível ao tipo do destino; aceitam-se tipos identicos ou as promocoes numericas de D-012. | `inteira x; x receba VER;` |
| `E-SEM-021` | Condição não booleana | A condição de `cond`, `durante` ou `repete` não resulta em `bool`. | `cond (1) edai [ ]` |
| `E-SEM-022` | `respost` fora de função | Um nó de retorno aparece sem função ativa. É principalmente uma defesa da AST, pois o parser normal só cria comandos dentro de funções. | — |
| `E-SEM-023` | Função `vazio` retorna valor | `respost` contém expressão dentro de função com retorno `vazio`. | `respost 1;` em função `vazio` |
| `E-SEM-024` | Função não vazia retorna sem valor | `respost;` aparece em função cujo retorno não é `vazio`. | `respost;` em função `inteira` |
| `E-SEM-025` | Tipo de retorno incompatível | O valor de `respost` não é atribuível ao retorno declarado. | `respost VER;` em função `inteira` |
| `E-SEM-026` | Vetor completo usado como valor | O nome de um vetor é usado sem indexação. Somente um elemento pode ser lido como valor. Uma variante cobre o vetor completo como alvo de atribuição: `destino receba origem` gera um erro só, no alvo, mesmo que `origem` também seja vetor. | `x receba itens;` |
| `E-SEM-027` | Chamada não vazia usada como comando | Uma chamada isolada em um comando retorna valor, quando somente chamada `vazio` pode ficar isolada. | `soma();` se `soma` retorna `inteira` |
| `E-SEM-028` | Comando de expressão inválido | Um nó de comando de expressão não contém uma chamada de função. É uma defesa interna da AST. | — |
| `E-SEM-029` | Nó inválido como comando | A AST contém em um bloco um tipo de nó que não representa comando. É uma defesa interna da AST. | — |
| `E-SEM-030` | Declaração de campo inválida | Um `registro` contém nó que não é declaração simples de campo. É uma defesa interna da AST. | — |
| `E-SEM-031` | Parâmetro inválido | A lista de parâmetros contém nó que não é declaração simples. É uma defesa interna da AST. | — |
| `E-SEM-032` | Retorno não garantido | Função não vazia não garante `respost` com valor em todos os caminhos de execução. Laços, por si só, não garantem retorno. | Função `inteira` sem `respost` |
| `E-SEM-033` | Declaração global inválida | A raiz da AST contém nó que não é registro, função, variável ou vetor global. É uma defesa interna da AST. | — |

Os códigos `E-SEM-018`, `E-SEM-019`, `E-SEM-022`, `E-SEM-028`, `E-SEM-029`,
`E-SEM-030`, `E-SEM-031` e `E-SEM-033` protegem a análise contra uma AST
inconsistente ou contra um destino que o parser normal não produz. Em um fluxo
normal, várias dessas formas são impedidas pelo parser antes de a semântica
começar. `E-SEM-000` sinaliza falha de alocação.

Os códigos `E-SEM-003`, `E-SEM-007` e `E-SEM-017` têm variantes de mensagem
para contextos relacionados. Isso não cria novos códigos: a categoria continua
sendo identificada pelo mesmo número.

## Um diagnóstico por erro

Cada erro real gera um diagnóstico, sem cascata (D-018):

- o lexer relata todos os erros léxicos; havendo algum, o parser não roda;
- o parser usa modo pânico com sincronização. O primeiro erro de um comando é
  relatado, os seguintes do mesmo comando são consequência dele e ficam em
  silêncio, e o parser retoma no próximo ponto seguro;
- a semântica só roda sobre uma AST sem erros sintáticos, e um erro de
  subexpressão vira o tipo `erro`, que não gera novos diagnósticos acima dele.

Dois erros reais em comandos distintos geram dois diagnósticos. O
`tests/manifest.tsv` registra, na coluna `erros`, quantos diagnósticos cada
caso deve produzir, e `tests/run_tests.sh` confere esse número exato.

## Manutenção do catálogo

Ao criar ou alterar um diagnóstico:

1. atribua o código explicitamente no ponto que emite o erro;
2. reutilize um código apenas para variantes da mesma regra;
3. não renumere códigos com base na posição no arquivo;
4. atualize este catálogo na mesma alteração;
5. confira a regra correspondente na especificação vigente do projeto.
