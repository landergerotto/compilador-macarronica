# Parte D (placa): alvos make placa*. Dono: frente da placa (luiz/placa-esp32).
# Incluido pelo Makefile principal; usa $(RELEASE_TARGET), $(EXEEXT), $(BASH)
# e $(CC).
#
# O firmware sempre usa o C99 gerado pelo compilador (--alvo=esp32). Rotas:
#   ROTA=arduino (padrao): arduino-cli e nucleo arduino-esp32 (tools/placa.sh);
#                          firmware em placa/<programa>/.
#   ROTA=idf:              ESP-IDF (tools/esp32.ps1 no Windows, idf.py fora).
#
#   make placa PROG=tests/placa/01_vetor.mac      compila para a placa
#   make placa-gravar PROG=... PORT=COM4          compila e grava
#   make placa-monitor PORT=COM4                  serial a 115200
#   make placa-pc PROG=...                        mesmo C, rodando no PC
#   make placa-testar [PROG=...] [PORT=COM4]      grava, le a serial e compara
#                                                 com o .esperado (sem PROG:
#                                                 todos os tests/placa/*.mac)
#   PILHA=N  bytes da pilha da tarefa do programa (e o --ram-limite)
#   PORT     na rota arduino o padrao e auto: a primeira placa USB conectada
PROG ?= examples/esp32_blink.mac
ROTA ?= arduino
ifeq ($(ROTA),arduino)
PORT ?= auto
else
PORT ?= COM4
endif
PILHA ?= 65536
IDF_PY ?= idf.py

.PHONY: placa placa-gerar placa-gravar placa-monitor placa-pc placa-testar

placa-pc: $(RELEASE_TARGET)
	"$(BASH)" tools/placa.sh pc "$(PROG)" "$(PILHA)" "$(RELEASE_TARGET)" "$(CC)"

ifeq ($(ROTA),arduino)
placa-gerar: $(RELEASE_TARGET)
	"$(BASH)" tools/placa.sh gerar "$(PROG)" "$(PILHA)" "$(RELEASE_TARGET)"

placa: $(RELEASE_TARGET)
	"$(BASH)" tools/placa.sh compilar "$(PROG)" "$(PILHA)" "$(RELEASE_TARGET)"

placa-gravar: placa
	"$(BASH)" tools/placa.sh gravar "$(PROG)" "$(PORT)"

placa-monitor:
	"$(BASH)" tools/placa.sh monitor "$(PORT)"

PLACA_TESTES = $(if $(filter command line,$(origin PROG)),$(PROG),$(sort $(wildcard tests/placa/*.mac)))
placa-testar: $(RELEASE_TARGET)
	"$(BASH)" tools/placa.sh testar "$(PORT)" "$(PILHA)" "$(RELEASE_TARGET)" $(PLACA_TESTES)
else ifeq ($(ROTA),idf)
ifeq ($(EXEEXT),.exe)
placa:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action build -Program "$(PROG)" -Port "$(PORT)"

placa-gerar:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action generate -Program "$(PROG)"

placa-gravar:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action flash -Program "$(PROG)" -Port "$(PORT)"

placa-monitor:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action monitor -Port "$(PORT)"
else
placa-gerar: $(RELEASE_TARGET)
	@mkdir -p build/esp32-generated
	$(RELEASE_TARGET) --alvo=esp32 --ram-limite=$(PILHA) "$(PROG)" -o build/esp32-generated/prog_gerado.c

placa: placa-gerar
	$(IDF_PY) -C esp32 -B ../build/esp32 -D SDKCONFIG="$(CURDIR)/build/esp32/sdkconfig" -D IDF_TARGET=esp32 build

placa-gravar: placa
	$(IDF_PY) -C esp32 -B ../build/esp32 -p "$(PORT)" -b 115200 flash

placa-monitor:
	$(IDF_PY) -C esp32 -B ../build/esp32 -p "$(PORT)" monitor
endif
else
$(error ROTA deve ser arduino ou idf, recebido: $(ROTA))
endif
