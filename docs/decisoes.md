# Decisões de projeto da linguagem Macaronica

**Versão da especificação:** 0.2.0 (PjBL 3)

**Estado:** contrato de linguagem para implementação e validação do PjBL 3

**Fonte normativa:** seção 4.2 (Macaronica) e seção 4.3 do enunciado `Zolang_Macaronica.pdf`

Este documento separa o que é dito pela fonte do que foi decidido pelo projeto. Os exemplos em C da calculadora orientam apenas a organização do programa; eles não definem tokens, sintaxe ou semântica da Macaronica. Vocabulário apresentado para outras linguagens no mesmo material não faz parte desta especificação.

As revisões para o PjBL 3 foram decididas pelo projeto para cumprir o escopo solicitado em 29/10/2026. São extensões ou restrições locais, sem atribuição ao PDF. O PDF citado não está presente no repositório; as regras oficiais abaixo são as já transcritas pela documentação do projeto. Este documento descreve o contrato a implementar, sem afirmar que todas as regras já passaram por validação executável. O mapeamento para LLVM e a ABI do runtime são detalhados em `decisoes_backend.md`.

## D-001 - Arquivo de entrada e codificação

1. **Regra oficial:** a fonte exige um front-end em C, mas não define extensão nem codificação do arquivo de entrada.
2. **Lacuna:** é necessário estabelecer como a ferramenta recebe um programa e como representa `NÃO`.
3. **Decisão do projeto:** a ferramenta recebe um caminho terminado em `.mac` ou `.mcn` (`.mcn` é a extensão dos casos de teste distribuídos pelo professor, aceita desde o PjBL 3/4). O conteúdo é UTF-8. A extensão é uma convenção da ferramenta, não parte da gramática. Palavras-chave são sensíveis a maiúsculas e acentos: `NÃO` é a única grafia da negação. `NAO` (sem til) e as grafias com til em outra caixa (`não`, `Não`, `NãO`...) geram `E-LEX-007`, que aponta a grafia certa; o lexer segue como se fosse `NÃO`, para não gerar erros sintáticos em cascata. `nao` e `Nao` continuam nomes válidos.
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
4. **Impacto:** `resultado1` é identificador; `1resultado` é número seguido de identificador; `resultado_1` contém caractere inválido. `VER`, `FAL`, `OU`, `E`, `NÃO` e `XOR` não podem ser nomes, nem `NAO` (`E-LEX-007`, D-001).

## D-004 - Literais

