# Guia simples para defender a autoria do compilador Macaronica

Este guia organiza os pontos que aparecem com mais frequência em uma
apresentação do projeto. O compilador funciona como uma sequência de
transformações, com um módulo responsável por cada fase.

Na versão atual, compilar significa ler um arquivo `.mac` e verificar
se ele está correto nos níveis léxico, sintático e semântico. O projeto ainda
não produz executável, assembly ou código para uma placa.

## 1. Visão geral do pipeline

```text
arquivo .mac
    ↓
SourceFile: texto completo e caminho do arquivo
    ↓
Lexer: caracteres viram tokens
    ↓
Parser: tokens viram AST
    ↓
Semântica: AST recebe tipos e símbolos
    ↓
fonte aceita ou lista de diagnósticos
```

O ponto de entrada é `main.c`. Ele lê as opções da linha de comando e chama
`compiler_compile_file`, em `compiler.c`. Essa função coordena as três fases.
Uma fase só começa quando a anterior termina sem erros na fonte.

## 2. Mapa dos arquivos

### Entrada e coordenação

| Arquivo | Responsabilidade |
|---|---|
| `src/main.c` | Lê `argv`, reconhece as opções, exige um único arquivo `.mac` e chama o compilador. |
| `src/compiler.h` | Declara as opções, os códigos de saída e a função principal do front-end. |
| `src/compiler.c` | Carrega o arquivo, chama lexer, parser e semântica, imprime dumps e libera os recursos. |

### Código comum

| Arquivo | Responsabilidade |
|---|---|
| `src/common/status.h` | Define os três resultados possíveis de uma fase: sucesso, erro na fonte ou erro interno. |
| `src/common/source.h` | Define posição, intervalo e arquivo-fonte. |
| `src/common/source.c` | Abre o arquivo, lê os bytes e controla a memória do texto carregado. |
| `src/common/diagnostic.h` | Define a estrutura de um erro e a lista de erros. |
| `src/common/diagnostic.c` | Armazena e imprime mensagens com arquivo, linha, coluna, fase e código. |

### Analisador léxico

| Arquivo | Responsabilidade |
|---|---|
| `src/lexer/token.h` | Define todos os tipos de token e as estruturas `Token` e `TokenBuffer`. |
| `src/lexer/token.c` | Mantém o array dinâmico de tokens e imprime tokens para debug. |
| `src/lexer/lexer.h` | Expõe a função que executa o lexer. |
| `src/lexer/lexer.c` | Percorre os caracteres, reconhece lexemas e produz tokens. |

### Parser e AST

| Arquivo | Responsabilidade |
|---|---|
| `src/parser/ast.h` | Define os tipos de nós, o formato da AST e os campos preenchidos pela semântica. |
| `src/parser/ast.c` | Cria, guarda, imprime e destrói os nós da AST. |
| `src/parser/parser.h` | Expõe a função que executa o parser. |
| `src/parser/parser.c` | Consome tokens conforme a gramática e constrói a AST. |

### Análise semântica

| Arquivo | Responsabilidade |
|---|---|
| `src/semantic/type.h` | Define tipos primitivos, registros, vetores e a tabela de tipos. |
| `src/semantic/type.c` | Cria e consulta tipos. |
| `src/semantic/symbol.h` | Define símbolo, categoria, namespace, escopo e tabela de símbolos. |
| `src/semantic/symbol.c` | Declara símbolos e procura nomes no escopo atual e nos pais. |
| `src/semantic/semantic.h` | Define o resultado da análise e a API da fase semântica. |
| `src/semantic/semantic.c` | Verifica declarações, usos, chamadas, operadores, atribuições, retornos e vetores. |

### Documentação, exemplos e testes

| Caminho | Responsabilidade |
|---|---|
| `docs/gramatica.md` | Registra tokens, produções e regras estáticas da linguagem. |
| `docs/decisoes.md` | Registra como as lacunas do enunciado foram resolvidas. |
| `docs/pipeline.md` | Resume contratos, ownership e ordem de destruição. |
| `examples/algo.mac` | Programa Macaronica usado em demonstrações. |
| `tests/manifest.tsv` | Lista casos aceitos e rejeitados. |
| `tests/run_tests.sh` | Executa a suíte e confere códigos de saída e fases dos erros. |
| `Makefile` | Define como construir as versões debug, release e executar testes. |

