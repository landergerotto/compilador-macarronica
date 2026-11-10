# Guia de defesa — Macaronica PjBL 3

Este guia prepara a explicação do compilador a partir dos tópicos reunidos
nas instruções do Student Reviewer e do código atual. As perguntas abaixo
são exercícios de preparação do grupo, não transcrições atribuídas ao
professor. A semântica da linguagem está em [decisoes.md](decisoes.md), e o
contrato LLVM/runtime em [decisoes_backend.md](decisoes_backend.md).

## 1. Entrada, saída e responsabilidade de cada fase

| Fase | Entrada → saída | Onde observar |
|---|---|---|
| Lexer | Caracteres UTF-8 → tokens com posição | [lexer.c](../src/lexer/lexer.c), `--dump-tokens` |
| Parser | Tokens → AST | [parser.c](../src/parser/parser.c), `--dump-ast` |
| Semântica | AST → AST validada com IDs de tipos e símbolos | [semantic.c](../src/semantic/semantic.c), `--dump-symbols` |
| Codegen | AST anotada + tabelas → LLVM IR textual | [codegen_llvm.c](../src/ir/codegen_llvm.c), opção `-o` |
| Ferramentas LLVM | IR → IR verificada, otimizada ou executada | `llvm-as`, `opt`, `lli`, `clang` |
| Runtime | Chamadas `zl_*` → E/S e serviços do hospedeiro | [zl_runtime_posix.c](../runtime/zl_runtime_posix.c) |

O lexer distingue lexema de token: a grafia `receba` é o lexema e sua classe
é um token. O parser usa lookahead e recursão descendente para aplicar a
gramática e construir a hierarquia de operadores. A AST conserva a estrutura
significativa, sem manter pontuação que só orientou o parsing.

A semântica usa ambientes de nomes, escopos e tipos para verificar o programa.
As anotações podem ser relacionadas à ideia de atributos: tipos resultantes
são calculados a partir de operandos, enquanto o contexto de função fornece
o retorno esperado. Isso não significa que o projeto utilize uma ferramenta
geradora de gramáticas de atributos.

O codegen recebe IDs já resolvidos. Ele consulta as tabelas, sem refazer a
resolução de nomes. [compiler.c](../src/compiler.c) só chama
`codegen_llvm_emit` após sucesso semântico e mantém AST e tabelas vivas até o
fim da emissão. Um diagnóstico de fonte impede a publicação da saída `.ll`.

## 2. AST, 3AC, DAG e LLVM IR

A AST descreve construções como atribuição, chamada e condicional. LLVM IR
descreve operações tipadas, funções, blocos e transferências de controle.
Essas representações atendem a fases diferentes.

O padrão de três endereços ajuda a compreender uma instrução como
`%t.2 = add i32 %t.0, %t.1`: um resultado e dois operandos. O projeto gera
LLVM IR diretamente; não constrói uma segunda IR própria de 3AC.

Um DAG poderia representar compartilhamento de subexpressões comuns.
A AST atual é uma árvore com propriedade dos filhos, e o codegen visita os
operandos na ordem definida. Ele não junta subexpressões por conta própria,
porque chamadas, leitura e alterações de globais têm efeitos observáveis.
O LLVM pode eliminar trabalho redundante quando provar que isso é seguro.

Em [codegen_llvm.c](../src/ir/codegen_llvm.c), `LlvmValue` guarda o tipo
resolvido e a grafia do operando que está sendo emitido. É um auxiliar do
emissor textual, sem uma lista própria de operações ou passes de otimização.

## 3. Memória e valores SSA

`emit_function` reserva parâmetros e locais com `alloca` no bloco `entry`.
As leituras usam `load`, as atribuições usam `store`, e os parâmetros são
armazenados a partir de `%arg.N`. Os locais começam com os valores padrão
documentados, uma vez por chamada da função.

Cada resultado `%t.N` é definido uma única vez. Isso satisfaz SSA para os
valores de registrador da IR. A mesma posição de memória pode receber vários
`store`: o conteúdo da memória não é um registrador SSA.

