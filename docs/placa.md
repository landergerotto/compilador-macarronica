# Placa: ESP32 DevKit V1 (Parte D do PjBL 4)

## Entrega da Parte D (17/11/2026)

**Placa:** ESP-WROOM-32 na DOIT ESP32 DevKit V1 (ESP32-D0WDQ6, Xtensa LX6
dual core, flash de 4 MB), ligada pela ponte USB-serial CP210x (COM4 nesta
máquina).

**Rota:** o compilador gera C99 a partir da AST anotada (`--alvo=esp32`,
[`src/codegen_c.c`](../src/codegen_c.c)), e o GCC do Xtensa compila esse C com
o runtime [`runtime/zl_runtime_esp32.c`](../runtime/zl_runtime_esp32.c). É a
rota C que o enunciado indica para o ESP32 clássico: o LLVM do PC não tem o
alvo Xtensa. Só o runtime da placa é escrito à mão.

**Tripla e CPU:** `xtensa-esp32-elf` (toolchain `xtensa-esp32-elf-gcc`). O GCC
do Xtensa é gerado para um núcleo fixo (LX6, com FPU só de precisão simples),
por isso não recebe `-mcpu`; no LLVM da Espressif o equivalente seria
`-mcpu=esp32`. A rota acrescenta `-ffp-contract=off`, para o GCC não fundir
multiplicação e soma (FMA) e as contas reais darem o mesmo resultado do PC.

**Ferramental e versões** (rota Arduino, a padrão do `make placa`):

- arduino-cli 1.4.1, o que vem com a Arduino IDE;
- núcleo `esp32:esp32` 3.3.11, FQBN `esp32:esp32:esp32doit-devkit-v1`;
- GCC `xtensa-esp32-elf-gcc` 14.2.0 (crosstool-NG esp-14.2.0_20260121);
- esptool v5.3.1 na gravação.

A rota ESP-IDF 5.5.2 (`ROTA=idf`, descrita mais abaixo) continua disponível.

**Comandos** (na raiz do repositório; no Windows, `mingw32-make`):

```text
mingw32-make placa PROG=tests/placa/01_laco_vetor.mac              # C gerado e firmware em placa/01_laco_vetor/
mingw32-make placa-gravar PROG=tests/placa/01_laco_vetor.mac PORT=COM4
mingw32-make placa-monitor PORT=COM4                                # serial da placa
mingw32-make placa-testar PORT=COM4                                 # grava os 8 de tests/placa e compara a serial
mingw32-make placa-pc PROG=tests/placa/04_led_botao.mac             # o mesmo C no PC, com pinos simulados
```

**Gravação nesta DevKit V1:** às vezes o esptool para em `Connecting...` e
falha com `Wrong boot mode detected (0x13)`. No Windows o DTR e o RTS mudam
em duas chamadas separadas, o EN sobe antes de o IO0 descer e o chip liga
normalmente em vez de entrar no modo de gravação. A solução é segurar o botão
BOOT (IO0) enquanto aparecer `Connecting...` e soltar quando a gravação
começar. O `placa-testar` tenta gravar até `TENTATIVAS` vezes (padrão 4) e
avisa quando segurar e quando soltar o BOOT; com `BOOT=1` avisa já na primeira
tentativa.

**Decisões dependentes do alvo:**

| Item | Na ESP32 | Motivo |
|---|---|---|
| `inteira` | `int64_t` | o mesmo i64 do PC (CPF de 11 dígitos); o GCC faz a conta de 64 bits em pares de registradores de 32 |
| `flut` | `float` | FPU de precisão simples do LX6 |
| `duplocarpado` | `double` IEEE 754 de 64 bits, emulado em software | o LX6 não tem FPU de precisão dupla; o resultado é o mesmo do PC, só mais lento |
| Impressão | `%.9g` para `flut`, `%.17g` para `duplocarpado`, `VER`/`FAL` para `bool` | [`runtime/zl_formato.h`](../runtime/zl_formato.h), o mesmo arquivo do PC |
| Pinos | o número no programa é o número do GPIO; LED azul no GPIO2 e botão BOOT no GPIO0 | regras em [`runtime/zl_placa.h`](../runtime/zl_placa.h), usadas também pelo simulador do PC |
| Uso inválido de pino | erro de execução 8 (`ZL_ERRO_PLACA`), com a regra na serial e o LED piscando | GPIO6 a 11 são da flash; 1 e 3, da serial USB; 34 a 39 só leem e não têm pull-up/pull-down; 20, 24 e 28 a 31 não existem; escrever exige modo saída e ler exige modo entrada |
| `espera(ms)` | `vTaskDelay`, arredondado para cima no tick | bloqueia só a tarefa do programa, sem ocupar a CPU |
| Pilha | tarefa FreeRTOS com 65536 bytes (`zl_config.h`) | o runtime mede a pilha usada e a mostra no fim |
| Memória | `--ram-limite=65536`: E-TGT-002 quando as globais mais o maior quadro de função passam do limite | ver a seção "Memória" |
| GPIO e espera fora da placa | sem `--alvo=esp32`, `pinoModo`, `pinoEscreve`, `pinoLe` e `espera` dão E-TGT-001 | o PC não tem pinos; `make placa-pc` usa o simulador [`runtime/zl_placa_pc.c`](../runtime/zl_placa_pc.c) |