1. **Regra oficial:** `VER` e `FAL` são booleanos. Os exemplos usam inteiros decimais. O tipo `palavra` existe, mas a fonte não formaliza seu literal. `flut` e `duplocarpado` existem, mas a fonte não diferencia seus literais.
2. **Lacuna:** faltam as expressões regulares de números e texto e uma regra para distinguir as duas precisões reais.
3. **Decisão do projeto:** são reconhecidos:
   - inteiro decimal: `[0-9]+`, com tipo `inteira`;
   - real decimal: `[0-9]+\.[0-9]+`, sem expoente, com tipo `flut`;
   - texto: sequência entre `"`, sem quebra de linha, com escapes `\\`, `\"`, `\n`, `\r` e `\t`, com tipo `palavra`;
   - `VER` e `FAL`, com tipo `bool`.

   O valor de um literal `palavra` chega à AST já decodificado: por exemplo, os dois caracteres `\` e `n` da origem tornam-se uma quebra de linha no valor semântico. O nó da AST preserva seu `SourceSpan`, apontando para toda a grafia original do literal, incluindo as aspas, para que diagnósticos continuem referenciando o arquivo-fonte corretamente.

   Não há literal próprio de `duplocarpado`. Um valor desse tipo pode vir de promoção, parâmetro, variável ou chamada de retorno `duplocarpado`. Promoções seguem D-012; acrescentar sufixo exige nova decisão. A faixa semântica dos literais numéricos passa a seguir D-021; o armazenamento temporário mais largo do compilador não aumenta a faixa da linguagem.
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
4. **Impacto:** o parser reconhece chamada após identificador. A semântica verifica existência, quantidade e compatibilidade dos argumentos conforme D-012 e o uso do retorno. A IR usa nó de chamada com lista ordenada de argumentos e emite as promoções necessárias.

## D-008 - Retorno e `vazio`

1. **Regra oficial:** `vazio` existe apenas como tipo de retorno; a forma mostrada é `respost valor;`. A fonte não define o retorno de função `vazio`.
2. **Lacuna:** omitir `respost` ou usar `respost;` são alternativas igualmente plausíveis.
3. **Decisão do projeto:** em função `vazio`, `respost;` é permitido e opcional, enquanto `respost expressão;` é proibido. Em função não vazia, todo caminho possível de execução deve chegar a `respost expressão;`, e a expressão deve ter tipo compatível com o retorno declarado conforme D-012. Uma cadeia `cond` só garante retorno quando possui um `casocontrario` final e todos os ramos garantem retorno. `durante` e `repete` não garantem retorno, pois podem executar zero vezes. `respost` termina imediatamente a função corrente; nenhum comando posterior do mesmo caminho é executado. A regra de entrada `principal` é especificada em D-015.
4. **Impacto:** o parser aceita `respost` com ou sem expressão. A semântica usa o tipo da função corrente, rejeita retorno fora de função e produz `E-SEM-032` quando uma função não vazia deixa algum caminho sem valor. A IR emite `ret` com o valor convertido quando necessário; o final alcançável de uma função comum `vazio` emite `ret void`.

## D-009 - Condições de controle

1. **Regra oficial:** a fonte restringe condição a `VAR COMPARADOR VAR`, `VAR COMPARADOR NUM` ou `NUM COMPARADOR NUM`.
2. **Lacuna:** booleanos e operadores lógicos existem, mas a fonte não os integra à regra de condição.
3. **Decisão do projeto:** fica aprovada uma extensão explícita sobre a regra oficial restritiva. As condições de `cond`, `casocontrario cond`, `durante` e `repete` aceitam sintaticamente qualquer `expressao`, sem importar construções de outras linguagens. A análise semântica exige que o resultado da expressão seja `bool`. Assim, são válidos literais e variáveis booleanas, `NÃO`, chamadas com retorno `bool`, comparações e combinações com quantos operadores `E`, `OU` e `XOR` forem necessários. A ordem de avaliação sintática segue a precedência e a associatividade de D-010.
4. **Impacto:** o lexer não ganha tokens. O parser reutiliza a produção `expressao` em todas as estruturas de controle. A semântica verifica a expressão normalmente e rejeita a condição quando seu tipo final não é `bool`. A AST mantém a expressão completa como condição, e a IR recebe um valor booleano já validado. A ordem de avaliação e o curto-circuito são definidos em D-020.

## D-010 - Precedência e associatividade

1. **Regra oficial:** a fonte enumera operadores, mas não dá precedência nem associatividade.
2. **Lacuna:** expressões como `a + b * c`, `a ^ b ^ c` e `NÃO a == b` são ambíguas.
3. **Decisão do projeto:** adota-se a tabela revisável de `gramatica.md`. Do maior para o menor nível: chamada/indexação/`raiz`; unários `+` e `-`; `^`; `*`, `/`, `//`; `+`, `-`; comparadores; `NÃO`; `E`; `XOR`; `OU`. `^` e `NÃO` associam à direita; aritméticos e lógicos binários associam à esquerda; comparadores não encadeiam.
4. **Impacto:** o parser deve materializar essa hierarquia. A semântica não tenta reinterpretar a AST. A IR recebe a ordem já resolvida pelo parser.

## D-011 - Semântica dos operadores

