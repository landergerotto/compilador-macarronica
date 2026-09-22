# Pipeline e ownership do front-end

O driver em [`compiler.c`](../src/compiler.c) executa esta sequência:

```text
SourceFile -> TokenBuffer -> AST -> SemanticResult
```

Lexer, parser e análise semântica só iniciam depois do sucesso da fase
anterior. Essas três fases podem deixar dados parciais quando encontram um
erro; o driver libera todo objeto que chegou a ser inicializado.

## Contrato dos objetos

- `source_file_load` recebe um `SourceFile` vazio. Quando termina com sucesso,
  o objeto passa a possuir o caminho e o buffer carregados. A destruição é feita
  por `source_file_destroy`.
- `lexer_scan_all` recebe `TokenBuffer` e `DiagnosticBag` inicializados. Cada
  token aponta para uma fatia do buffer da fonte e não libera esse texto.
- `parser_parse` consulta a fonte e os tokens. A AST possui seus nós e as cópias
  de nomes e textos. Em um erro sintático recuperável, `out_program` pode conter
  uma árvore parcial que também precisa ser destruída.
- `semantic_result_init` deve ser chamado antes de `semantic_analyze`. Um mesmo
  resultado é analisado uma única vez. A análise anota a AST com IDs de tipos e
  símbolos pertencentes ao `SemanticResult`.
- `DiagnosticBag` possui as mensagens, mas guarda uma referência não
  proprietária para o `SourceFile` de cada diagnóstico.

A fonte deve permanecer viva enquanto houver tokens ou diagnósticos que a
referenciem. Um consumidor da AST tipada precisa manter a AST e o
`SemanticResult` vivos enquanto usar as anotações semânticas.

## Dados preparados para a IR

Cada nó da AST tem `node_id` e `SourceSpan`. Depois da análise semântica, os
campos `semantic.type_id`, `semantic.symbol_id` e `semantic.flags` registram o
tipo resolvido, o símbolo associado e propriedades como `lvalue`, `rvalue`,
constante, vetor ou erro. Referências de tipo e nomes declarados também recebem
IDs resolvidos.

Esses IDs só valem enquanto o `SemanticResult` correspondente estiver vivo. A
futura IR terá seus próprios valores e um mapa entre `node_id` e valor da IR;
IDs da IR não devem ser gravados na AST.

A API pública atual, `compiler_compile_file`, retorna apenas um código de saída
e destrói a AST e o resultado semântico antes de terminar. O lowering para IR
vai exigir uma unidade de compilação que preserve esses objetos ou deverá
acontecer dentro do driver antes da limpeza. Essa interface ainda não existe.

## Status das fases

Cada fase retorna um destes valores:

- `FRONTEND_STATUS_OK`
- `FRONTEND_STATUS_SOURCE_ERROR`
- `FRONTEND_STATUS_INTERNAL_ERROR`

Erros na fonte resultam no código de saída 1. Falta de memória ou falhas
internas resultam no código 4, mesmo quando já existem diagnósticos da fonte. O
driver imprime o `DiagnosticBag` uma vez antes da limpeza.

Um erro léxico impede a execução do parser. Um erro sintático impede a análise
semântica. Quando a semântica encontra um erro, ela ainda pode manter tabelas
parciais para o dump de símbolos.

## Ordem de limpeza

`compiler_compile_file` libera os objetos nesta ordem:

```text
SemanticResult
AST
TokenBuffer
DiagnosticBag
SourceFile
```

O `SourceFile` fica por último porque tokens e diagnósticos podem apontar para
seu caminho ou seu buffer.
