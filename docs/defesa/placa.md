# Defesa: frente da placa (Parte D do PjBL 4 e back-end C)

Material de estudo do Luiz para a arguição. Cada seção diz o que foi feito, por
que foi feito assim e onde está no código.

## 1. `inteira` como `int64_t` no back-end C (entrega 1)

**O quê.** O contrato do PjBL 3/4 fixa `inteira` em 64 bits em tudo: `i64` na
IR do LLVM, `int64_t` no C gerado e no runtime. O back-end C ainda usava
`int32_t`; agora usa `int64_t` (`c_type` em `src/codegen_c.c`). `bool`
continua `int32_t` 0/1, porque é assim que ele cruza a ABI do runtime nos dois
back-ends (`zl_imprime_bool(int32_t)`, `zl_pino_escreve(..., int32_t nivel, ...)`).

**Pontos que mudaram em `src/codegen_c.c`:**

| Onde | Antes (32 bits) | Agora (64 bits) | Por quê |
|---|---|---|---|
| `c_type` | `int32_t` | `int64_t` | contrato i64 |
| `c_literal` | erro E-C-001 se o literal passava de `INT32` | `INT64_C(n)` | o lexer já limita a magnitude a `INT64_MAX`; `INT64_C` garante 64 bits mesmo onde `long` tem 32 (Xtensa) |
| `c_binary`, `//` e `/` | guarda `INT32_MIN / -1` | guarda `INT64_MIN / -1` | o quociente não cabe: em C é comportamento indefinido; gera o erro de execução 2, igual à IR |
| `c_binary`, `+ - *` | `mac_i32((uint32_t)a op (uint32_t)b)` | `mac_i64((uint64_t)a op (uint64_t)b)` | overflow assinado é indefinido em C; em `uint64_t` a conta é módulo 2^64, como `add/sub/mul i64` |
| `c_unary`, menos | caso especial para `-2147483648` | `mac_i64(0 - (uint64_t)x)` | `-INT64_MIN` volta a `INT64_MIN`, como `sub i64 0, x`; o caso especial era só do i32 |
| assinatura de `principal` | tipo da inteira | `int32_t zl_principal(void)` | é a assinatura de `runtime/zl_runtime.h`; o `respost` usa `mac_status(x)`, os 32 bits baixos, igual ao `trunc i64 to i32` da IR |
| chamadas de `pinoModo`, `pinoEscreve`, `pinoLe`, `espera` | passavam o valor direto | `mac_arg_i32(x)` | o runtime recebe `int32_t`; a conversão satura em vez de truncar, então `2^32 + 2` não vira o GPIO2: chega como `INT32_MAX`, que o runtime rejeita |

**As três funções auxiliares** ficam no começo de todo C gerado (`codegen_c_emit`):

- `mac_i64(uint64_t)`: volta de `uint64_t` para `int64_t` sem conversão fora
  da faixa (que em C depende do compilador): se o valor passa de `INT64_MAX`,
  calcula `-1 - (UINT64_MAX - v)`.
- `mac_status(int64_t)`: pega os 32 bits baixos e reconstrói o `int32_t` da
  mesma forma.
- `mac_arg_i32(int64_t)`: satura em `INT32_MIN`/`INT32_MAX`.

**Testes.** `tests/execucao/12_inteira_64bits` (CPF, `2^62`, `INT64_MAX + 1`,
`-INT64_MIN`, índice calculado com inteira grande) e
`tests/execucao/erros/02_divisao_overflow` (`INT64_MIN // -1`) passaram a
passar na rota C. `tests/esp32/ordem_wrap` testava o *wrap* de 32 bits
(`2147483647 + 1 = -2147483648`); com i64 essa conta dá `2147483648`. O teste
não foi afrouxado: o `.esperado` mudou para o valor de 64 bits e ganhou três
linhas com o *wrap* de verdade, em `INT64_MAX + 1`, `INT64_MIN * -1` e
`-INT64_MIN`. Todo C gerado pela bateria compila com
`-Wall -Wextra -pedantic -Wconversion -Werror`.