1. **Regra oficial:** `//` é divisão inteira, `^` é potência, `raiz` é raiz quadrada, `XOR` é lógico e `!` é comparador de diferença. A fonte não especifica domínios e tipos resultantes.
2. **Lacuna:** sem essas regras, a verificação de tipos não é determinística.
3. **Decisão do projeto:**
   - `+`, `-` e `*`: dois operandos numéricos; a conta é feita no tipo comum (D-012) e o resultado tem esse tipo;
   - `/`: divisão real; dois operandos numéricos convertidos para o tipo comum, ou para `duplocarpado` se os dois forem `inteira`; resultado real;
   - `//`: dois operandos `inteira`; quociente truncado em direção a zero e resultado `inteira`; operando real é erro semântico;
   - `^`: base e expoente numéricos convertidos para o tipo comum; resultado desse tipo (`inteira ^ inteira` é `inteira`); associa à direita;
   - `raiz(expr)`: operando numérico; `flut` e `duplocarpado` mantêm o tipo e `inteira` é calculada em `duplocarpado`;
   - `<` e `>`: operandos numéricos comparados no tipo comum; resultado `bool`;
   - `==` e `!`: operandos numéricos (no tipo comum) ou escalares de mesmo tipo; resultado `bool`; `!` é binário e significa diferente;
   - `NÃO`: um `bool`; resultado `bool`;
   - `E`, `OU` e `XOR`: dois `bool`; resultado `bool`.

   Divisão por zero, raiz de valor negativo e demais falhas dependentes de valor não constante são de execução e ficam fora do front-end. Se todos os operandos forem constantes, a semântica pode diagnosticá-las sem ser obrigada a executar o programa.
4. **Impacto:** o parser distingue `!` binário de `NÃO` unário. A semântica usa a matriz acima; a IR futura deve manter operações diferentes para `/` e `//`.

## D-012 - Conversões numéricas

1. **Regra oficial:** não há regra de conversão entre `inteira`, `flut` e `duplocarpado`.
2. **Lacuna:** não se sabe se atribuições, argumentos, retornos ou operadores promovem tipos.
3. **Decisao do projeto (revisada com aprovacao do usuario):** atribuicoes, argumentos e retornos aceitam tipos identicos e as promocoes `inteira -> flut`, `inteira -> duplocarpado` e `flut -> duplocarpado`. Conversoes inversas e casts explicitos nao sao aceitos. **Revisao no PjBL 3** (enunciado, Secao 4.3): operadores binarios aceitam expressoes mistas; o tipo comum e o mais largo dos dois na ordem `inteira < flut < duplocarpado` e o operando de menor ordem e convertido para ele antes da conta (detalhes por operador em D-011 e em `decisoes_backend.md`). Vetores e registros nao ganham conversao estrutural; elementos numericos de vetores seguem a regra escalar. Inteiros grandes podem sofrer arredondamento ao serem convertidos para ponto flutuante.
4. **Impacto:** a expressão mantém seu tipo original na AST; num operador misto, a semântica grava o tipo comum em `semantic.conversion_type_id`. O destino da atribuição, o parâmetro da assinatura ou o retorno declarado fornece o tipo esperado. O gerador LLVM compara esses tipos e emite `sitofp` para inteiro -> ponto flutuante e `fpext` para flut -> duplocarpado. Inteira -> duplocarpado é direta, sem passar por flut. As conversões são explícitas nas instruções LLVM; isso não cria sintaxe de cast na fonte. Testes de execução devem verificar atribuição, argumento e retorno, inclusive arredondamento de `inteira -> flut`.

## D-013 - Registros

1. **Regra oficial:** `registro nome @ [ tipo nome; ... ]` declara um registro, mas não há sintaxe de acesso a campo.
2. **Lacuna:** a fonte não explica como instanciar ou acessar campos.
3. **Decisão do projeto (revisada no PjBL 3):** o nome do registro é um tipo nominal válido em declarações, parâmetros e retornos. Dois registros declarados com nomes diferentes são tipos diferentes, mesmo com campos iguais. Campos têm nomes únicos e tipo não `vazio`; sua ordem é a ordem textual da declaração. O projeto acrescenta o acesso `expressao.campo`, permitindo cadeias como `pessoa.endereco.numero` e acesso sobre elemento de vetor ou resultado de função. O campo pode ser lido quando a base é valor e atribuído quando a base é destino atribuível. Registros podem ser copiados, passados e retornados por valor, sem conversão estrutural. Ciclos de campos por valor, diretos ou indiretos, são proibidos porque produziriam objetos sem tamanho finito. Vetores de registros são permitidos; campos de vetor continuam fora da gramática.
4. **Impacto:** o lexer ganha o token `.` como extensão do projeto. O parser cria um nó de acesso com base e nome do campo; a semântica resolve o símbolo e tipo do campo, propaga a capacidade de destino da base e rejeita ciclos de registros. A IR usa o tipo nominal e a posição textual do campo para GEP ou extração de valor. Testes incluem campos aninhados, cópia, parâmetros/retornos, campo inexistente, base não registro e ciclos.

