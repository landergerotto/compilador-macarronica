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