## 3. Conceitos de C usados no projeto

### `enum`

Um `enum` dá nomes a valores inteiros. Por exemplo, `TokenKind` possui nomes
como `TOKEN_IDENTIFIER` e `TOKEN_PLUS`. Isso é melhor do que espalhar números
sem significado pelo código.

Em uma explicação curta, o `enum` responde: "qual é a categoria deste objeto?".

### `struct`

Uma `struct` agrupa dados relacionados. Um `Token`, por exemplo, guarda sua
categoria, posição no arquivo, início do lexema, tamanho e possível valor
literal.

Uma `struct` funciona como uma ficha que reúne os dados do mesmo objeto.

### `union`

Uma `union` permite que campos diferentes ocupem a mesma região de memória. Na
AST, o campo `kind` informa qual formato está ativo. Um nó binário usa os campos
de expressão binária; um nó de retorno usa os campos de retorno.

Não se deve acessar qualquer alternativa aleatoriamente. Primeiro consulta-se o
`kind`, normalmente em um `switch`, e depois acessa-se o membro correspondente.

### Ponteiros proprietários e não proprietários

Ownership indica quem é responsável por chamar `free`. Neste projeto:

- `SourceFile` possui o caminho e o buffer que carregou do disco;
- cada `Token` apenas aponta para uma parte desse buffer e não chama `free` nela;
- a AST possui seus nós e as cópias dos nomes e textos;
- `DiagnosticBag` possui as mensagens de erro, mas apenas referencia a fonte;
- `SemanticResult` possui as tabelas de tipos, símbolos e escopos.

Por isso, a fonte precisa continuar viva enquanto tokens e diagnósticos ainda
podem consultá-la.

### Arrays dinâmicos

Não sabemos antecipadamente quantos tokens, nós, símbolos ou erros existirão.
As listas guardam:

- `items`: endereço do array;
- `count`: quantos elementos estão sendo usados;
- `capacity`: quantos elementos cabem no espaço atual.

Quando `count` alcança `capacity`, usa-se `realloc` para aumentar o array,
normalmente dobrando sua capacidade. Dobrar evita realocar memória a cada novo
elemento.

As verificações que aparecem antes das alocações evitam overflow no cálculo do
tamanho. Elas parecem detalhadas, mas protegem o programa contra acesso fora da
memória.

## 4. Como funciona o lexer

O lexer recebe caracteres e produz tokens. Sua estrutura de estado guarda:

- o arquivo-fonte;
- a posição atual em bytes;
- linha e coluna;
- a lista de tokens;
- a lista de diagnósticos;
- uma indicação de falha interna.

O fluxo básico é:

1. Ignorar espaços e atualizar linha/coluna.
2. Observar o próximo caractere.
3. Se for letra, ler identificador ou palavra reservada.
4. Se for dígito, ler inteiro ou real.
5. Se for aspas, ler literal `palavra`.
6. Se for operador ou pontuação, emitir seu token.
7. Caso contrário, registrar erro léxico.
8. Ao final, emitir `TOKEN_EOF`.

O lexer usa maior casamento. Ao encontrar `/`, verifica se o
próximo caractere também é `/`, pois `//` é um único operador de divisão
inteira. O mesmo vale para `==`.

O suporte a UTF-8 existe principalmente para reconhecer corretamente `NÃO`,
textos e posições em caracteres. O lexer não verifica tipos e não constrói a
AST.

## 5. Como funcionam parser, precedência e AST

O parser recebe o `TokenBuffer`, guarda o índice do token atual e possui funções
como `check`, `advance`, `match` e `expect`:

- `check`: consulta o token atual sem consumi-lo;
- `advance`: consome um token;
- `match`: consome somente se o tipo for o esperado;
- `expect`: exige um tipo e produz erro sintático se ele não aparecer.

O parser é recursivo descendente: suas funções correspondem às produções da
gramática. Por exemplo, existem funções diferentes para declaração, bloco,
comando e expressão.

### Precedência

