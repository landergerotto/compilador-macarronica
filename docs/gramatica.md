# Gramática operacional da Macaronica

**Versão:** 0.2.0 (contrato do PjBL 3)

**Notação:** EBNF; `{ X }` na metalinguagem significa repetição e `[ X ]` significa opcionalidade. Terminais aparecem entre aspas. As chaves e colchetes da linguagem aparecem como terminais `"{"`, `"}"`, `"["` e `"]"`.

Esta é a gramática implementável derivada das regras oficiais e das decisões explícitas em `decisoes.md`. Quando a fonte não determinou uma regra, a produção abaixo é uma decisão de projeto, não uma afirmação sobre o PDF.

As extensões do PjBL 3 incluem acesso a campos e funções intrínsecas de E/S, além da assinatura restrita de `principal`. Este documento especifica o comportamento esperado; o estado implementado e verificado deve ser consultado no README e nos resultados de testes.

## 1. Léxico

### 1.1 Tokens e palavras reservadas

| Grupo | Lexemas/tokens | Origem |
|---|---|---|
| Tipos nativos | `bool`, `inteira`, `flut`, `palavra`, `duplocarpado`, `vazio` | Fonte; `vazio` somente como retorno |
| Estruturas | `receba`, `repete`, `durante`, `cond`, `casocontrario`, `principal`, `respost` | Fonte |
| Estruturas usadas nas formas oficiais | `registro`, `outrafuncao`, `vet`, `edai` | Fonte, embora ausentes da lista resumida |
| Reservada sem produção | `func` | Fonte; reservada, mas sem uso gramatical em 0.1.0 |
| Booleanos | `VER`, `FAL` | Fonte |
| Lógicos | `OU`, `E`, `NÃO`, `XOR` | Fonte |
| Intrínseca | `raiz` | Fonte, tokenizada separadamente de `(` e `)` |
| Comparadores | `<`, `>`, `==`, `!` | Fonte; `!` significa diferente |
| Aritméticos | `+`, `-`, `*`, `/`, `//`, `^` | Fonte |
| Pontuação | `;`, `,`, `@`, `(`, `)`, `[`, `]`, `{`, `}` | Fonte/estruturas oficiais |
| Acesso a campo | `.` | Extensão do projeto, D-013 |
| Literais | `LIT_INTEIRA`, `LIT_REAL`, `LIT_PALAVRA` | Decisão D-004 |
| Nomes e controle | `IDENT`, `EOF`, `INVALIDO` | Necessários à implementação |

O lexer usa maior casamento: `//` precede `/` e `==` precede `=`. Um `=` isolado é inválido. Um `!` isolado é válido e é o comparador binário de diferença. Não há token de comentário.

### 1.2 Formas regulares

```text
LETRA         = [A-Za-z]
DIGITO        = [0-9]
IDENT         = LETRA (LETRA | DIGITO)*
LIT_INTEIRA   = DIGITO+
LIT_REAL      = DIGITO+ "." DIGITO+
LIT_PALAVRA   = '"' (CARACTERE_SEGURO | ESCAPE)* '"'
ESCAPE        = "\\\\" | "\\\"" | "\\n" | "\\r" | "\\t"
ESPACO        = " " | "\t" | "\r" | "\n"
```

`CARACTERE_SEGURO` é qualquer caractere UTF-8 válido, exceto aspas, barra invertida, CR ou LF. Espaços são descartados após atualizar linha e coluna. Não existe sinal dentro do token numérico: `-12` é `-` seguido de `LIT_INTEIRA`. Os tipos e limites semânticos dos números seguem D-021: `inteira` tem 32 bits e o literal real tem precisão `flut` binary32. A magnitude `2147483648` só é admitida como literal diretamente negado para formar `-2147483648`.

Ao construir a AST, `LIT_PALAVRA` produz `AST_LITERAL_EXPR` com `AST_LITERAL_STRING` e o conteúdo já decodificado conforme `ESCAPE`. O nó preserva o `SourceSpan` de toda a grafia original, incluindo as aspas. A decodificação ocorre uma única vez no pipeline.