`mem2reg` identifica posições locais elegíveis, substitui leituras/escritas
por valores SSA e insere `phi` onde os caminhos trazem valores diferentes.
Um agregado ou endereço usado de modo incompatível com a promoção pode
continuar em memória. Não se deve afirmar que todo `alloca` desaparece.

No exemplo [ssa.mac](../examples/ssa.mac), `i` e `soma` mudam no laço,
e `soma` também recebe valores em dois ramos. Compare os artefatos descritos
em [otimizacoes.md](otimizacoes.md), observando os predecessores de cada
`phi` e quais `load`/`store` desapareceram.

## 4. Blocos básicos, líderes e CFG

Um bloco básico é uma sequência de instruções com entrada no começo e um
terminador no fim. No LLVM, rótulos como `if.then.N` e `loop.cond.N`
identificam esses blocos. A primeira instrução de uma função ou de um destino
de salto pode ser relacionada à noção de líder apresentada para construir
blocos a partir de uma sequência de instruções.

O CFG é o grafo desses blocos, com arestas dadas pelos saltos. `emit_if`
gera teste, ramos e, quando necessário, encontro. Se todos os ramos retornam,
o gerador não cria uma continuação alcançável. `emit_while` liga condição,
corpo e saída; `emit_for` acrescenta inicialização e iteração.

`ret`, `br` e `unreachable` são terminadores. Depois de `respost`,
`gen->terminated` impede a emissão de outro comando no mesmo caminho.
Uma guarda de runtime possui uma saída de erro que chama uma função
`noreturn` e termina com `unreachable`, e uma saída que continua o cálculo.

## 5. Lógica e `phi`

Em `E`, o segundo operando só é avaliado se o primeiro for verdadeiro.
Em `OU`, só se o primeiro for falso. `emit_binary` gera blocos para o
operando direito e o encontro. No encontro, um `phi i1` escolhe o resultado
de acordo com o predecessor pelo qual a execução chegou.

Exemplo esquemático, para um `E` sem outras expressões que abram blocos:

```llvm
  br i1 %esquerda, label %direita, label %encontro
direita:
  ; produz %valorDireito
  br label %encontro
encontro:
  %resultado = phi i1 [ false, %origem ], [ %valorDireito, %direita ]
```

Se o cálculo de um operando abrir outros blocos, como uma guarda de divisão,
o predecessor efetivo deixa de ser necessariamente o rótulo inicial.
Por isso o emissor salva `gen->current_block` após emitir cada operando.
`phi` não executa ambos os cálculos e não é uma operação de comparação.

`XOR` avalia os dois operandos, da esquerda para a direita, e usa `xor i1`.
O teste [04_logica.mac](../tests/execucao/04_logica.mac) usa uma função que
altera uma global para tornar essa diferença observável.

## 6. Tipos, conversões, chamadas e retornos

| Fonte | Representação/ação LLVM | Motivo |
|---|---|---|
| `inteira` | `i32`, operações inteiras e `icmp` | Inteiro assinado de 32 bits |
| `flut` | `float`, operações reais e `fcmp` | Precisão binary32 |
| `duplocarpado` | `double`, operações reais e `fcmp` | Precisão binary64 |
| `bool` | `i1` | Resultado lógico |
| Promoção inteira → real | `sitofp` | Converte valor assinado, não reinterpreta bits |
| Promoção flut → duplocarpado | `fpext` | Aumenta precisão do formato real |
| Retorno comum `vazio` | `ret void` | Ausência de valor |
| `principal` | `i32 @zl_principal()` | Contrato de entrada com o runtime |

`convert_value` recebe o tipo esperado pelo destino, parâmetro ou retorno.
O tipo original da expressão continua na AST. `fptrunc` reduziria precisão,
mas essa conversão não é autorizada pela linguagem atual. Os operadores
binários não promovem automaticamente seus operandos.

