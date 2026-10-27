# Decisões de projeto da linguagem Macaronica

**Versão da especificação:** 0.1.0  
**Estado:** decisões provisórias para a primeira implementação do front-end  
**Fonte normativa:** seção 4.2 (Macaronica) e seção 4.3 do enunciado `Zolang_Macaronica.pdf`

Este documento separa o que é dito pela fonte do que foi decidido pelo projeto. Os exemplos em C da calculadora orientam apenas a organização do programa; eles não definem tokens, sintaxe ou semântica da Macaronica. Vocabulário apresentado para outras linguagens no mesmo material não faz parte desta especificação.

## D-001 - Arquivo de entrada e codificação

1. **Regra oficial:** a fonte exige um front-end em C, mas não define extensão nem codificação do arquivo de entrada.
2. **Lacuna:** é necessário estabelecer como a ferramenta recebe um programa e como representa `NÃO`.
3. **Decisão do projeto:** a ferramenta recebe um caminho terminado em `.mac`. O conteúdo é UTF-8. A extensão é uma convenção da ferramenta, não parte da gramática. Palavras-chave são sensíveis a maiúsculas e acentos: `NÃO` é aceito e `NAO` não é seu sinônimo.
4. **Impacto:** o `main` valida a extensão e a abertura; o lexer decodifica UTF-8 e mantém linha e coluna em caracteres. Parser, semântica e IR não dependem do nome do arquivo.

## D-002 - Vocabulário ausente do alfabeto resumido

1. **Regra oficial:** o alfabeto resumido lista `[ ] { } ( )`, letras, dígitos, `+ - * / ; ^ < > = !`. As estruturas oficiais também usam `@` e `,`; a tabela usa `//`, `==` e `NÃO`; o tipo `palavra` pressupõe algum literal textual. A lista de reservadas inclui `func`, mas as estruturas usam `registro`, `outrafuncao`, `vet` e `edai`.
2. **Lacuna:** o resumo do alfabeto e das palavras reservadas não comporta todos os exemplos normativos da própria Macaronica.
3. **Decisão do projeto:** o lexer reconhece todos os lexemas exigidos pelas estruturas de Macaronica. `registro`, `outrafuncao`, `vet`, `edai` e `raiz` são reservadas. `func` também é token reservado por constar na lista oficial, mas não inicia construção alguma na versão 0.1.0; seu uso produz diagnóstico sintático específico. `@`, `,`, aspas e barra invertida (nos escapes definidos em D-004) são extensões explícitas do alfabeto operacional.
4. **Impacto:** a tabela de tokens deve ser a de `gramatica.md`, e não somente o alfabeto resumido do PDF. O parser nunca interpreta uma reservada como identificador.

## D-003 - Identificadores, espaços e caixa

1. **Regra oficial:** o alfabeto contém `a..z`, `A..Z` e `0..9`, sem formalizar identificadores ou espaços.
2. **Lacuna:** início de identificador, separadores e sensibilidade a caixa não são definidos.
3. **Decisão do projeto:** identificadores seguem `[A-Za-z][A-Za-z0-9]*`. Sublinhado não é aceito nesta versão. Espaço, tabulação, CR e LF separam tokens e são ignorados, mas atualizam a localização. Todos os lexemas são sensíveis a caixa.
4. **Impacto:** `resultado1` é identificador; `1resultado` é número seguido de identificador; `resultado_1` contém caractere inválido. `VER`, `FAL`, `OU`, `E`, `NÃO` e `XOR` não podem ser nomes.

## D-004 - Literais