## D-014 - Escopos, declarações e funções

1. **Regra oficial:** o requisito técnico exige ao menos escopo global e escopo de função, declaração antes do uso, tipos e assinatura de função.
2. **Lacuna:** a fonte não define sombreamento, chamadas antecipadas, visibilidade temporal de globais ou escopos de blocos de controle.
3. **Decisão do projeto:** há escopo global e um escopo local por função; blocos `cond`, `repete` e `durante` não criam novo escopo em 0.1.0. Duplicatas no mesmo escopo são erro. Um nome local pode sombrear variável global. Os nomes de todos os registros e as assinaturas de todas as funções são pré-coletados antes da análise dos corpos, permitindo tipos de registro posteriores, recursão e chamadas a funções declaradas depois. Variáveis globais, porém, obedecem à ordem textual: o corpo de uma função enxerga somente as globais declaradas antes dessa função no arquivo. Assim, uma função anterior não enxerga uma global declarada depois. Variáveis locais também precisam ser declaradas anteriormente no fluxo do próprio escopo.
4. **Impacto:** a semântica pré-coleta tipos de registro e assinaturas de função, mas não antecipa símbolos de variáveis globais. Ao analisar cada corpo, deve respeitar o ponto textual da declaração da função, mesmo que os corpos sejam verificados em uma passagem posterior. Tipos, funções e valores usam namespaces distintos. O parser preserva a ordem das declarações globais e dos comandos; a IR futura recebe referências já resolvidas, sem alterar essas regras de visibilidade.

## D-015 - Unidade de programa e `principal`

1. **Regra oficial:** a fonte distingue função não principal e `tipo principal(...) @ [ ... ]`.
2. **Lacuna:** não define quantidade, ordem ou possibilidade de chamar `principal`.
3. **Decisão do projeto (revisada no PjBL 3):** cada arquivo contém exatamente uma função `principal`, sem parâmetros e com retorno `inteira` ou `vazio`. Essa restrição de assinatura é uma decisão do projeto para definir a entrada executável, sem inventar argumentos do programa. Registros, funções, declarações globais e `principal` podem aparecer em qualquer ordem sintática; a visibilidade de globais segue D-014. `principal` não pode ser chamada como função comum. D-008 continua valendo: `inteira principal()` deve retornar valor em todos os caminhos, enquanto `vazio principal()` pode chegar ao fim ou usar `respost;`.
4. **Impacto:** a semântica rejeita ausência, duplicidade, parâmetros e retorno incompatível de `principal`. O gerador normaliza ambas as formas para `define i32 @zl_principal()`: na forma inteira, `respost expressão;` fornece o resultado, reduzido aos 32 bits baixos (`trunc i64` para `i32`); na forma vazia, retorno ou fim fornece zero. O `main` do runtime recebe esse resultado e o devolve como status do processo, sujeito à representação de status do sistema hospedeiro. Testes conferem assinatura, retorno e status.

## D-016 - Comandos de repetição