**Resultados na placa** (`make placa-testar PORT=COM4`, 17/11/2026). Flash é o
programa na partição de 1310720 bytes (20% a 21%); RAM é a memória estática,
de 327680 bytes (6%), além da pilha da tarefa.

| Programa | O que cobre | Resultado | Flash (bytes) | RAM (bytes) | Pilha usada (de 65536) |
|---|---|---|---|---|---|
| `01_laco_vetor` | laço e vetor | saída igual ao `.esperado`, status 0 | 272696 | 22196 | 808 |
| `02_recursao` | função recursiva | saída igual, status 0 | 272008 | 22116 | 952 |
| `03_erro_indice` | erro de execução da linguagem (índice) | mensagem igual ao `.erro` | 272452 | 22116 | — |
| `04_led_botao` | E/S física: LED no GPIO2, botão BOOT no GPIO0 | saída igual, status 0 | 274244 | 22156 | 736 |
| `05_erro_pino` | erro da placa: escrita no pino 4 sem modo saída | mensagem igual ao `.erro` | 273836 | 22156 | — |
| `06_reais` | `flut` e `duplocarpado` | saída igual, status 0 | 277732 | 22116 | 1976 |
| `07_entrada` | leitura pela serial (`.in`) | saída igual, status 0 | 280480 | 22116 | 1976 |
| `08_rejeitado_memoria` | verificação de memória | recusado pelo compilador com E-TGT-002 | — | — | — |

As evidências de cada programa ficam em `placa/<programa>/`:

- `serial.txt`: a serial desde o boot;
- `saida.txt`: o que saiu entre as marcas do runtime;
- `proveniencia.txt`: commit do compilador, comando de geração, versões, tamanhos e sha256 do `.bin`;
- o C gerado (`.c`) e o firmware (`.bin`, bootloader e partições).

O `.elf`, de 5,7 MB cada, fica fora do git. O 01 e o 02 gravaram na segunda
tentativa, com o botão BOOT.

**Vídeo (até 1 minuto):** pendente; o link entra aqui antes da entrega.

## Rota ESP-IDF: blink do LED (10/11/2026)

## Programa e configuração

Fonte: [esp32_blink.mac](../examples/esp32_blink.mac). O compilador reutiliza
lexer, parser, AST anotada e semântica; `--alvo=esp32` seleciona a geração
C99. O ESP-IDF compila **esse C gerado** com o runtime ESP32. O runtime define
`app_main`, que chama `zl_principal`; o laço e as esperas vêm da fonte Macaronica.
O backend LLVM e o runtime POSIX continuam disponíveis para o PjBL 3.
Em 10/11 o runtime ESP32 só tinha GPIO, espera, impressão de texto e falha de
execução, o suficiente para este blink. Desde 15/11 ele também imprime e lê
todos os tipos e tem potência, raiz e comparação de texto (ver
[defesa/placa.md](defesa/placa.md), seção 3).