A precedência é obtida por níveis de funções. Simplificando:

```text
expressão lógica
    chama comparação
        chama soma
            chama multiplicação
                chama potência
                    chama expressão primária
```

Assim, ao analisar `2 + 3 * 4`, a multiplicação é construída antes da soma. A
AST resultante equivale a:

```text
      +
     / \
    2   *
       / \
      3   4
```

A AST guarda a estrutura já resolvida. A semântica não precisa tentar descobrir
novamente a precedência.

### Recuperação de erro

Depois de um erro, o parser procura um ponto seguro, como `;`, início de outro
comando ou fim do bloco. Essa sincronização permite encontrar mais de um erro e
garante que o parser sempre avance, evitando laço infinito.

Os limites de profundidade evitam estouro da pilha com arquivos excessivamente
aninhados.

## 6. Como funciona a análise semântica

A sintaxe pode estar correta e o programa ainda ter erros, como usar variável
não declarada ou atribuir `flut` a `inteira`. Esses casos pertencem à análise
semântica.

### Tipos

A `TypeTable` começa com os tipos nativos: `bool`, `inteira`, `flut`, `palavra`,
`duplocarpado` e `vazio`. Registros e vetores acrescentam tipos à tabela.

Cada tipo recebe um ID. Comparar IDs é mais simples e seguro do que repetir
comparações de texto pelo analisador inteiro. `TYPE_ID_ERROR` representa uma
expressão cujo tipo já falhou; ele ajuda a evitar vários erros derivados da
mesma causa.

### Símbolos, namespaces e escopos

Um símbolo registra nome, categoria, tipo, escopo e posição de declaração.
Exemplos de categorias são variável global, variável local, parâmetro, função,
registro e campo.

Há namespaces de tipos, funções e valores. Isso deixa explícito que procurar um
tipo não é a mesma operação que procurar uma variável.

O projeto possui escopo global, escopo de função e escopo de registro. Blocos de
`cond`, `repete` e `durante` não abrem outro escopo. A busca começa no escopo
atual e segue para o pai. Uma variável local pode sombrear uma global, mas duas
variáveis com o mesmo nome no mesmo escopo são erro.

### Duas etapas importantes

Antes de analisar os corpos, a semântica pré-coleta:

1. nomes de registros;
2. assinaturas de funções.

Isso permite recursão e chamada de uma função declarada mais adiante. Depois, as
declarações são analisadas na ordem do arquivo. Variáveis globais não são
antecipadas: uma função só enxerga as globais declaradas antes dela.

### Expressões e comandos

Analisar uma expressão significa calcular seu tipo e anotar o nó da AST. A
semântica verifica:

- os operandos aceitos por cada operador;
- o tipo dos dois lados de uma atribuição;
- o número e os tipos dos argumentos de uma chamada;
- o valor de `respost` contra o retorno da função;
- se o índice do vetor é `inteira`;
- se um índice calculável estaticamente está dentro dos limites;
- se um destino pode receber valor.

O programa deve declarar exatamente uma `principal`, e essa função não pode ser
chamada como uma função comum. Chamadas usadas como comando precisam retornar
`vazio`; um retorno `vazio` não pode ser usado como valor. Atribuições,
argumentos e retornos exigem tipos idênticos, pois não há promoção numérica.

Funções não vazias devem garantir `respost` em todos os caminhos. Uma cadeia de
`cond` cumpre essa regra quando tem um `casocontrario` final e todos os ramos
retornam. `durante` e `repete` não garantem retorno porque podem não executar.

`lvalue` significa um local que pode receber um valor, como uma variável ou um
elemento de vetor. `rvalue` significa algo que pode ser lido como valor. O vetor
completo não é nenhum dos dois nesta versão; somente `vetor{indice}` representa
um elemento utilizável.

## 7. Erros e códigos de saída

Um diagnóstico contém fase, código, mensagem e intervalo no arquivo. O formato
é semelhante a:

```text
arquivo.mac:linha:coluna: semantico E-SEM-007: identificador não declarado
```

As famílias são:

- `E-LEX-`: erro nos caracteres ou lexemas;
- `E-SIN-`: sequência de tokens não obedece à gramática;
- `E-SEM-`: sintaxe válida, mas significado inválido.