1. **Regra oficial:** `VER` e `FAL` são booleanos. Os exemplos usam inteiros decimais. O tipo `palavra` existe, mas a fonte não formaliza seu literal. `flut` e `duplocarpado` existem, mas a fonte não diferencia seus literais.
2. **Lacuna:** faltam as expressões regulares de números e texto e uma regra para distinguir as duas precisões reais.
3. **Decisão do projeto:** são reconhecidos:
   - inteiro decimal: `[0-9]+`, com tipo `inteira`;
   - real decimal: `[0-9]+\.[0-9]+`, sem expoente, com tipo `flut`;
   - texto: sequência entre `"`, sem quebra de linha, com escapes `\\`, `\"`, `\n`, `\r` e `\t`, com tipo `palavra`;
   - `VER` e `FAL`, com tipo `bool`.

   O valor de um literal `palavra` chega à AST já decodificado: por exemplo, os dois caracteres `\` e `n` da origem tornam-se uma quebra de linha no valor semântico. O nó da AST preserva seu `SourceSpan`, apontando para toda a grafia original do literal, incluindo as aspas, para que diagnósticos continuem referenciando o arquivo-fonte corretamente.

   Não há literal próprio de `duplocarpado` em 0.1.0. Um valor desse tipo pode vir de parâmetro, variável ou chamada de retorno `duplocarpado`. Promocoes seguem D-012; acrescentar sufixo exige nova decisao. Literais também precisam caber na representação usada pelo compilador: inteiros acima de `LLONG_MAX` e reais cuja conversão ultrapassa `DBL_MAX` são rejeitados.
4. **Impacto:** o lexer emite tokens distintos para inteiro, real, palavra e booleano. Texto sem fechamento, escape desconhecido e literal numérico fora da faixa são erros léxicos. Lexer e parser devem definir um único responsável pela decodificação, sem decodificar duas vezes; ao construir `AST_LITERAL_EXPR` com `AST_LITERAL_STRING`, o conteúdo armazenado é o valor decodificado e o `SourceSpan` é o da origem. A IR futura recebe o valor decodificado e pode reutilizar o `SourceSpan` para diagnóstico. A ausencia de literal de duplocarpado e preservada; promocoes seguem D-012.

## D-005 - Comentários e conflito com `//`

1. **Regra oficial:** a fonte não define comentários; `//` significa divisão inteira.
2. **Lacuna:** nenhuma sintaxe de comentário Macaronica foi especificada.
3. **Decisão do projeto:** arquivos `.mac` não possuem comentários na versão 0.1.0. `//` é sempre um operador. Os comentários `// debug - remover` solicitados para a implementação são comentários do código C e não devem ser reconhecidos dentro de programas Macaronica.
4. **Impacto:** o lexer aplica maior casamento para reconhecer `//` antes de `/`. Não existe descarte de comentários.

## D-006 - Acesso a vetor

1. **Regra oficial:** a declaração é `tipo vet{tamanho} nome, ... ;`; a fonte não mostra acesso a elemento.
2. **Lacuna:** a sintaxe de indexação é uma ambiguidade obrigatória da seção 4.3.
3. **Decisão do projeto:** acesso e atribuição a elemento usam `nome{expressão}`. O tamanho da declaração deve ser um literal `inteira` maior que zero. O índice deve ter tipo `inteira`; índices começam em zero. O nome de um vetor inteiro não é um valor legível nem um destino atribuível: é proibido ler, copiar ou atribuir o vetor como unidade. Somente `nome{indice}` produz um elemento que pode ser usado como `rvalue` ou `lvalue`.
4. **Impacto:** o lexer já possui `{` e `}`; o parser diferencia declaração pelo token `vet` e indexação após identificador. A semântica rejeita indexar não vetor, usar um vetor inteiro em expressão ou como argumento/retorno, atribuir a ele e copiá-lo. Ela também verifica os limites quando consegue avaliar estaticamente a expressão inteira usada como índice. A AST/semântica marca apenas o acesso a elemento com capacidades de valor e destino. A IR futura representa a indexação explicitamente e não precisa de operação de cópia de vetor nesta versão.

## D-007 - Chamada de função

1. **Regra oficial:** a fonte define declaração, mas não define formalmente a chamada; o exemplo usa `quadrado(numero)`.
2. **Lacuna:** a seção 4.3 manda documentar a forma natural.
3. **Decisão do projeto:** chamadas usam `nome(argumentos)`, com zero ou mais expressões separadas por vírgula. Uma chamada que retorna valor pode compor uma expressão. Uma chamada de função `vazio` pode ser um comando terminado por `;` e não pode compor expressão de valor.
4. **Impacto:** o parser reconhece chamada após identificador. A semântica verifica existência, quantidade e tipo exato dos argumentos e o uso do retorno. A IR futura usa nó de chamada com lista ordenada de argumentos.

## D-008 - Retorno e `vazio`

