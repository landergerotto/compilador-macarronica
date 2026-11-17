# Comandos Macaronica — computador e ESP32

Comandos para **PowerShell no Windows**, com as ferramentas já instaladas nesta máquina.
Execute a preparação ao abrir um terminal novo. Troque o valor de `$fonte` pelo seu arquivo `.mac`.

## 0. Preparar o terminal

```powershell
Set-Location 'C:\Faculdade\Materias\9 Periodo\Linguagens Formais e Compiladores\PjBL\compilador-macarronica-front'
$env:Path = "C:\msys64\ucrt64\bin;C:\mingw64\bin;C:\msys64\usr\bin;$env:Path"

# Compilar o próprio compilador Macaronica.
mingw32-make release
```

## 1. Compilar e executar no computador

### Rota LLVM do PjBL 3

```powershell
# Exemplo pronto: imprime 22. Troque pelo caminho do seu programa.
$fonte = '.\tests\execucao\05_lacos.mac'
New-Item -ItemType Directory -Force .\build\experiments | Out-Null

# Macaronica -> LLVM IR textual.
.\build\release\macaronica.exe $fonte -o .\build\experiments\programa.ll

# Validar o LLVM IR.
llvm-as .\build\experiments\programa.ll -o .\build\experiments\programa.bc
opt -passes=verify .\build\experiments\programa.ll -disable-output

# LLVM IR + runtime do PC -> executável Windows.
clang .\build\experiments\programa.ll .\runtime\zl_runtime_posix.c -lm -o .\build\experiments\programa.exe

# Executar no computador.
.\build\experiments\programa.exe
```

### Alternativa: gerar C99 e executar no computador

```powershell
$fonte = '.\tests\execucao\05_lacos.mac'
New-Item -ItemType Directory -Force .\build\experiments | Out-Null

.\build\release\macaronica.exe --emit-c $fonte -o .\build\experiments\programa.c
gcc -std=c99 -Wall -Wextra -pedantic -I .\runtime .\build\experiments\programa.c .\runtime\zl_runtime_posix.c -lm -o .\build\experiments\programa_c.exe
.\build\experiments\programa_c.exe
```

## 2. Compilar e mandar para a ESP32

A placa atual usa **COM4**. O exemplo pisca o LED azul integrado no **GPIO 2**.
O runtime ESP32 atual atende GPIO, espera e impressão de texto/nova linha;
entrada e impressão numéricas e helpers matemáticos ainda exigem ampliar esse runtime.

### Gerar, compilar e gravar — comando principal

```powershell
# Troque pelo seu programa Macaronica para a placa.
$fonte = '.\examples\esp32_blink.mac'

# Faz tudo: compila o compilador, gera C, constrói o firmware e grava.
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action flash -Program $fonte -Port COM4
```

### Acompanhar as mensagens da placa

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action monitor -Port COM4

# Sair do monitor: Ctrl + ]
# Feche o monitor antes de gravar de novo para liberar a COM4.
```

### Somente gerar C ou construir o firmware

```powershell
$fonte = '.\examples\esp32_blink.mac'

# Só gerar o C, sem construir nem gravar firmware.
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action generate -Program $fonte

# Gerar C e construir firmware, sem gravar na placa.
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\esp32.ps1 -Action build -Program $fonte

# Ver o C gerado para a placa.
Get-Content .\build\esp32-generated\prog_gerado.c

# Ver o firmware construído.
Get-Item .\build\esp32\macaronica_esp32.bin
```

O script ativa automaticamente o ESP-IDF instalado em `C:\Espressif\frameworks\esp-idf-v5.5.2`.
Após editar seu `.mac`, repita o comando `flash`. Após gravar, a placa executa o
programa automaticamente e volta a executá-lo ao reiniciar enquanto o firmware estiver gravado.

## 3. Testes do projeto (opcional)

```powershell
# Executar após a preparação do terminal.
mingw32-make test

# Somente os testes da geração C.
mingw32-make test-c
```
