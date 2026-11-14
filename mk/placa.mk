# Parte D (placa): alvos make placa*. Dono: frente da placa (luiz/placa-esp32).
# Incluido pelo Makefile principal; usa $(RELEASE_TARGET), $(EXEEXT) e $(BASH).
#
# Rota C99 -> ESP-IDF; o firmware sempre usa a saida do compilador Macaronica.
PROG ?= examples/esp32_blink.mac
PORT ?= COM4
IDF_PY ?= idf.py

.PHONY: placa placa-gerar placa-gravar placa-monitor

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
	$(RELEASE_TARGET) --alvo=esp32 "$(PROG)" -o build/esp32-generated/prog_gerado.c

placa: placa-gerar
	$(IDF_PY) -C esp32 -B ../build/esp32 -D SDKCONFIG="$(CURDIR)/build/esp32/sdkconfig" -D IDF_TARGET=esp32 build

placa-gravar: placa
	$(IDF_PY) -C esp32 -B ../build/esp32 -p "$(PORT)" -b 115200 flash

placa-monitor:
	$(IDF_PY) -C esp32 -B ../build/esp32 -p "$(PORT)" monitor
endif