1. **Regra oficial:** `vazio` existe apenas como tipo de retorno; a forma mostrada é `respost valor;`. A fonte não define o retorno de função `vazio`.
2. **Lacuna:** omitir `respost` ou usar `respost;` são alternativas igualmente plausíveis.
3. **Decisão do projeto:** em função `vazio`, `respost;` é permitido e opcional, enquanto `respost expressão;` é proibido. Em função não vazia, todo caminho possível de execução deve chegar a `respost expressão;`, e a expressão deve ter exatamente o tipo declarado. Uma cadeia `cond` só garante retorno quando possui um `casocontrario` final e todos os ramos garantem retorno. `durante` e `repete` não garantem retorno, pois podem executar zero vezes.
4. **Impacto:** o parser aceita `respost` com ou sem expressão. A semântica usa o tipo da função corrente, rejeita retorno fora de função e produz `E-SEM-032` quando uma função não vazia deixa algum caminho sem valor. A IR futura terá `RETURN` com operando opcional e receberá corpos cuja cobertura de retorno já foi validada.

## D-009 - Condições de controle

1. **Regra oficial:** a fonte restringe condição a `VAR COMPARADOR VAR`, `VAR COMPARADOR NUM` ou `NUM COMPARADOR NUM`.
2. **Lacuna:** booleanos e operadores lógicos existem, mas a fonte não os integra à regra de condição.
3. **Decisão do projeto:** fica aprovada uma extensão explícita sobre a regra oficial restritiva. As condições de `cond`, `casocontrario cond`, `durante` e `repete` aceitam sintaticamente qualquer `expressao`, sem importar construções de outras linguagens. A análise semântica exige que o resultado da expressão seja `bool`. Assim, são válidos literais e variáveis booleanas, `NÃO`, chamadas com retorno `bool`, comparações e combinações com quantos operadores `E`, `OU` e `XOR` forem necessários. A ordem de avaliação sintática segue a precedência e a associatividade de D-010.
4. **Impacto:** o lexer não ganha tokens. O parser reutiliza a produção `expressao` em todas as estruturas de controle. A semântica verifica a expressão normalmente e rejeita a condição quando seu tipo final não é `bool`. A AST mantém a expressão completa como condição, e a IR futura recebe um valor booleano já validado. Esta decisão define o agrupamento sintático, mas ainda não escolhe entre curto-circuito e avaliação ansiosa para `E` e `OU`; isso deve ser resolvido antes do lowering para IR.

## D-010 - Precedência e associatividade

1. **Regra oficial:** a fonte enumera operadores, mas não dá precedência nem associatividade.
2. **Lacuna:** expressões como `a + b * c`, `a ^ b ^ c` e `NÃO a == b` são ambíguas.
3. **Decisão do projeto:** adota-se a tabela revisável de `gramatica.md`. Do maior para o menor nível: chamada/indexação/`raiz`; unários `+` e `-`; `^`; `*`, `/`, `//`; `+`, `-`; comparadores; `NÃO`; `E`; `XOR`; `OU`. `^` e `NÃO` associam à direita; aritméticos e lógicos binários associam à esquerda; comparadores não encadeiam.
4. **Impacto:** o parser deve materializar essa hierarquia. A semântica não tenta reinterpretar a AST. A IR recebe a ordem já resolvida pelo parser.

## D-011 - Semântica dos operadores

1. **Regra oficial:** `//` é divisão inteira, `^` é potência, `raiz` é raiz quadrada, `XOR` é lógico e `!` é comparador de diferença. A fonte não especifica domínios e tipos resultantes.
2. **Lacuna:** sem essas regras, a verificação de tipos não é determinística.
3. **Decisão do projeto:**
   - `+`, `-` e `*`: dois operandos do mesmo tipo numérico; resultado do mesmo tipo;
   - `/`: dois operandos do mesmo tipo `flut` ou `duplocarpado`; resultado do mesmo tipo;
   - `//`: dois operandos `inteira`; quociente truncado em direção a zero e resultado `inteira`;
   - `^`: base e expoente do mesmo tipo numérico; resultado desse tipo; associa à direita;
   - `raiz(expr)`: operando `flut` ou `duplocarpado`; resultado do mesmo tipo;
   - `<` e `>`: operandos do mesmo tipo numérico; resultado `bool`;
   - `==` e `!`: operandos escalares de mesmo tipo; resultado `bool`; `!` é binário e significa diferente;
   - `NÃO`: um `bool`; resultado `bool`;
   - `E`, `OU` e `XOR`: dois `bool`; resultado `bool`.

   Divisão por zero, raiz de valor negativo e demais falhas dependentes de valor não constante são de execução e ficam fora do front-end. Se todos os operandos forem constantes, a semântica pode diagnosticá-las sem ser obrigada a executar o programa.