1. **Regra oficial:** `repete ( var , condição , iteração ) edai [ ... ]` e `durante ( condição ) edai [ ... ]` são formas oficiais; `var` e `iteração` não são detalhados.
2. **Lacuna:** não existem `++`/`--` na tabela Macaronica e a forma das cláusulas do `repete` é indefinida.
3. **Decisão do projeto:** inicialização e iteração de `repete` são atribuições sem `;`, por exemplo `i receba 0` e `i receba i + 1`. A condição usa D-009. Declaração dentro do cabeçalho e cláusula vazia não são aceitas.
4. **Impacto:** o parser reutiliza `atribuicao_sem_ponto_virgula`; a semântica verifica ambos os alvos e expressões normalmente.

## D-017 - Entrada e saída

1. **Regra oficial:** a seção de Macaronica não define tokens nem estruturas de leitura ou impressão, embora a seção geral de testes peça uso de entrada/saída para a linguagem escolhida.
2. **Lacuna:** o requisito de teste e a especificação de Macaronica são incompatíveis.
3. **Decisão do projeto (revisada no PjBL 3):** a solicitação de implementar E/S via runtime é atendida por funções intrínsecas predeclaradas com a sintaxe de chamada comum. São uma extensão explícita da Macaronica do projeto e não palavras reservadas do PDF. As assinaturas são:

   | Nome na fonte | Parâmetros | Retorno |
   |---|---|---|
   | `imprimeInteira` | `inteira valor` | `vazio` |
   | `imprimeFlut` | `flut valor` | `vazio` |
   | `imprimeDuplocarpado` | `duplocarpado valor` | `vazio` |
   | `imprimePalavra` | `palavra valor` | `vazio` |
   | `imprimeBool` | `bool valor` | `vazio` |
   | `imprimeNl` | nenhum | `vazio` |
   | `leInteira` | nenhum | `inteira` |
   | `leFlut` | nenhum | `flut` |
   | `leDuplocarpado` | nenhum | `duplocarpado` |

   Essas assinaturas seguem as conversões de D-012 e ocupam o namespace global de funções; uma função do usuário não pode redefini-las. A saída não acrescenta espaço nem nova linha automaticamente: `imprimeNl()` emite uma nova linha. Inteiros usam decimal assinado, `flut` nove dígitos significativos, `duplocarpado` dezessete, `bool` usa `VER`/`FAL` e `palavra` usa UTF-8. As leituras consomem um número de `stdin`, com espaços separando entradas e ponto decimal para reais; EOF, formato inválido ou valor fora da faixa terminam com erro de execução. Não há leitura de texto nesta versão.
4. **Impacto:** lexer e parser reutilizam identificadores e chamadas. A semântica predeclara as intrínsecas e verifica seus argumentos/retorno; a IR as identifica pelos símbolos, sem depender de chamadas variádicas C. O runtime POSIX separado implementa a ABI `zl_*`; esses nomes com sublinhado pertencem à IR/C e não à fonte Macaronica. A correspondência exata de símbolos é documentada em `decisoes_backend.md`. Testes usam entrada conhecida e comparam a saída integral.

## D-018 - Diagnósticos e recuperação

1. **Regra oficial:** erro léxico deve indicar ao menos linha; erro sintático, linha e quando possível token; o programa não pode travar. A rubrica valoriza coluna e múltiplos erros.
2. **Lacuna:** formato e estratégia de recuperação não são definidos.
3. **Decisão do projeto:** todo diagnóstico tem `arquivo:linha:coluna`, fase, código estável e mensagem, por exemplo `algo.mac:3:8: lexico E-LEX-001: caractere '_' não pertence à linguagem`. O lexer avança ao menos um caractere em erro. A meta é um diagnóstico por erro real; dois erros em comandos distintos geram dois diagnósticos. O parser usa modo pânico com sincronização. O primeiro erro de um comando liga o modo pânico, e os erros seguintes do mesmo comando, que são consequência dele, não são relatados. O parser descarta tokens até um ponto seguro e desliga o modo pânico. Pontos seguros: depois de `;` ou de um bloco `[ ... ]`, antes de `]`, de uma palavra que só inicia comando ou declaração, ou de um identificador que abre linha com cara de comando. Um cabeçalho entre parênteses com erro é pulado até o `)`, e o corpo ainda é analisado. O saldo de colchetes do arquivo (`[` menos `]`) decide entre supor um `[` esquecido, descartar um `[` solto ou fechar um ramo antes de `casocontrario`. A semântica usa tipo de erro interno para evitar diagnósticos em cascata. Os testes conferem a quantidade exata de diagnósticos (coluna `erros` de `tests/manifest.tsv`). Logs de depuração vão para `stderr`, começam por `[debug]` e as linhas C que os produzem recebem `// debug - remover`.
4. **Impacto:** cada token carrega posição inicial e final. As três fases acumulam erros quando puderem continuar com segurança e retornam estado de falha sem abortar o processo.