## 2. `test-c` estável no Windows

**Sintoma.** De vez em quando o `mingw32-make test-c` falhava com
`ld: reopening .../build/test-c.XXXX/programa.exe: Permission denied`.

**Causa.** O script reescrevia o mesmo `programa.exe` a cada teste. No
Windows, um executável recém-criado ou recém-executado pode ficar preso por
instantes (o antivírus o abre para examinar; um processo morto pelo `timeout`
demora a liberar o arquivo). O `ld` do MinGW fecha e reabre o executável no
fim da ligação; se outro processo o segura nesse instante, a reabertura falha.

**Correção** (`tests/run_c_tests.sh`):

1. Cada teste tem os seus arquivos (`$tmp/<nome>.c`, `$tmp/<nome>.exe`); nada
   é reescrito.
2. `compile_c` tenta de novo, até 5 vezes com 1 s de intervalo, **só** quando
   o erro contém `Permission denied`; qualquer outro erro falha na hora.
3. O runtime POSIX é compilado uma vez (`zl_runtime_posix.o`) e só ligado em
   cada teste, o que também deixa o `test-c` mais rápido.

Verificação: `test-frontend` com 88 de 88 e três `test-c` seguidos com 22 de 22.

## 3. Runtime da placa completo (entrega 2)

O runtime da placa implementa toda a interface `runtime/zl_runtime.h`. Ele é
escrito à mão, mas é só a plataforma: o programa vem inteiro do compilador.

| Arquivo | Papel |
|---|---|
| `runtime/zl_formato.h` | Texto de cada valor, linha de erro e validação da entrada, **o mesmo código no PC e na placa** |
| `runtime/zl_placa.h` | Regras dos pinos da DOIT DevKit V1 (quais existem, quais são só entrada) e da espera |
| `runtime/zl_runtime_esp32.c` | Runtime da placa: impressão e leitura pela serial, potência, raiz, textos, GPIO, espera, erro com LED |
| `runtime/zl_esp32.h` | As 4 funções de serial que cada rota fornece e `zl_esp32_inicia` |
| `runtime/zl_plataforma_arduino.cpp` | Rota Arduino: ponte C++ → C para a `Serial` |
| `runtime/zl_plataforma_idf.c` | Rota ESP-IDF: driver de UART0 e `app_main` |
| `runtime/zl_placa_pc.c` | Os mesmos pinos simulados no PC, com as mesmas regras (para o `test-c`) |

Decisões:

- **Mesma saída por construção.** O formato saiu do runtime POSIX para
  `zl_formato.h`, um cabeçalho com implementação: cada runtime o inclui com
  `ZL_FORMATO_IMPLEMENTACAO` em um único arquivo. Assim nenhuma linha de
  ligação existente mudou (testes, bench, qa continuam ligando só
  `zl_runtime_posix.c`). No runtime POSIX mudou apenas isso.
- **Reais.** `flut` sai com `%.9g` e `duplocarpado` com `%.17g` (o mínimo que
  volta ao mesmo valor binário). Os algarismos vêm da libc, que arredonda
  corretamente no glibc, no printf ANSI do MinGW e no newlib da placa. O texto
  é normalizado: NaN vira sempre `nan` (o glibc escreve `-nan`), `inf`/`-inf`,
  expoente com 2 algarismos.
- **Erro de execução.** A mesma linha do PC (`linha N: erro de execucao C:
  mensagem`), pela serial, depois uma marca e o LED azul (GPIO2) piscando a
  cada 250 ms para sempre. A placa não reinicia (isso repetiria o programa).
- **Código 8, serviços da placa.** Uso inválido de pino ou espera (pino
  inexistente, GPIO6 a 11 da flash, GPIO1/3 da serial, saída em GPIO34 a 39,
  escrita sem `pinoModo`, espera negativa) dá `erro de execucao 8`, com o
  texto da regra. As regras ficam em `zl_placa.h`, usadas pela placa e pela
  simulação no PC, que dão o mesmo texto.
