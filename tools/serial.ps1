# Le a serial da placa para tools/placa.sh (make placa-testar), no Windows.
#
# Reinicia a placa pela linha RTS (EN), como o esptool faz, para pegar a saida
# desde o boot. Manda o arquivo -Entrada (o .in do programa) assim que a marca
# de inicio do runtime aparece e para na marca de fim ou de erro, ou depois de
# -Segundos. Escreve na saida padrao tudo o que chegou pela serial.
param(
    [Parameter(Mandatory = $true)][string]$Porta,
    [int]$Baud = 115200,
    [int]$Segundos = 20,
    [string]$Entrada = ''
)
$ErrorActionPreference = 'Stop'
$utf8 = New-Object System.Text.UTF8Encoding $false
try { [Console]::OutputEncoding = $utf8 } catch { }   # sem console (saida redirecionada)
$inicio = '--- Macaronica: inicio do programa ---'
$finais = @(' bytes ---', 'LED GPIO2 piscando ---')

$serial = New-Object System.IO.Ports.SerialPort $Porta, $Baud
$serial.Encoding = $utf8
$serial.DtrEnable = $false   # IO0 solto: boot normal, nao o modo de gravacao
$serial.RtsEnable = $true    # EN em nivel baixo: a placa fica em reset
$serial.Open()
$texto = New-Object System.Text.StringBuilder
try {
    Start-Sleep -Milliseconds 200
    $serial.RtsEnable = $false   # solta o EN: a placa reinicia e roda o programa
    $enviou = ($Entrada -eq '')
    $limite = (Get-Date).AddSeconds($Segundos)
    while ((Get-Date) -lt $limite) {
        [void]$texto.Append($serial.ReadExisting())
        $agora = $texto.ToString()
        $posicao = $agora.IndexOf($inicio)
        if ($posicao -ge 0) {
            if (-not $enviou) {
                $serial.Write([System.IO.File]::ReadAllText($Entrada))
                $enviou = $true
            }
            $resto = $agora.Substring($posicao)
            $acabou = $false
            foreach ($final in $finais) {
                if ($resto.Contains($final + "`n")) { $acabou = $true }
            }
            if ($acabou) { break }
        }
        Start-Sleep -Milliseconds 20
    }
} finally {
    $serial.Close()
}
[Console]::Out.Write($texto.ToString())
