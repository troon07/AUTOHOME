param([string]$OutputDirectory = 'benchmarks/data/synthetic_speech')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Speech
$taskOutputPath = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDirectory))
[System.IO.Directory]::CreateDirectory($taskOutputPath) | Out-Null
$synthesizer = New-Object System.Speech.Synthesis.SpeechSynthesizer
$format = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(16000, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
$records = @()
try {
    $rooms = @('bedroom','living room','kitchen','study')
    for($device=0; $device -lt 8; $device++) {
        foreach($action in @('on','off')) {
            $commandId = $device*2 + $(if($action -eq 'on'){1}else{2})
            $name = $(if($device % 2){'fan'}else{'light'})
            $phrase = "turn $action $($rooms[[int][Math]::Floor($device/2)]) $name"
            $fileName = 'command_{0:D2}.wav' -f $commandId
            $synthesizer.SetOutputToWaveFile((Join-Path $taskOutputPath $fileName),$format)
            $synthesizer.Speak($phrase)
            $synthesizer.SetOutputToNull()
            $records += [pscustomobject]@{file=$fileName;expected_id=$commandId;phrase=$phrase;audio_source='SIMULATED';speaker=$synthesizer.Voice.Name}
        }
    }
    $records | Export-Csv -LiteralPath (Join-Path $taskOutputPath 'manifest.csv') -NoTypeInformation
    Write-Output "Generated $($records.Count) SIMULATED speech clips. These do not measure human-speaker accuracy."
} finally { $synthesizer.Dispose() }