`emit_call` avalia os argumentos antes de escrever `call`, preservando sua
ordem. Cada argumento é convertido para o parâmetro correspondente.
Uma função de usuário recebe nome `@mac.nome`; a entrada é `@zl_principal`.
Funções podem chamar funções posteriores e a si mesmas porque a semântica
já pré-coletou as assinaturas. A execução ainda usa os retornos e a pilha
normais das chamadas.

## 7. Vetores, registros e texto

`emit_address` calcula endereços de identificadores, elementos de vetor e
campos sem carregar seus valores. Na atribuição, calcula esse endereço e
suas guardas antes de avaliar o lado direito, conforme a decisão de ordem.

`getelementptr` calcula um endereço usando tipos e índices. Ele não lê o
elemento nem calcula manualmente um offset em bytes. Para vetor, a guarda
confere `0 <= índice < N` antes do GEP. Para campo, `symbol->field_index`
é a posição textual no registro, diferente do ID global do símbolo.

Registros são nominais e passam por valor. Um campo de registro temporário,
como `copia(pessoa).idade`, usa `extractvalue`, pois o resultado de chamada
é um agregado como valor. Um campo de variável usa endereço e `load`.
Ciclos de registro por valor são rejeitados pela semântica porque não
produziriam um tamanho finito.

Texto literal vira constante global terminada em zero. `palavra` é `ptr`,
e igualdade textual chama `zl_texto_igual` para comparar conteúdo.
Comparar os ponteiros verificaria identidade de armazenamento, que não é
a igualdade definida pela linguagem.

## 8. Runtime, erros e portabilidade

[zl_runtime.h](../runtime/zl_runtime.h) declara a fronteira fixa com C.
A IR usa funções `zl_*`, enquanto o runtime implementa impressão, leitura
e operações matemáticas e mantém as chamadas variádicas da biblioteca C.
Para impressão booleana, o gerador amplia `i1` para `i32`, evitando depender
da ABI C de `_Bool`.

As guardas evitam executar divisão por zero, divisão inteira que exceda a
faixa e indexação fora dos limites. O runtime também verifica domínio
matemático e entrada numérica. Os diagnósticos incluem a linha da fonte.
`zl_erro_execucao` termina o processo; `unreachable` expressa que o controle
não volta dessa chamada.

O `.ll` gerado não fixa triple, datalayout ou placa. GEP deixa a disposição
de memória para o LLVM no alvo escolhido ao compilar. O runtime tem `main`,
que chama `zl_principal`; assim a linguagem não incorpora a assinatura de
entrada de uma plataforma. O módulo do runtime compilado pode conter os
detalhes do hospedeiro, e deve corresponder ao ambiente usado na ligação.
Portabilidade do módulo não significa que um binário nativo rode em qualquer
placa. A escolha da placa continua no PjBL 4.

## 9. Demonstrar e interpretar testes

```sh
make test
./build/debug/macaronica examples/ssa.mac -o build/ssa.ll
llvm-as build/ssa.ll -o build/ssa.bc
opt -passes=verify build/ssa.ll -disable-output
opt -S -passes=mem2reg build/ssa.ll -o build/ssa_mem2reg.ll
opt -S -O2 build/ssa.ll -o build/ssa_O2.ll
```

No Windows, use `mingw32-make` e o executável `macaronica.exe`.
Consulte [decisoes_backend.md](decisoes_backend.md) e o README para a
compilação/ligação do runtime. Use as mesmas entradas ao comparar original,
`mem2reg` e `-O2`.

`llvm-as` lê a IR textual e produz bitcode, detectando problemas de sintaxe
e consistência. `opt -passes=verify` verifica invariantes da IR, como tipos,
dominância e entradas dos `phi`. Eles não provam que o resultado implementa
a semântica pretendida. `lli` executa o módulo ligado no hospedeiro;
Clang gera o executável nativo. A comparação com resultados esperados testa
o comportamento observável das entradas cobertas.