- **Tarefa própria.** `zl_principal` roda numa tarefa FreeRTOS `macaronica`,
  no núcleo 1, com pilha de `ZL_PILHA_BYTES` (64 KiB por padrão), e não na
  tarefa do `loop()` do Arduino (8 KiB). O watchdog de tarefas vigia só o
  ocioso do núcleo 0, então um cálculo longo não reinicia a placa.
- **Marcas na serial.** `--- Macaronica: inicio do programa ---` antes e
  `--- Macaronica: fim do programa, status N, pilha usada X de Y bytes ---`
  depois. O script de captura compara só o que fica entre elas. A pilha usada
  vem da marca d'água do FreeRTOS: é o número real da placa.
- **Entrada.** Tokens separados por espaço, como no PC, lidos da serial. Na
  placa não há fim de arquivo: a leitura espera o próximo byte.

Verificação: `test-frontend` 88/88 e `test-c` 22/22 no Windows. O sketch do
blink com o runtime completo compila e liga com o `arduino-cli` (nucleo
esp32 3.3.11). `zl_runtime_esp32.c` e `zl_plataforma_idf.c` compilam com
`-Wall -Wextra -Werror` no gcc Xtensa, contra os cabeçalhos do ESP-IDF 5.5.5
do núcleo Arduino. **Não testado:** o build pela rota ESP-IDF (`idf.py`),
porque o ESP-IDF não está instalado nesta máquina.

## Perguntas prováveis da arguição

**Por que `int64_t` e não `long long`?** `int64_t` tem exatamente 64 bits em
qualquer alvo (`<stdint.h>`); `long long` tem *pelo menos* 64. É a tradução
direta de `i64`.

**Por que fazer a soma em `uint64_t`?** Em C, overflow de inteiro com sinal é
comportamento indefinido: o compilador pode supor que ele não acontece e
otimizar errado. Overflow sem sinal é definido (módulo 2^64). A IR do LLVM
faz `add i64` sem `nsw`, que também dá a volta módulo 2^64; assim os dois
back-ends dão o mesmo resultado.

**E a divisão?** `INT64_MIN // -1` daria `2^63`, que não cabe em `int64_t`. Em
C é indefinido (no x86 a instrução `idiv` gera exceção). Os dois back-ends
testam antes e chamam `zl_erro_execucao(2, linha)`. A divisão por zero é
testada antes também (erro 1).

**Por que `zl_principal` devolve `int32_t` se a inteira tem 64 bits?** Porque
é o status do processo (no PC, o código de saída do `main`), e a ABI com o
runtime foi fixada em 32 bits. O `respost` da principal passa os 32 bits
baixos, igual ao `trunc` da IR.

**O que acontece com `pinoEscreve(4294967298, VER)`?** O valor não cabe no
`int32_t` do runtime. Truncar daria 2 (o GPIO2, um pino válido). A conversão
satura e entrega `INT32_MAX`, e o runtime responde com erro de pino
inexistente.

**Como vocês garantem que a placa imprime o mesmo que o PC?** O texto não é
escrito duas vezes: o PC e a placa compilam o mesmo `zl_formato.h`. O que
muda é só o destino (stdout/stderr no PC, serial na placa). Os `tests/placa`
têm `.esperado` gerado no PC, e o script de captura compara com a serial.

**Por que não usar `printf` direto na placa?** Daria o mesmo para inteiros,
mas o `%lld` e o `-nan` variam entre bibliotecas C. Com `zl_formato.h` a
conversão de inteiro é manual e os casos especiais dos reais são fixados.

**O que acontece se o programa tentar usar o GPIO6?** Erro de execução 8,
"pino reservado para a memoria flash". Esse pino liga a flash SPI: escrever
nele trava a placa. Melhor recusar com mensagem do que travar.