### 1.3 Erros léxicos mínimos

- caractere fora do alfabeto operacional;
- `=` não seguido de `=`;
- literal de palavra sem fechamento antes de quebra de linha ou EOF;
- escape textual desconhecido;
- sequência UTF-8 inválida;
- número real incompleto, como `12.`;
- literal inteiro acima de `LLONG_MAX` ou literal real cuja conversão ultrapassa `DBL_MAX`, limites do armazenamento temporário do lexer.

Mesmo quando a conversão léxica cabe no armazenamento temporário, a semântica rejeita valores que excedam a faixa da linguagem de D-021. Não ocorre truncamento silencioso no codegen.

## 2. Unidade de compilação

```ebnf
programa = { declaracao_global }, EOF ;

declaracao_global = declaracao_registro
                  | declaracao_funcao
                  | declaracao_principal
                  | declaracao_variavel
                  | declaracao_vetor
                  ;
```

Restrição semântica: deve haver exatamente uma `declaracao_principal`.

## 3. Tipos e declarações

```ebnf
tipo_retorno = tipo_valor | "vazio" ;

tipo_valor = "bool"
           | "inteira"
           | "flut"
           | "palavra"
           | "duplocarpado"
           | IDENT                  (* nome de registro já declarado *)
           ;

declaracao_variavel = tipo_valor, lista_nomes, ";" ;
lista_nomes         = IDENT, { ",", IDENT } ;

declaracao_vetor = tipo_valor, "vet", "{", LIT_INTEIRA, "}",
                   lista_nomes, ";" ;

declaracao_registro = "registro", IDENT, "@", "[",
                      { declaracao_campo }, "]" ;
declaracao_campo    = tipo_valor, IDENT, ";" ;
```

Restrições semânticas:

- `vazio` não declara variável, vetor, campo ou parâmetro;
- tamanho de vetor está entre 1 e `2147483647`;
- nomes de campos são únicos dentro do registro;
- duplicata no mesmo escopo é erro.
- tipos de registro são nominais; ciclos de campos por valor, diretos ou indiretos, são proibidos.

Variáveis locais e globais têm os valores iniciais definidos em D-022: zero numérico, `FAL`, texto vazio e inicialização recursiva de agregados. Todos os locais são inicializados uma vez na entrada da função.

## 4. Funções

```ebnf
declaracao_funcao = "outrafuncao", tipo_retorno, IDENT,
                    "(", [ parametros ], ")", "@", bloco ;

declaracao_principal = ( "inteira" | "vazio" ), "principal",
                       "(", ")", "@", bloco ;

parametros = parametro, { ",", parametro } ;
parametro  = tipo_valor, IDENT ;

bloco = "[", { comando }, "]" ;
```

`func` não participa desta gramática porque a fonte o lista como reservada, mas usa `outrafuncao` na estrutura oficial. Ao encontrar `func`, o parser deve sugerir `outrafuncao` ou a forma especial de `principal`.

A restrição de `principal` é uma decisão de projeto do PjBL 3 (D-015); se o parser aceitar a forma mais ampla para recuperação, a semântica deve rejeitar parâmetros e outros retornos. A IR expõe `i32 @zl_principal()`. Retorno inteiro fornece o status do processo; principal vazia fornece zero.

## 5. Comandos

```ebnf
comando = declaracao_variavel
        | declaracao_vetor
        | atribuicao
        | chamada_comando
        | repeticao_repete
        | repeticao_durante
        | decisao
        | retorno
        ;

atribuicao = atribuicao_sem_ponto_virgula, ";" ;

atribuicao_sem_ponto_virgula = destino, "receba", expressao ;

destino = ( IDENT | acesso_vetor ), { ".", IDENT } ;

chamada_comando = chamada, ";" ;

retorno = "respost", [ expressao ], ";" ;
```