Os retornos do executável são:

| Código | Significado |
|---:|---|
| 0 | fonte aceita |
| 1 | erro na fonte |
| 2 | uso incorreto da linha de comando |
| 3 | falha ao ler o arquivo |
| 4 | falha interna, como falta de memória |

## 8. Por que existe `goto cleanup`

Em `compiler_compile_file`, várias operações podem falhar depois que alguns
recursos já foram criados. Repetir todas as chamadas de destruição antes de cada
`return` seria longo e fácil de errar.

O `goto cleanup` usado ali tem um destino único e serve apenas para chegar à
região comum de limpeza. Ele não cria saltos arbitrários entre algoritmos.

A ordem é:

```text
SemanticResult
AST
TokenBuffer
DiagnosticBag
SourceFile
```

Ela respeita as referências entre objetos. Por exemplo, diagnósticos podem
apontar para a fonte, então a fonte é destruída por último.

## 9. Debug e dumps

Nesta etapa, os rastros de debug estão habilitados no build padrão. As linhas
temporárias são marcadas no C com `// debug - remover` e aparecem em `stderr`.

Existem três dumps:

- `--dump-tokens`: mostra a saída do lexer;
- `--dump-ast`: mostra a árvore criada pelo parser;
- `--dump-symbols`: mostra escopos, símbolos e tipos após a semântica.

O dump da AST ocorre logo depois do parser, antes de a semântica preencher as
anotações de tipos e símbolos. Se a análise semântica falhar, o dump de símbolos
ainda pode mostrar as tabelas parciais construídas até o erro.

A versão `release` desliga os rastros temporários, mas mantém o funcionamento do
compilador.

## 10. Comandos para uma demonstração

No terminal MSYS2 UCRT64, dentro da pasta do projeto:

```sh
mingw32-make clean
mingw32-make
mingw32-make test
```

Executar o exemplo normalmente:

```sh
build/debug/macaronica.exe examples/algo.mac
```

Mostrar cada transformação:

```sh
build/debug/macaronica.exe --dump-tokens examples/algo.mac
build/debug/macaronica.exe --dump-ast examples/algo.mac
build/debug/macaronica.exe --dump-symbols examples/algo.mac
```

Mostrar tudo em uma execução:

```sh
build/debug/macaronica.exe --dump-tokens --dump-ast --dump-symbols examples/algo.mac
```

Demonstrar um erro de cada fase:

```sh
build/debug/macaronica.exe tests/invalidos/lexico_igual_isolado.mac
build/debug/macaronica.exe tests/invalidos/sintatico_ponto_virgula_ausente.mac
build/debug/macaronica.exe tests/invalidos/semantico_nao_declarado.mac
```