4. **Impacto:** o parser distingue `!` binário de `NÃO` unário. A semântica usa a matriz acima; a IR futura deve manter operações diferentes para `/` e `//`.

## D-012 - Conversões numéricas

1. **Regra oficial:** não há regra de conversão entre `inteira`, `flut` e `duplocarpado`.
2. **Lacuna:** não se sabe se atribuições, argumentos, retornos ou operadores promovem tipos.
3. **Decisao do projeto (revisada com aprovacao do usuario):** atribuicoes, argumentos e retornos aceitam tipos identicos e as promocoes `inteira -> flut`, `inteira -> duplocarpado` e `flut -> duplocarpado`. Conversoes inversas, casts explicitos e promocao entre operandos de operadores nao sao aceitos. Vetores e registros nao ganham conversao estrutural; elementos numericos de vetores seguem a regra escalar. Inteiros grandes podem sofrer arredondamento ao serem convertidos para ponto flutuante.
4. **Impacto:** a expressao mantem seu tipo original na AST. O destino da atribuicao, o parametro da assinatura ou o retorno declarado fornece o tipo esperado. O futuro gerador LLVM devera comparar esses tipos e emitir `sitofp` para inteiro -> ponto flutuante e `fpext` para flut -> duplocarpado. Inteira -> duplocarpado deve ser direta, sem passar por flut. Este repositorio ainda implementa somente o front-end; nao ha execucao nem geracao LLVM para validar conversoes em tempo de execucao.

## D-013 - Registros

1. **Regra oficial:** `registro nome @ [ tipo nome; ... ]` declara um registro, mas não há sintaxe de acesso a campo.
2. **Lacuna:** a fonte não explica como instanciar ou acessar campos.
3. **Decisão do projeto:** o nome do registro passa a ser um tipo válido em declarações, parâmetros e retornos. Campos devem ter nomes únicos e tipo não `vazio`. Como não há operador de acesso oficial, acesso a campos não é aceito em 0.1.0.
4. **Impacto:** lexer não ganha pontuação de campo. Parser mantém declaração de registro e tipos nomeados. Semântica possui namespace de tipos e valida os campos, mas qualquer tentativa de acesso produz erro léxico/sintático.

## D-014 - Escopos, declarações e funções

1. **Regra oficial:** o requisito técnico exige ao menos escopo global e escopo de função, declaração antes do uso, tipos e assinatura de função.
2. **Lacuna:** a fonte não define sombreamento, chamadas antecipadas, visibilidade temporal de globais ou escopos de blocos de controle.
3. **Decisão do projeto:** há escopo global e um escopo local por função; blocos `cond`, `repete` e `durante` não criam novo escopo em 0.1.0. Duplicatas no mesmo escopo são erro. Um nome local pode sombrear variável global. Os nomes de todos os registros e as assinaturas de todas as funções são pré-coletados antes da análise dos corpos, permitindo tipos de registro posteriores, recursão e chamadas a funções declaradas depois. Variáveis globais, porém, obedecem à ordem textual: o corpo de uma função enxerga somente as globais declaradas antes dessa função no arquivo. Assim, uma função anterior não enxerga uma global declarada depois. Variáveis locais também precisam ser declaradas anteriormente no fluxo do próprio escopo.
4. **Impacto:** a semântica pré-coleta tipos de registro e assinaturas de função, mas não antecipa símbolos de variáveis globais. Ao analisar cada corpo, deve respeitar o ponto textual da declaração da função, mesmo que os corpos sejam verificados em uma passagem posterior. Tipos, funções e valores usam namespaces distintos. O parser preserva a ordem das declarações globais e dos comandos; a IR futura recebe referências já resolvidas, sem alterar essas regras de visibilidade.

## D-015 - Unidade de programa e `principal`

1. **Regra oficial:** a fonte distingue função não principal e `tipo principal(...) @ [ ... ]`.
2. **Lacuna:** não define quantidade, ordem ou possibilidade de chamar `principal`.
3. **Decisão do projeto:** cada arquivo contém exatamente uma função `principal`. Registros, funções, declarações globais e `principal` podem aparecer em qualquer ordem sintática; essa liberdade não antecipa a visibilidade de variáveis globais, que continua obedecendo à ordem textual definida em D-014. `principal` não pode ser chamada como função comum. Seu tipo de retorno segue `tipo_retorno` e respeita D-008.
4. **Impacto:** ausência ou duplicidade de `principal` é erro semântico global. A IR futura marca esse símbolo como ponto de entrada.