## D-019 - Instrumentação temporária de depuração

1. **Regra oficial:** a fonte exige analisadores léxico, sintático e semântico, mas não determina instrumentação ou formato de logs de depuração.
2. **Lacuna:** é preciso tornar observável a primeira etapa sem incorporar mensagens de debug à linguagem ou aos diagnósticos normativos.
3. **Decisão do projeto:** na Macaronica 0.1.0, a instrumentação temporária dos três analisadores fica habilitada por padrão. Cada `fprintf` temporário de depuração no código C deve ter, na própria linha, o comentário exato `// debug - remover`. As mensagens vão para `stderr` e começam com `[debug]`. Essa convenção pertence somente à implementação C: ela não cria comentários, tokens ou qualquer outra sintaxe em arquivos `.mac`.
4. **Impacto:** lexer, parser e semântica emitem rastros durante o pipeline normal; testes que validam a saída funcional devem separar ou filtrar `stderr`. Uma configuração de release pode desabilitar a instrumentação, sem mudar o resultado da análise. A remoção posterior pode localizar com segurança todas as linhas pela marca exata. AST e IR não representam esses logs.

## D-020 - Ordem de avaliação e lógica no PjBL 3

1. **Regra oficial:** há operadores lógicos e chamadas, mas a fonte não fixa a ordem de execução nem o curto-circuito.
2. **Lacuna:** chamadas podem ter efeitos, como alterar uma global ou imprimir; avaliar um operando desnecessário pode até causar erro de execução.
3. **Decisão do projeto:** operandos e argumentos são avaliados da esquerda para a direita. `E` usa curto-circuito: se o operando esquerdo for `FAL`, o direito não é executado. `OU` usa curto-circuito: se o esquerdo for `VER`, o direito não é executado. `XOR` executa ambos os operandos, da esquerda para a direita, e resulta em verdadeiro se forem diferentes. Na atribuição, o endereço do destino (incluindo índice e verificações) é calculado antes do valor da direita. Cada expressão é avaliada uma única vez, exceto quando o controle do programa a executa novamente em um laço.
4. **Impacto:** a AST conserva a precedência de D-010. A IR de `E`/`OU` usa blocos e saltos condicionais, com um valor booleano reunido na saída; `XOR` pode usar `xor i1`. Testes com chamadas e efeitos observáveis devem comprovar a ordem e a omissão do segundo operando. Nenhuma nova regra de parser é necessária.

## D-021 - Representação numérica e limites no PjBL 3

