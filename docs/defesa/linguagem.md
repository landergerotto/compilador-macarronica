# Defesa: frente linguagem (front-end e back-end LLVM)

Material de estudo do Luiz para a arguição. Cada seção diz o que mudou, por
que mudou, onde está no código e as perguntas prováveis com resposta curta.

## 1. Correção do PjBL 1/2

### 1.1 Crash em `inteira a, ;`, `inteira a, 5;` e `inteira a, vet{3} c;`

**O que acontecia.** Em `parse_declaration_after_type`
([src/parser/parser.c](../../src/parser/parser.c)), o laço das vírgulas lê um
nome depois de cada `,`. Quando não havia nome, `expect` devolvia `NULL`, o
laço saía com `break` e a linha seguinte fazia
`node->span.end = last_end(parser, name->span.end)`: leitura por ponteiro
nulo, *segmentation fault*. Os 66 crashes do fuzzing do professor tinham todos
essa causa.

**Correção.** A função guarda `last_name`, o último nome aceito, e fecha o
intervalo do nó com ele. `name` só é usado logo depois de ser testado.

**Testes.** `tests/invalidos/sintatico_declaracao_virgula_sem_nome.mac`,
`..._virgula_literal.mac` e `..._vetor_misturado.mac`, registrados no
`tests/manifest.tsv` como rejeitados na fase sintática.

**Perguntas prováveis.**

- *Por que o programa travava em vez de dar erro?* O parser já relatava o
  erro, mas depois usava o ponteiro do nome que faltava para calcular a
  posição final do nó.
- *Como garantir que não há outro caso igual?* Todo `expect` que pode falhar
  é testado antes do uso; o fuzzing com ASan (`make sanitize` no WSL) cobre
  entradas mutadas.

### 1.2 Cascatas de erro: um diagnóstico por erro real

**O que acontecia.** `inteira vet{n} v;` gerava 5 erros; `b receba 2 2;` e
`b receba NAO VER;`, 2 cada. Depois do primeiro erro, o parser continuava do
ponto exato em que parou, e cada token fora do lugar virava um erro novo. A
sincronização antiga também parava em qualquer identificador, mesmo no meio
do comando errado.

**Como funciona agora: modo pânico com sincronização**
([src/parser/parser.c](../../src/parser/parser.c)).

1. O primeiro erro de um comando liga `parser->panic_mode` (em
   `syntax_error`). Enquanto ele está ligado, nenhum erro é relatado: os
   próximos são consequência do primeiro.
2. Quem lê uma sequência chama `synchronize` depois de um item com erro.
   Sequências são os comandos de um bloco (`parse_block_items`), os campos de
   um registro (`parse_struct`) e as declarações globais (`parser_parse`). A
   função descarta tokens até um ponto seguro e desliga o modo pânico.
3. Pontos seguros:
   - depois de `;` ou de um bloco `[ ... ]` completo;
   - antes de `]` e do fim do arquivo;
   - antes de uma palavra que só começa comando ou declaração (`repete`,
     `durante`, `cond`, `respost`, tipos);
   - antes de um identificador que abre uma linha nova com cara de comando
     (`x receba`, `f(`). Isso cobre o `;` esquecido na linha anterior.
4. Um `[` de verdade desliga o modo pânico: um erro no cabeçalho de um `cond`
   não esconde os erros reais do corpo dele.
5. Recuperações locais, que não descartam o comando inteiro:
   - cabeçalho entre parênteses com erro: pula até o `)` (`close_header`) e
     o corpo ainda é analisado;
   - `vet{n}` e `vet{2 + 1}`: um erro, e o resto da declaração segue;
   - `func` no lugar de `outrafuncao` e função sem `outrafuncao`: um erro, e
     a função é lida inteira;
   - função sem nome: pula o cabeçalho e lê o corpo.
6. **Saldo de colchetes.** Antes de começar, o parser conta `[` menos `]` no
   arquivo. O saldo decide recuperações que, sem ele, seriam um chute:
   - saldo negativo e bloco sem `[`: supõe o `[` esquecido e lê o bloco, para
     que o `]` dele não feche o bloco de fora;
   - saldo positivo e `[` solto no início de um comando: descarta só o `[`;
   - saldo positivo e `casocontrario` dentro do ramo: fecha o ramo ali;
   - saldo negativo e comando logo depois de um `]` no nível global: esse `]`
     fechou a função cedo. Um erro só, e os comandos órfãos são descartados.

   Com saldo zero, `cond (a) edai respost;` (sem colchete nenhum) não é
   confundido com um `[` esquecido.
7. **Léxico:** `NAO`, `não` e `Não` geram `E-LEX-007` ("a negação lógica se
   escreve 'NÃO'"). O lexer emite o token da negação mesmo assim, e o parser
   não vê nada estranho depois.
8. **Semântica:**
   - duas `principal` geram só `E-SEM-003` (redefinição); `E-SEM-004` agora
     é só para a falta de `principal`;
   - `destino receba origem` com dois vetores gera um `E-SEM-026` só.

**Como foi verificado.**

- 18 cenários escritos à mão, um por tipo de erro, dão 1 diagnóstico cada. O
  cenário com dois erros reais em comandos diferentes dá 2.
- `tests/manifest.tsv` ganhou a coluna `erros`, e `tests/run_tests.sh` exige o
  número exato. Prova de que a checagem funciona: com o esperado do caso de
  dois erros trocado para 1, o teste falha ("esperava 1 diagnóstico(s),
  recebeu 2").
- O mini-fuzzer `tests/fuzz/cascata.sh` gera duas mutações por token de um
  programa válido: sem o token e com ele duplicado. Cada mutação é um erro
  real, então pode dar no máximo um diagnóstico. Na base
  `tests/fuzz/base_cascata.mac` são 312 mutações. Só com o modo pânico, 29
  ainda davam cascata, quase todas de colchete. Com o saldo de colchetes e as
  recuperações de cabeçalho, nenhuma dá cascata, e nenhuma trava.

**Perguntas prováveis.**

- *O que é modo pânico?* Depois do primeiro erro, o parser para de relatar e
  descarta tokens até um ponto onde a gramática recomeça com segurança (fim de
  comando, início de comando). É a técnica clássica de recuperação dos
  parsers descendentes recursivos.
- *Por que não relatar todos os erros que aparecem?* Os erros seguintes do
  mesmo comando são consequência do primeiro. Relatá-los confunde: o
  programador corrige um e metade da lista some.
- *E se houver dois erros de verdade?* Saem dois diagnósticos, desde que
  estejam em comandos diferentes. O teste
  `invalidos/sintatico_dois_erros_reais.mac` cobre isso.
- *Pode entrar em laço infinito?* Não. Se o item com erro não consumiu nada,
  `synchronize` consome pelo menos um token (variável `progressed`), e o fim
  do arquivo sempre encerra os laços. O pulo de blocos é iterativo, sem
  recursão, então 513 blocos aninhados não estouram a pilha.
- *Para que o saldo de colchetes?* Para distinguir um `[` esquecido de um
  comando sem colchetes. Em `cond (a) edai x receba 1; ]`, sobrando um `]`, o
  melhor é supor o `[`; em `cond (a) edai respost;`, com saldo zero, o melhor
  é descartar o comando. Sem o saldo, qualquer escolha erra um dos dois casos.
- *Por que `NAO` virou erro, e não um nome?* `NAO` como nome quase sempre é a
  negação sem til (teclado sem acento). Como erro léxico com a grafia certa, o
  programador entende de imediato. `nao` e `Nao` continuam nomes válidos.