Somente chamada com retorno `vazio` pode ser usada como `chamada_comando`. `respost` com ou sem expressão é validado conforme o tipo da função, pela decisão D-008, e termina imediatamente sua execução. Um acesso a campo só pode ser destino quando a base também pode receber atribuição. Um campo de resultado temporário de função pode ser lido, mas não atribuído.

## 6. Controle

### 6.1 Condições

```ebnf
condicao = expressao ;
```

A fonte limita condições a `VAR COMPARADOR VAR | VAR COMPARADOR NUM | NUM COMPARADOR NUM`. A decisão D-009 aprova uma extensão do projeto: qualquer expressão é aceita sintaticamente como condição, mas seu tipo final deve ser `bool`. Essa extensão pertence à Macaronica do projeto e não importa regras de outra linguagem.

### 6.2 Estruturas

```ebnf
repeticao_repete = "repete", "(",
                   atribuicao_sem_ponto_virgula, ",",
                   condicao, ",",
                   atribuicao_sem_ponto_virgula,
                   ")", "edai", bloco ;

repeticao_durante = "durante", "(", condicao, ")",
                    "edai", bloco ;

decisao = "cond", "(", condicao, ")", "edai", bloco,
          { "casocontrario", "cond", "(", condicao, ")",
            "edai", bloco },
          [ "casocontrario", "edai", bloco ] ;
```

`casocontrario` não pode aparecer isolado e a alternativa sem `cond` deve ser a última da cadeia.

## 7. Expressões

As expressões aparecem em atribuições, argumentos, retornos, índices e condições de controle. Em uma condição, a expressão é sintaticamente geral e deve resultar em `bool` após a análise semântica.

```ebnf
expressao         = expr_ou ;

expr_ou           = expr_xor, { "OU", expr_xor } ;
expr_xor          = expr_e,   { "XOR", expr_e } ;
expr_e            = expr_nao, { "E", expr_nao } ;
expr_nao          = "NÃO", expr_nao
                  | expr_comparacao
                  ;

expr_comparacao   = expr_aditiva,
                    [ ( "<" | ">" | "==" | "!" ), expr_aditiva ] ;

expr_aditiva      = expr_multiplicativa,
                    { ( "+" | "-" ), expr_multiplicativa } ;

expr_multiplicativa = expr_potencia,
                      { ( "*" | "/" | "//" ), expr_potencia } ;

expr_potencia     = expr_unaria, [ "^", expr_potencia ] ;
expr_unaria       = ( "+" | "-" ), expr_unaria
                  | expr_alta
                  ;

expr_alta         = ( chamada | acesso_vetor | raiz | primaria ),
                    { ".", IDENT } ;

chamada           = IDENT, "(", [ argumentos ], ")" ;
argumentos        = expressao, { ",", expressao } ;

acesso_vetor      = IDENT, "{", expressao, "}" ;
raiz              = "raiz", "(", expressao, ")" ;

primaria          = IDENT
                  | LIT_INTEIRA
                  | LIT_REAL
                  | LIT_PALAVRA
                  | "VER"
                  | "FAL"
                  | "(", expressao, ")"
                  ;
```

### 7.1 Precedência

| Nível | Operadores/formas | Associatividade |
|---:|---|---|
| 10 (maior) | chamada `()`, indexação `{}`, `raiz()`, campo `.` | campos encadeiam à esquerda; chamada/indexação seguem as formas da EBNF |
| 9 | unários `+`, `-` | direita |
| 8 | `^` | direita |
| 7 | `*`, `/`, `//` | esquerda |
| 6 | `+`, `-` | esquerda |
| 5 | `<`, `>`, `==`, `!` | não associativo |
| 4 | `NÃO` | direita |
| 3 | `E` | esquerda |
| 2 | `XOR` | esquerda |
| 1 (menor) | `OU` | esquerda |

Comparações encadeadas como `a < b < c` são erro sintático. Pela tabela, `NÃO a == b` significa `NÃO (a == b)`, e `-2 ^ 2` significa `(-2) ^ 2`.