1. **Regra oficial:** os nomes dos tipos existem, sem larguras ou limites fixados pela fonte.
2. **Lacuna:** aceitar um valor no front-end e truncá-lo silenciosamente no backend tornaria o significado dependente da implementação.
3. **Decisão do projeto:** `inteira` é um inteiro assinado de 64 bits (`-9223372036854775808` a `9223372036854775807`), `flut` é IEEE 754 binary32 e `duplocarpado` é IEEE 754 binary64. Um literal real tem tipo `flut`, arredondado para binary32; seu armazenamento temporário em `double` no compilador não muda esse tipo. Literais reais maiores que `FLT_MAX` são rejeitados; valores pequenos podem arredondar para subnormal ou zero. Um literal inteiro vai até `9223372036854775807`; acima disso o lexer rejeita o token (E-LEX-006). Como não há sinal dentro do token, o menor valor de `inteira` é escrito por expressão, por exemplo `-9223372036854775807 - 1`. Tamanhos de vetor ficam entre 1 e `2147483647`, com índice `inteira` e origem zero.

   **Revisão (PjBL 3/4, 2026-11-03):** a primeira versão fixava `inteira` em 32 bits. Os programas de teste do professor (`casos_de_teste/nivel3` a `nivel5`) guardam CPFs de 11 dígitos em `inteira`, fora dessa faixa; em vez de rejeitá-los, a representação passou a 64 bits. O erro E-SEM-037 (literal fora de 32 bits) deixou de existir.
   Soma, subtração, multiplicação, menos unário e potência inteira usam aritmética modular de 64 bits. Em particular, negar `-9223372036854775808` produz o mesmo valor. `//` continua truncando em direção a zero e verifica as exceções de D-023. Aritmética real segue IEEE 754, sem pressupor identidades algébricas de números reais; o backend não usa `fast-math`. Comparações `<`, `>` e `==` são falsas se algum operando for NaN; `!` é verdadeira nesse caso. `palavra` é comparada pelo conteúdo UTF-8, não pelo endereço de armazenamento.
4. **Impacto:** o front-end valida faixas e evita aceitar tipos de tamanho infinito ou índice incompatível. LLVM usa `i64`, `float` e `double`; operações inteiras modulares não recebem `nsw`/`nuw`. O índice de vetor é verificado em `i64` e só então reduzido a `i32` para a GEP. O runtime usa tipos C de largura fixa e evita overflow assinado de C. Testes incluem limites, arredondamento e valores negativos. Portabilidade não implica reduzir `duplocarpado` para `float` em uma placa futura; em uma placa de 32 bits, `i64` ocupa um par de registradores sem mudar o significado.

## D-022 - Inicialização de variáveis

1. **Regra oficial:** há declarações sem inicializador, mas a fonte não define o valor inicial.
2. **Lacuna:** a semântica atual aceita leitura antes de atribuição e não possui análise de atribuição definitiva.
3. **Decisão do projeto:** globais e locais começam com zero para tipos numéricos, `FAL` para `bool`, palavra vazia para `palavra` e inicialização recursiva desses valores em vetores e registros. Locais são inicializados uma vez por chamada da função, antes dos comandos; executar novamente uma declaração dentro de laço não reinicializa a variável. Parâmetros recebem seus argumentos. As regras de declaração antes do uso e escopos de D-014 continuam valendo.
4. **Impacto:** o codegen reserva todos os locais no bloco de entrada e inicializa seus armazenamentos antes do corpo, inclusive os declarados em ramos. A representação interna de palavra vazia pode ser uma constante ou ponteiro nulo tratado como texto vazio pelo runtime; o programa observa o mesmo valor. Testes devem ler valores iniciais escalares e agregados e verificar persistência de locais entre iterações.

## D-023 - Falhas de execução e operações numéricas

1. **Regra oficial:** a fonte não fixa a reação a divisão por zero, domínio inválido ou índice dinâmico fora dos limites.
2. **Lacuna:** operações LLVM podem produzir comportamento indefinido ou `poison` quando seus pré-requisitos são violados.
3. **Decisão do projeto:** o programa termina com status não zero e diagnóstico em `stderr`, incluindo ao menos a linha da fonte, nos seguintes casos: divisor zero de `/` ou `//` (incluindo `-0.0`), `-9223372036854775808 // -1`, índice fora de `0..N-1`, argumento negativo de `raiz`, expoente negativo de potência inteira e falha de leitura de D-017. A raiz real e a potência real usam operações da precisão declarada; para potência, domínio inválido detectado pela biblioteca matemática também é erro. `0 ^ 0` vale 1. Potência inteira pode ser calculada por exponenciação por quadrados em aritmética modular. Estes testes de valor podem ocorrer estaticamente, quando comprováveis, ou no runtime; uma entrada sem erro estático não pode escapar das verificações dinâmicas necessárias.
4. **Impacto:** o codegen emite guardas antes de divisão/GEP ou chama auxiliares verificados do runtime. O runtime fornece uma rotina de falha `zl_*` sem retorno e funções matemáticas quando necessárias, com ABI em `decisoes_backend.md`. Testes negativos de execução conferem status e linha; testes positivos conferem truncamento de `//` com sinais diferentes e raiz/potência nas duas precisões. O runtime fica separado e o módulo LLVM não fixa placa, triple ou datalayout.

