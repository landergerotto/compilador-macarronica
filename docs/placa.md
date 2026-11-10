# ESP32 DevKit V1: blink do LED integrado

## Programa e configuração

Fonte: [esp32_blink.mac](../examples/esp32_blink.mac). O compilador reutiliza
lexer, parser, AST anotada e semântica; `--alvo=esp32` seleciona a geração
C99. O ESP-IDF compila **esse C gerado** com o runtime ESP32. O runtime define
`app_main`, que chama `zl_principal`; o laço e as esperas vêm da fonte Macaronica.
O backend LLVM e o runtime POSIX continuam disponíveis para o PjBL 3.
O runtime ESP32 desta etapa implementa GPIO, espera, impressão de texto/nova
linha e falha de execução, suficientes para este blink. Entrada numérica,
impressão numérica e helpers matemáticos ainda usam somente o runtime POSIX;
programas que dependam deles exigem ampliar o runtime ESP32 antes de ligar.

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