Os operadores lógicos podem se repetir sem limite gramatical. Por exemplo, `a < b E c < d E e < f` associa à esquerda. Em `a < b OU c < d E e < f XOR VER`, primeiro são resolvidas as comparações, depois `E`, depois `XOR` e por último `OU`. Parênteses podem alterar esse agrupamento.

Essa precedência define a forma da AST. A execução segue D-020: operandos e argumentos da esquerda para a direita; `E` só executa a direita se a esquerda for verdadeira e `OU` só a executa se a esquerda for falsa. `XOR` executa ambos. Na atribuição, o endereço do destino é calculado antes do valor atribuído.

## 8. Regras estáticas de tipos

| Construção | Tipos aceitos | Resultado |
|---|---|---|
| `+`, `-`, `*` binários | operandos idênticos em `inteira`, `flut` ou `duplocarpado` | tipo dos operandos |
| `/` | operandos idênticos em `flut` ou `duplocarpado` | tipo dos operandos |
| `//` | `inteira`, `inteira` | `inteira` |
| `^` | operandos idênticos e numéricos | tipo dos operandos |
| `+`, `-` unários | um numérico | tipo do operando |
| `raiz` | `flut` ou `duplocarpado` | tipo do operando |
| `<`, `>` | operandos idênticos e numéricos | `bool` |
| `==`, `!` | operandos escalares idênticos | `bool` |
| `NÃO` | `bool` | `bool` |
| `E`, `OU`, `XOR` | `bool`, `bool` | `bool` |
| `nome{indice}` | `nome` é vetor; índice `inteira` | tipo do elemento |
| `expr.campo` | base é registro e campo existe | tipo declarado do campo |
| `nome(args)` | argumentos compativeis com a assinatura conforme D-012 | retorno declarado |

Atribuicao, argumento e retorno aceitam tipos identicos ou promocao `inteira -> flut`, `inteira -> duplocarpado` e `flut -> duplocarpado`. Conversoes inversas e casts explicitos nao sao aceitos. Operadores mantem as regras da tabela, sem promocao de operandos. Nao ha concatenacao de `palavra`. `vazio` nao e tipo de valor.

Um identificador que nomeia um vetor não é, isoladamente, `lvalue` nem `rvalue`. O vetor completo não pode ser lido, copiado, atribuído, passado como argumento ou retornado. Somente `nome{indice}` denota um elemento com o tipo declarado do vetor e pode funcionar como destino ou valor.

Registros do mesmo tipo nominal podem ser copiados, passados e retornados por valor. O acesso `expr.campo` tem capacidade de valor quando sua base é valor, e capacidade de destino quando sua base é destino. A ordem textual dos campos determina sua posição no tipo LLVM. `palavra` usa igualdade por conteúdo. As operações numéricas e falhas verificadas em execução seguem D-021 e D-023; `/` de inteiros continua proibida.

### 8.1 Intrínsecas de entrada e saída

As funções abaixo são predeclaradas no namespace global de funções e não podem ser redefinidas. Não acrescentam palavras reservadas ao lexer e usam a produção `chamada` já existente. As regras de compatibilidade de D-012 se aplicam aos argumentos.

| Função | Parâmetros | Retorno |
|---|---|---|
| `imprimeInteira` | um `inteira` | `vazio` |
| `imprimeFlut` | um `flut` | `vazio` |
| `imprimeDuplocarpado` | um `duplocarpado` | `vazio` |
| `imprimePalavra` | um `palavra` | `vazio` |
| `imprimeBool` | um `bool` | `vazio` |
| `imprimeNl` | nenhum | `vazio` |
| `leInteira` | nenhum | `inteira` |
| `leFlut` | nenhum | `flut` |
| `leDuplocarpado` | nenhum | `duplocarpado` |

A saída não acrescenta separadores. `imprimeNl()` emite a nova linha. Os formatos e erros de leitura seguem D-017. Nomes `zl_*` são símbolos de runtime C/LLVM, mapeados em `decisoes_backend.md`, e não identificadores válidos da fonte Macaronica.