No PowerShell, se o MSYS2 não estiver no `Path`, pode-se usar:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
& "C:\msys64\ucrt64\bin\mingw32-make.exe"
.\build\debug\macaronica.exe .\examples\algo.mac
```

## 11. Roteiro oral de cinco minutos

### Minuto 1: objetivo e entrada

> Este projeto implementa o front-end da linguagem Macaronica em C99. Ele lê um
> arquivo `.mac` e executa análise léxica, sintática e semântica. A `main` cuida
> da linha de comando, e `compiler_compile_file` coordena o pipeline.

### Minuto 2: lexer

> O lexer percorre o texto mantendo byte, linha e coluna. Ele agrupa caracteres
> em tokens, como identificadores, números, palavras reservadas e operadores.
> Cada token aponta para seu lexema na fonte e guarda um `SourceSpan`. Erros de
> caractere, texto ou UTF-8 são registrados como diagnósticos léxicos.

### Minuto 3: parser e AST

> O parser é recursivo descendente. Suas funções representam as produções da
> gramática, e os níveis de funções de expressão implementam a precedência. Ele
> consome tokens e cria uma AST. Cada nó tem um `kind`, uma posição e os filhos
> apropriados para sua construção.

### Minuto 4: semântica

> A semântica cria tabelas de tipos, símbolos e escopos. Primeiro coleta
> registros e assinaturas para permitir recursão. Depois percorre as declarações
> em ordem, resolve nomes e verifica os usos. Funções não vazias precisam
> retornar um valor em todos os caminhos. A AST recebe anotações que poderão ser
> usadas por uma futura fase de IR.

### Minuto 5: robustez e limite da etapa

> Cada fase diferencia erro na fonte de falha interna. Os recursos têm donos
> definidos e são liberados em ordem no cleanup. A suíte cobre programas válidos
> e inválidos. Nesta etapa ainda não existe IR, otimização ou backend, portanto o
> compilador valida a fonte, mas ainda não gera executável nem código da placa.

## 12. Perguntas que o professor pode fazer

### Por que o token não copia seu lexema?

Porque o texto completo já está no `SourceFile`. O token guarda um ponteiro para
o primeiro byte e o tamanho da fatia. Isso evita uma alocação por token. Por
isso, o `SourceFile` precisa viver mais que o `TokenBuffer`.

### Por que a AST copia os nomes?

Porque a AST é um resultado próprio do parser e deve possuir os dados que usa.
Ela não depende de que cada nome continue sendo interpretado como uma fatia de
token.

### Qual a diferença entre token e nó da AST?

Token é uma unidade do texto, como `inteira`, `nome` ou `+`. Um nó representa
uma construção, como declaração, soma, chamada ou retorno, e aponta para seus
filhos.

### Como a precedência é implementada?

Cada nível de precedência possui uma função. A função de soma chama a de
multiplicação antes de montar o nó de soma; assim, multiplicação fica mais funda
na árvore e é executada primeiro no futuro.

### Por que existe um token EOF?

Ele representa explicitamente o fim da entrada. O parser pode perguntar pelo
token atual sem acessar além do array.

### Por que pré-coletar funções?

Para permitir recursão e chamadas a funções declaradas posteriormente. Somente
a assinatura é necessária nesse momento; o corpo é analisado depois.

### O que é sombreamento?

É quando um nome no escopo local esconde temporariamente um nome igual no escopo
global. A busca começa no escopo mais próximo e depois segue para o pai.

### Para que serve `TYPE_ID_ERROR`?

Ele marca uma expressão que já possui erro. As verificações seguintes podem
continuar sem fingir que ela possui um tipo válido e sem repetir mensagens para
a mesma causa.

### Por que usar `realloc`?

Porque a quantidade de elementos não é conhecida antes da análise. Quando o
array enche, `realloc` reserva um bloco maior e preserva os itens existentes.

### Por que usar `goto` na limpeza?

Para todas as saídas passarem pela mesma sequência de destruição. Nesse caso há
um único destino, no fim da função, e isso reduz repetição e vazamentos.

### O compilador já gera um programa executável?

Não. Ele já valida caracteres, gramática, nomes e tipos, mas ainda faltam IR,
otimizações e backend.

## 13. O que ainda não está implementado

O pipeline futuro será:

```text
AST semanticamente validada
    ↓
IR: representação intermediária
    ↓
otimizações
    ↓
backend específico da placa
    ↓
código alvo
```

Nesta versão não existem:

- execução ou interpretação do programa Macaronica;
- geração de IR;
- otimizações;
- geração de assembly ou código de máquina;
- backend para a placa;
- ligação de um executável final.

O lexer, o parser e o analisador semântico não contêm decisões específicas da
futura placa.

### O que já está preparado

Os nós da AST têm `node_id` e `SourceSpan`. A semântica acrescenta IDs de tipos
e símbolos e marca propriedades como valor legível, destino atribuível,
constante, vetor ou erro. Esses dados ajudam no lowering, mas só são válidos
enquanto o `SemanticResult` correspondente estiver vivo.

Hoje, `compiler_compile_file` libera a AST e o resultado semântico antes de
retornar. A próxima fase precisará criar uma unidade de compilação que preserve
esses objetos ou fazer o lowering antes da limpeza.

## 14. Resumo para memorizar

```text
Lexer: caracteres → tokens
Parser: tokens → AST
Semântica: AST → AST validada e anotada
IR/backend: ainda não implementados
```

Para apresentar a organização do projeto, explique quem produz, quem consome e
quem libera cada objeto desse caminho.
