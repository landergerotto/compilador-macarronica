# Extensão vetorial da Macaronica para o PjBL 4

Esta especificação define a Parte A: operações numéricas sobre vetores de
tamanho estático, usando a AST e a tabela de tipos existentes. Os núcleos são
gerados em MLIR pelo backend híbrido, sem expandir essas operações diretamente
em laços escalares no gerador LLVM. A construção, os passes e os experimentos
da Parte B/C são descritos em `relatorio_otimizacao.md`.

## Sintaxe e tokens

Não há novos tokens nem palavras reservadas. Reutilizam-se os tokens de
identificador, `receba`, `+`, `-`, `*`, `(`, `)`, `,` e `;`. `somavet`,
`maxvet` e `prodesc` são funções intrínsecas predeclaradas no namespace global
de funções. Sua redefinição é rejeitada como a das intrínsecas de E/S. A
sobrecarga de operadores é contextual e decidida pela análise semântica;
as regras escalares anteriores permanecem iguais.

Produções contextuais, após a gramática sintática comum:

```ebnf
atribuicao_vetorial = identificador_vetor, "receba", operacao_vetorial, ";" ;
operacao_vetorial = identificador_vetor, operador_elemento, identificador_vetor
                  | expressao_escalar, "*", identificador_vetor
                  | identificador_vetor, "*", expressao_escalar ;
operador_elemento = "+" | "-" | "*" ;
reducao = "somavet", "(", identificador_vetor, ")"
        | "maxvet", "(", identificador_vetor, ")"
        | "prodesc", "(", identificador_vetor, ",", identificador_vetor, ")" ;
```

`identificador_vetor` é um identificador já declarado como vetor numérico.
`expressao_escalar` é uma expressão cujo resultado é `inteira`, `flut` ou
`duplocarpado`. Parênteses de agrupamento preservam a AST usual. Não são
permitidos operadores vetoriais aninhados, como `c receba (a + b) * a;`:
separe-os em atribuições com um vetor intermediário. As reduções só recebem
identificadores de vetor, não resultados vetoriais temporários.

Exemplos:

```text
c receba a + b;
c receba a - b;
c receba a * b;
c receba 3 * a;
c receba a * 1.5;
soma receba somavet(a);
maior receba maxvet(a);
produto receba prodesc(a, b);
```

O parser existente já reconhece essas formas como atribuições, operações
binárias e chamadas. As restrições contextuais são verificadas na semântica,
com diagnóstico localizado; não se acrescenta uma segunda gramática ao lexer.

## Operações e resultados

| Operação | Significado por índice ou redução |
|---|---|
| `c receba a + b;` | `c{i}` recebe a soma dos elementos `a{i}` e `b{i}` |
| `c receba a - b;` | `c{i}` recebe a diferença dos elementos |
| `c receba a * b;` | `c{i}` recebe o produto dos elementos |
| `c receba k * a;` | cada elemento recebe o produto pelo escalar `k` |
| `somavet(a)` | soma dos elementos em ordem crescente de índice |
| `maxvet(a)` | maior elemento, iniciando pelo elemento de índice zero |
| `prodesc(a, b)` | soma de `a{i} * b{i}` em ordem crescente de índice |

As reduções são expressões escalares: seu valor precisa ser usado em
atribuição, retorno, argumento ou outra expressão. Uma chamada de redução
isolada como comando é rejeitada, preservando a regra anterior de que chamadas
usadas como comando devem retornar `vazio`.

## Tipos e promoções

Somente elementos `inteira`, `flut` e `duplocarpado` são aceitos. Vetores de
`bool`, `palavra` ou registros continuam disponíveis conforme a linguagem
base, mas não participam destas operações numéricas da DSL.

O tipo comum de operação é calculado pela ordem:

```text
inteira < flut < duplocarpado
```

| Operandos numéricos | Tipo comum |
|---|---|
| `inteira`, `inteira` | `inteira` |
| `inteira`, `flut` | `flut` |
| `inteira`, `duplocarpado` | `duplocarpado` |
| `flut`, `flut` | `flut` |
| `flut`, `duplocarpado` | `duplocarpado` |
| `duplocarpado`, `duplocarpado` | `duplocarpado` |

A ordem dos operandos não altera a escolha. No produto escalar, os elementos
são convertidos ao tipo comum antes da multiplicação e acumulação. Na soma
e no máximo de um vetor, o resultado tem o tipo de seu elemento.

O destino vetorial pode ter o mesmo tipo de elemento do resultado ou receber
uma promoção permitida: `inteira -> flut`, `inteira -> duplocarpado` ou
`flut -> duplocarpado`. Não há narrowing implícito. A operação ocorre no
tipo comum e só depois o resultado por elemento é convertido ao tipo do
destino; atribuir um resultado `flut` a um destino `duplocarpado` não faz a
operação original ganhar retroativamente precisão binary64.

Essa promoção entre elementos é uma regra nova da DSL. Ela não modifica a
proibição anterior de operações **escalares** binárias com tipos diferentes.
O helper `type_common_numeric` centraliza a decisão; o backend emite as
conversões explícitas no MLIR. Atribuições escalares do resultado de uma
redução seguem D-012 normalmente.

## Tamanhos, destinos e aliasing