Em `-O2`, relacione mudanças observadas com propagação/folding de constantes,
eliminação de código morto ou inalcançável, cópias, CSE e strength reduction
quando houver evidência. Não atribua uma mudança a um passe específico sem
um relatório. Efeitos colaterais, aliases e ponto flutuante limitam as
transformações; `fast-math` não está habilitado. Liveness e análise de fluxo
explicam por que definições precisam chegar a usos, e análises iterativas
podem buscar ponto fixo; o grupo não implementou essas análises no PjBL 3.

## 10. Perguntas de arguição preparadas

| Pergunta | Resposta esperada no nosso código |
|---|---|
| Por que LLVM IR? | Oferece operações tipadas, verificador, passes e execução/compilação; o grupo implementa o lowering da AST e preserva sua semântica. |
| Por que manter a tabela semântica viva? | A AST contém IDs que apontam para tipos e símbolos consultados pelo emissor. |
| A IR inicial já é SSA? | Cada resultado de instrução tem uma definição; variáveis mutáveis ficam em memória. `mem2reg` promove as posições elegíveis. |
| Por que surge um `phi`? | Um bloco recebe valores de predecessores diferentes; o valor selecionado depende da aresta de entrada. |
| Por que `alloca` fica em `entry`? | Reserva locais uma vez por chamada e facilita a promoção por `mem2reg`. |
| O `phi` executa o lado direito de `E`? | Não. Os saltos decidem executar o lado direito; `phi` reúne o valor na chegada. |
| O que acontece depois de `ret`? | Aquele bloco terminou. O emissor evita anexar comandos ao caminho encerrado. |
| GEP faz checagem de índice? | Não. O compilador emite comparações e saltos de erro antes de calcular o endereço. |
| Por que `sitofp` e não `bitcast`? | Precisamos converter o valor numérico, não interpretar os mesmos bits de outro modo. |
| Por que não usar `fptrunc`? | Reduzir precisão não é uma conversão autorizada pela Macaronica atual. |
| `/` e `//` podem compartilhar uma instrução? | Não: `/` usa reais e `fdiv`; `//` usa `sdiv i32` e trunca para zero. |
| Por que não emitir `nsw`? | Overflow de soma/subtração/multiplicação é modular; `nsw` permitiria pressupor ausência de overflow assinado. |
| Por que `-2147483648 // -1` tem guarda? | O resultado positivo não cabe em `i32`; executar `sdiv` nesse caso violaria seu contrato. |
| Igualdade de palavra usa `icmp ptr`? | Não. `zl_texto_igual` compara conteúdo; dois endereços distintos podem conter o mesmo texto. |
| Registro temporário precisa de GEP? | Seu campo pode ser extraído por `extractvalue`; GEP é usado quando temos um endereço de armazenamento. |
| `lli` emula a placa futura? | Não. Ele executa no hospedeiro atual; a toolchain de placa pertence ao PjBL 4. |
| Um teste que passa prova correção? | Não. Ele é evidência para entradas cobertas, complementada pela revisão e pelas verificações estruturais. |

## Revisão de explicabilidade

O emissor concentra a emissão textual em `write_ir` e separa tipos, valores,
endereços, chamadas e estruturas de controle. Essa decomposição permite
explicar um recurso por vez sem introduzir uma IR própria ou um framework de
passes. A complexidade adicional dos blocos de guarda e dos predecessores de
curto-circuito é necessária para manter a semântica e uma IR válida.

Antes da apresentação, cada integrante deve acompanhar uma atribuição,
uma chamada com conversão, um laço com `phi` após `mem2reg`, um acesso a
registro temporário e uma falha de índice, do programa fonte até o resultado.
A revisão pedagógica não encontrou defeito concreto nesses mecanismos.
A bateria integrada passou 119 testes no Windows com LLVM/Clang 22.1.8,
incluindo verificação de IR, execução `lli` com runtime DLL, execução nativa,
erros de runtime e equivalência após `mem2reg`/`-O2`. O modo POSIX com runtime
ligado em bitcode está automatizado, mas não foi executado em Linux nesta
validação. Os testes são evidência dos casos cobertos, não prova formal.