## D-016 - Comandos de repetição

1. **Regra oficial:** `repete ( var , condição , iteração ) edai [ ... ]` e `durante ( condição ) edai [ ... ]` são formas oficiais; `var` e `iteração` não são detalhados.
2. **Lacuna:** não existem `++`/`--` na tabela Macaronica e a forma das cláusulas do `repete` é indefinida.
3. **Decisão do projeto:** inicialização e iteração de `repete` são atribuições sem `;`, por exemplo `i receba 0` e `i receba i + 1`. A condição usa D-009. Declaração dentro do cabeçalho e cláusula vazia não são aceitas.
4. **Impacto:** o parser reutiliza `atribuicao_sem_ponto_virgula`; a semântica verifica ambos os alvos e expressões normalmente.

## D-017 - Entrada e saída

1. **Regra oficial:** a seção de Macaronica não define tokens nem estruturas de leitura ou impressão, embora a seção geral de testes peça uso de entrada/saída para a linguagem escolhida.
2. **Lacuna:** o requisito de teste e a especificação de Macaronica são incompatíveis.
3. **Decisão do projeto:** não importar comandos de outra linguagem. Entrada/saída ficam sem sintaxe e são rejeitadas em 0.1.0 até confirmação do professor ou nova decisão do grupo.
4. **Impacto:** lexer e parser não reconhecem comandos de I/O. O risco de não cumprir literalmente um caso obrigatório deve ser levado ao professor e registrado na entrega.

## D-018 - Diagnósticos e recuperação

1. **Regra oficial:** erro léxico deve indicar ao menos linha; erro sintático, linha e quando possível token; o programa não pode travar. A rubrica valoriza coluna e múltiplos erros.
2. **Lacuna:** formato e estratégia de recuperação não são definidos.
3. **Decisão do projeto:** todo diagnóstico tem `arquivo:linha:coluna`, fase, código estável e mensagem, por exemplo `algo.mac:3:8: lexico E-LEX-001: caractere '_' não pertence à linguagem`. O lexer avança ao menos um caractere em erro. O parser sincroniza em `;`, `]` ou início de declaração/controle. A semântica usa tipo de erro interno para evitar diagnósticos em cascata. Logs de depuração vão para `stderr`, começam por `[debug]` e as linhas C que os produzem recebem `// debug - remover`.
4. **Impacto:** cada token carrega posição inicial e final. As três fases acumulam erros quando puderem continuar com segurança e retornam estado de falha sem abortar o processo.

## D-019 - Instrumentação temporária de depuração

1. **Regra oficial:** a fonte exige analisadores léxico, sintático e semântico, mas não determina instrumentação ou formato de logs de depuração.
2. **Lacuna:** é preciso tornar observável a primeira etapa sem incorporar mensagens de debug à linguagem ou aos diagnósticos normativos.
3. **Decisão do projeto:** na Macaronica 0.1.0, a instrumentação temporária dos três analisadores fica habilitada por padrão. Cada `fprintf` temporário de depuração no código C deve ter, na própria linha, o comentário exato `// debug - remover`. As mensagens vão para `stderr` e começam com `[debug]`. Essa convenção pertence somente à implementação C: ela não cria comentários, tokens ou qualquer outra sintaxe em arquivos `.mac`.
4. **Impacto:** lexer, parser e semântica emitem rastros durante o pipeline normal; testes que validam a saída funcional devem separar ou filtrar `stderr`. Uma configuração de release pode desabilitar a instrumentação, sem mudar o resultado da análise. A remoção posterior pode localizar com segurança todas as linhas pela marca exata. AST e IR não representam esses logs.

## Questões pendentes para uma versão posterior

- Avaliar promocao entre operandos de operadores, fora do escopo de D-012.
- Definir literal próprio de `duplocarpado`.
- Definir acesso a campos de registro.
- Obter do professor uma sintaxe de entrada/saída para Macaronica ou uma dispensa explícita do caso de teste.
- Definir a ordem de avaliação dos operandos e se `E` e `OU` usam curto-circuito antes de implementar a IR.
