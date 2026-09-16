# Pipeline e ownership do front-end

O driver executa `SourceFile -> TokenBuffer -> AST -> SemanticResult`. Uma fase
so e iniciada quando a anterior retorna `FRONTEND_STATUS_OK`. Cada funcao pode
deixar resultado parcial quando retorna erro; por isso o driver sempre destroi
todos os objetos que chegou a inicializar.

## Contratos

- `source_file_load` recebe um `SourceFile` vazio e passa a possuir caminho e
  buffer. `source_file_destroy` aceita uma estrutura vazia.
- `lexer_scan_all` recebe um `TokenBuffer` inicializado e acrescenta tokens. Os
  lexemas sao fatias nao proprietarias do buffer da fonte.
- `parser_parse` apenas consulta fonte e tokens. A AST devolvida possui seus nos
  e as copias de nomes/textos.
- `semantic_result_init` deve preceder `semantic_analyze`. A analise anota a AST
  com IDs que referenciam tabelas pertencentes ao `SemanticResult`.
- `DiagnosticBag` possui as mensagens, mas cada diagnostico referencia a fonte
  sem possui-la.

Consumidores futuros da AST tipada devem manter vivos a AST e o resultado
semantico. Os diagnosticos devem ser impressos enquanto a fonte ainda existe.
A futura IR deve manter seu proprio mapa `node_id -> valor IR`; IDs de valores
da IR nao pertencem a AST e nao ficam armazenados nela.

## Retornos e limpeza

Cada fase retorna `FRONTEND_STATUS_OK`, `FRONTEND_STATUS_SOURCE_ERROR` ou
`FRONTEND_STATUS_INTERNAL_ERROR`. Assim, falta de memoria e falhas de
infraestrutura resultam no codigo de saida 4 mesmo se ja houver diagnosticos da
fonte. O driver imprime o bag uma unica vez e limpa na ordem:

```text
SemanticResult
AST
TokenBuffer
DiagnosticBag
SourceFile
```

Erro lexico impede parser e semantica; erro sintatico impede semantica. Uma
analise semantica com erro ainda pode produzir tabela parcial apenas para debug.