## D-024 - Intrínsecas de GPIO para o alvo ESP32

1. **Regra oficial:** a sintaxe Macaronica não define GPIO, temporização de placa nem nomes de pinos. Os tipos e as regras de expressão dos PjBL 1 e 3 continuam válidos.
2. **Lacuna:** um programa-fonte precisa controlar GPIO físico no ESP32 DevKit V1 sem incorporar chamadas C ou símbolos `zl_*` à gramática da Macaronica.
3. **Decisão do projeto para o PjBL 4:** acrescentam-se funções intrínsecas predeclaradas, com a sintaxe de chamada existente e sem novos tokens. Seus nomes seguem D-003, que não admite sublinhado em identificadores da fonte:

   | Nome na fonte | Parâmetros | Retorno | Significado |
   |---|---|---|---|
   | `pinoModo` | `inteira pino, inteira modo` | `vazio` | configura 0 = entrada, 1 = saída, 2 = entrada com pull-up, 3 = entrada com pull-down |
   | `pinoEscreve` | `inteira pino, bool nivel` | `vazio` | `VER` = nível alto de 3,3 V; `FAL` = nível baixo |
   | `pinoLe` | `inteira pino` | `bool` | lê nível lógico; alto resulta em `VER` |
   | `espera` | `inteira milissegundos` | `vazio` | solicita atraso em ticks do FreeRTOS, arredondando duração positiva para cima; a precisão física depende do escalonador |

   `espera(0)` não introduz espera. Duração negativa é erro de execução. Os quatro nomes ocupam o namespace global de funções e não podem ser redefinidos. Essas intrínsecas somente podem ser chamadas na compilação com `--alvo=esp32`; o alvo padrão de PC/POSIX continua com as regras do PjBL 3. Uma chamada sem o alvo ESP32 é erro de validação de alvo com localização da chamada, após a análise semântica comum, mesmo quando a assinatura é válida. Valores de pino e modo são conferidos pelo runtime ESP-IDF. Pinos ligados à flash (GPIO6–11) são proibidos; GPIO34–39 não podem ser saídas e não oferecem os modos com pull interno. `pinoEscreve` exige configuração prévia como saída; `pinoLe`, configuração prévia como entrada. A escolha de pino para um circuito concreto depende do pinout da placa e é registrada em `placa.md`, sem presumir LED onboard.
4. **Impacto:** lexer e parser reutilizam identificadores, chamadas e tipos existentes. A semântica predeclara as quatro assinaturas; a validação do alvo rejeita seu uso fora de ESP32 com diagnóstico específico. O gerador C seleciona a intrínseca por identidade de símbolo. O backend LLVM do PC mantém o contrato do PjBL 3; a validação de alvo impede chamadas GPIO nessa rota. A ABI C da placa usa `zl_pino_modo`, `zl_pino_escreve`, `zl_pino_le` e `zl_espera_ms`, com linha da fonte oculta acrescentada pelo gerador para erros de execução. O argumento `bool` de escrita é expandido para inteiro 0/1 na fronteira C; a leitura retorna inteiro 0/1 e o gerador o converte para `bool`. Testes verificam assinaturas, proibição no alvo padrão, modos e erros de GPIO no alvo ESP32. Esta extensão pertence ao projeto, não ao enunciado original da Macaronica.

## Questões pendentes para uma versão posterior

- Avaliar promoção entre operandos de operadores, fora do escopo de D-012.
- Definir literal próprio de `duplocarpado`.
- Definir leitura de texto e outras operações de texto, se necessárias.
- Medir na ferramenta ESP-IDF escolhida o custo e a precisão efetiva de `duplocarpado` como binary64 no ESP32 clássico, preservando a semântica documentada no PjBL 3.