## 9. Validações semânticas mínimas

1. Pré-coletar os nomes de todos os registros e as assinaturas de todas as funções no escopo global, permitindo referências a registros e funções declarados posteriormente, inclusive recursão.
2. Exigir exatamente uma `principal`, sem parâmetros e com retorno `inteira` ou `vazio`.
3. Rejeitar duplicata no mesmo namespace e escopo.
4. Exigir declaração anterior ao uso de variável. Globais seguem ordem textual: uma função vê apenas as globais declaradas antes dela, ainda que seu corpo seja analisado posteriormente.
5. Validar alvo e tipo de cada atribuição.
6. Validar quantidade e tipos de argumentos de chamada.
7. Impedir uso de retorno `vazio` como valor.
8. Validar `respost` contra o retorno declarado. Toda função não vazia deve garantir retorno em todos os caminhos: uma cadeia `cond` precisa de `casocontrario` final e retorno em todos os ramos; `durante` e `repete` não garantem retorno.
9. Exigir resultado `bool` nas condições de `cond`, `casocontrario cond`, `durante` e `repete`.
10. Validar operadores pela tabela da seção 8.
11. Validar tamanho e índice de vetores; conferir o limite quando a expressão inteira do índice puder ser avaliada estaticamente; rejeitar qualquer uso do vetor inteiro como valor ou destino.
12. Não criar escopo novo para blocos de controle nesta versão.
13. Validar acesso a campo e capacidade de destino, impedir ciclos de registros e preservar identidade nominal.
14. Validar a faixa numérica e o tamanho de vetor conforme D-021, incluindo o caso especial do mínimo inteiro.
15. Predeclarar e proteger as assinaturas das intrínsecas de E/S de D-017.

## 10. Limites operacionais

O parser limita expressões e blocos a 512 níveis de profundidade. Esse limite
protege a pilha do processo e produz `E-SIN-009`; ele pertence à implementação,
não à definição da linguagem.

## 11. Diagnósticos esperados

Formato comum:

```text
arquivo.mac:linha:coluna: fase CODIGO: mensagem
```

Famílias recomendadas:

| Fase | Prefixo | Exemplos |
|---|---|---|
| Léxica | `E-LEX-` | caractere inválido, texto não terminado, escape inválido |
| Sintática | `E-SIN-` | token inesperado, `;` ausente, delimitador desbalanceado |
| Semântica | `E-SEM-` | nome não declarado, duplicata, incompatibilidade de tipo, assinatura, retorno |

O token inesperado e sua grafia devem aparecer no erro sintático quando disponíveis. Após erro, toda fase precisa avançar ou sincronizar; nenhuma entrada inválida pode provocar travamento ou laço infinito.

## 12. Exemplos conformes à versão 0.1.0

```text
outrafuncao inteira quadrado(inteira n) @
[
    respost n * n;
]

inteira principal() @
[
    inteira numero, resultado;
    numero receba 5;
    resultado receba quadrado(numero);

    cond (resultado > 20 E numero > 0 OU resultado == 0 XOR FAL) edai
    [
        respost 1;
    ]
    casocontrario edai
    [
        respost 0;
    ]
]
```

```text
vazio principal() @
[
    inteira vet{10} valores;
    inteira i;
    i receba 0;

    repete (i receba 0, i < 10, i receba i + 1) edai
    [
        valores{i} receba i;
    ]

    respost;
]
```

## 13. Formas intencionalmente não aceitas

```text
// comentário            # `//` é divisão inteira
cond (1 + 2) edai [ ]    # sintaticamente válida, mas o resultado não é `bool`
cond (a < b < c) edai [ ] # comparações não podem ser encadeadas
inteira x;
x receba 1.0;            # flut nao pode ser convertido implicitamente para inteira
func inteira f() @ [ ]   # usar `outrafuncao`
```