Todos os vetores envolvidos em uma operação devem ter o mesmo comprimento,
inclusive o destino. Os tamanhos são literais inteiros entre 1 e 2147483647,
como já estabelecido para a linguagem base. Tamanhos diferentes são erro
semântico; não há truncamento, broadcasting ou redimensionamento implícito.
Vetores com comprimento conhecido só em execução estão fora desta versão.

O destino pode ser um dos operandos, inclusive ambos:

```text
a receba a + b;
a receba a * a;
```

Isso é seguro porque cada saída depende somente das entradas de **mesmo
índice**, lidas antes da escrita naquele elemento. A linguagem não oferece
fatias, ponteiros de fonte nem vistas parcialmente sobrepostas de vetores.
Não se aplicam atributos `noalias` aos argumentos dos núcleos. Cada iteração
de uma operação elemento a elemento pode ser distribuída entre threads sem
criar dependência entre índices, respeitadas essas restrições.

Uma expressão escalar usada em `k * a` é calculada uma única vez, antes de
executar o núcleo vetorial. Chamadas presentes nessa expressão não são
repetidas para cada elemento. A ordem geral de avaliação de D-020 é
preservada.

A cópia pura `destino receba origem;` continua rejeitada, assim como passagem
de um vetor inteiro para uma função comum. A autorização de ler um vetor
inteiro é restrita aos contextos vetoriais desta especificação.

## Reduções e regras numéricas

Vetores vazios são rejeitados na declaração, portanto não há resultado de
redução sobre vetor vazio. Para tamanho um, soma e máximo têm o valor
numérico do único elemento; produto escalar multiplica o par de elementos.

`somavet` e `prodesc` acumulam a partir de zero, em ordem crescente de índice,
na versão estrita. `inteira` usa aritmética modular de 32 bits, inclusive nos
produtos intermediários; não se promete ausência de overflow assinado para
essas operações LLVM. Os tipos reais preservam binary32/binary64 e não
recebem autorização implícita de reassociação ou `fast-math`.

Em IEEE 754, somar a partir de `+0` pode alterar o sinal de um zero negativo;
a regra de tamanho um acima descreve igualdade numérica, não identidade de
bits. O máximo começa no primeiro elemento e atualiza apenas quando o
elemento seguinte é estritamente maior: NaN no primeiro elemento permanece;
NaN posterior não substitui o acumulador; empates preservam o elemento
anterior, inclusive o sinal do zero. Inteiros usam comparação assinada.

Reordenar uma soma real pode alterar o resultado por arredondamento. Por
isso, qualquer experimento com reassociação/`fast-math` deve ser identificado
como uma configuração experimental separada, com comparação numérica e
limites explícitos; não pode alterar silenciosamente a semântica padrão.

## Contrato da AST e integração

Os nós são os mesmos da linguagem base, anotados com `AST_SEMANTIC_DSL`:

- operação binária vetorial: tipo `TYPE_ARRAY` com comprimento e elemento
  comum resolvidos, flags `DSL | ARRAY | RVALUE`;
- atribuição vetorial: destino resolvido e flags `DSL | ARRAY`;
- chamada de redução: tipo escalar resultante, símbolo da intrínseca e
  flags `DSL | RVALUE`;
- identificador vetorial em contexto permitido: tipo e símbolo resolvidos,
  flags `ARRAY | LVALUE | RVALUE`.

As intrínsecas possuem as identidades `BUILTIN_VECTOR_SUM`,
`BUILTIN_VECTOR_MAX` e `BUILTIN_VECTOR_DOT`. Suas assinaturas são contextuais:
o símbolo usa marcadores internos, e cada chamada é anotada com o tipo
concreto. Esses marcadores não são tipos atribuíveis nem curingas para
chamadas comuns. O backend usa a identidade do símbolo, não comparação de
nomes escritos pelo usuário.

## Diagnósticos e testes

| Código | Condição |
|---|---|
| `E-DSL-001` | quantidade errada de argumentos de redução |
| `E-DSL-002` | argumento de redução não é identificador de vetor numérico |
| `E-DSL-003` | comprimentos diferentes entre vetores de entrada |
| `E-DSL-004` | resultado de redução descartado como comando |
| `E-DSL-005` | operador ou tipos inválidos para operação vetorial |
| `E-DSL-006` | operando vetorial aninhado não suportado |
| `E-DSL-007` | atribuição ao vetor sem operação vetorial |
| `E-DSL-008` | tamanho do destino incompatível ou narrowing |

`tests/dsl/01_elementwise.mac` compara soma, subtração, multiplicação e
aliasing com expressões escalares por elemento. `02_escalar_promocao.mac`
compara as duas ordens de escalar × vetor e promoção inteira/flut com uma
referência escalar. `03_reducoes.mac` compara soma e máximo com laços, usando
valores negativos e vetor de tamanho um. `04_produto_escalar.mac` compara o
produto escalar misto com um laço e exercita promoção do resultado para
`duplocarpado`. Os arquivos `.esperado` definem a saída; os programas retornam
status não zero quando a referência escalar diverge.

`tests/dsl/invalidos/` contém tamanhos incompatíveis, tipo não numérico,
narrowing, produto escalar com tamanhos diferentes, composição vetorial
aninhada e redução de escalar. Os testes anteriores da linguagem base devem
continuar passando. Nenhum resultado de execução é afirmado apenas pela
existência dos arquivos de teste; a evidência está nas execuções do runner.