Placa informada: ESP-WROOM-32 / DOIT ESP32 DevKit V1, Xtensa LX6. A
[documentação da placa no Zephyr](https://docs.zephyrproject.org/latest/boards/others/doit_esp32_devkit_v1/doc/index.html)
e sua [definição DTS](https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/others/doit_esp32_devkit_v1/doit_esp32_devkit_v1_procpu.dts)
identificam o **LED azul programável no GPIO 2, ativo em nível alto**.
O LED de alimentação é separado. Algumas variantes/clones podem não montar o
LED programável; a identificação do chip por USB não confirma sua presença.
Não há troca automática para LED externo ou outro pino.

A fonte configura `pinoModo(2, 1)` uma vez e repete continuamente:
`pinoEscreve(2, VER)`, `espera(1000)`, `pinoEscreve(2, FAL)`, `espera(1000)`.
As mensagens ACESO/APAGADO permitem acompanhar a execução pela UART.
O intervalo solicitado é 1000 ms por estado, sujeito ao tick e escalonamento
FreeRTOS e ao tempo das chamadas de impressão. Não é uma garantia metrológica.

## Construir, gravar e monitorar

No PowerShell, na raiz do repositório:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action generate
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action build
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action flash -Port COM4
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action monitor -Port COM4
```

Sair do monitor com **Ctrl+]**. Feche outro monitor serial antes de gravar.
Se a conexão automática parar em `Connecting...`, mantenha BOOT pressionado
no início da conexão e solte quando a gravação começar.

`-Program caminho/programa.mac` escolhe outra fonte. `-IdfPath` permite usar
outra instalação do ESP-IDF. O script usa `IDF_PATH` ou a instalação local
`C:/Espressif/frameworks/esp-idf-v5.5.2`; ativa o ambiente automaticamente.
O alias Windows 8.3 evita espaços nos caminhos usados pelo ESP-IDF, sem copiar
o projeto. Se o sistema não fornecer esse alias, use uma pasta sem espaços.

Também há `mingw32-make placa`, `placa-gerar`, `placa-gravar PORT=COM4`
e `placa-monitor PORT=COM4`. Em POSIX, use `make` com o ambiente ESP-IDF ativo
e a porta correspondente em `PORT`.

Saídas:

- `build/esp32-generated/prog_gerado.c`: C produzido pelo compilador.
- `build/esp32-generated/proveniencia.json`: hashes da fonte, compilador e C (script Windows).
- `build/esp32`: firmware ELF/BIN, bootloader, partições, mapa do linker e configuração.

`flash` sempre regenera o C e reconstrói antes de gravar. Não existe um blink
manual alternativo dentro de `app_main`.

## Contrato das funções

| Fonte Macaronica | Tipo | Runtime (último argumento oculto: linha) |
|---|---|---|
| `pinoModo(p, modo)` | `(inteira, inteira) → vazio` | `zl_pino_modo(int32_t, int32_t, int32_t)` |
| `pinoEscreve(p, nivel)` | `(inteira, bool) → vazio` | `zl_pino_escreve(int32_t, int32_t, int32_t)` |
| `espera(ms)` | `(inteira) → vazio` | `zl_espera_ms(int32_t, int32_t)` |

A assinatura de `pinoLe(inteira) → bool` já existia no contrato desta extensão;
ela não é usada pelo blink. Não há tokens novos. `bool` cruza a ABI como 0/1.
As chamadas GPIO são rejeitadas no alvo PC com `E-TGT-001`.
Modos: 0 entrada, 1 saída, 2 entrada com pull-up, 3 entrada com pull-down.
O runtime valida pinos, modos, configuração prévia e resultados ESP-IDF.
GPIO6–11 são reservados à flash; GPIO34–39 não aceitam saída nem pull interno.
GPIO2 é configurado após a inicialização, sem mudar seu papel no boot.
Espera negativa é erro; zero não bloqueia; positiva arredonda para cima em ticks.
Erros registram código e linha e encerram a tarefa Macaronica.

## Memória

A checagem `--ram-limite=65536` usa um orçamento de projeto, não RAM física
medida. Soma globais e a maior soma de parâmetros/locais de uma função,
incluindo vetores e campos, sem padding. Exclui temporários C, alinhamento,
chamadas aninhadas, recursão, heap, runtime e FreeRTOS. Aceitação não garante
caber na placa. O build ESP-IDF e seu mapa dão a ocupação real estática.

## Evidências desta sessão (10/11/2026)

- ESP-IDF 5.5.2 instalado em `C:/Espressif/frameworks/esp-idf-v5.5.2`.
- Toolchain Xtensa GCC 14.2.0 (esp-14.2.0_20251107), Python 3.13.14.
- `esptool --port COM4 chip_id`: ESP32-D0WDQ6, revisão 1.0, dual core, cristal 40 MHz.
- `esptool --port COM4 flash_id`: flash detectada de 4 MB.
- Fonte Macaronica gerada em C com GPIO2, níveis 1/0 e esperas de 1000 ms.
- Build ESP-IDF concluído: imagem da aplicação com 144480 bytes (141,1 KiB),
  partição de aplicação de 1 MiB, flash configurada em 4 MB / DIO / 40 MHz.
- Firmware gravado na COM4: bootloader, aplicação e tabela de partições;
  hashes verificados pelo esptool, seguido de reset automático.
- Captura UART real a 115200 baud: oito eventos ACESO/APAGADO alternados;
  sete intervalos entre 0,9997 s e 1,0002 s. Isso verifica mensagens da
  execução da fonte e não mede opticamente o LED.
- Registros locais: build/esp32-build.log, build/esp32-flash.log e
  build/esp32/serial_blink.json. Repetir o monitor não exige nova gravação.
- make test: 119 verificações PjBL 1/3 e 21 verificações da rota C, todas
  aprovadas. Inclui três ciclos do blink com stub no host, rejeição de GPIO
  no PC, ordem de avaliação, wrap e erros de execução.
- A expectativa do teste de recursão foi corrigida para a fonte atual
  `fatorial(7)`: 5040 e sete chamadas, sem mudar o programa.
- Student Reviewer: sem bloqueador no gerador C, seleção de backend e blink.
- Confirmação visual do usuário em 10/11/2026: o LED azul integrado está piscando. A captura UART complementa essa observação com os intervalos das mensagens.

## Para explicar na arguição

1. Por que gerar C da AST anotada? O frontend já resolveu nomes/tipos;
   a toolchain Xtensa faz a compilação cruzada, sem alterar o LLVM do PC.
2. Onde fica a dependência da placa? No runtime `zl_*` e projeto ESP-IDF;
   o laço e os valores de pino/tempo vêm do programa Macaronica.
3. Por que `espera` usa FreeRTOS? Bloqueia a tarefa pelo tempo solicitado e
   permite ao sistema executar outras tarefas; um laço de espera ocupada não faz isso.
