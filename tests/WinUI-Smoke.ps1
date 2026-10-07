param(
 [Parameter(Mandatory=$true)][string]$AppDirectory,
 [Parameter(Mandatory=$true)][string]$ArtifactsDirectory,
 [Parameter(Mandatory=$true)][string]$FixtureDirectory,
 [Parameter(Mandatory=$true)][string]$Dotnet,
 [string]$NetworkUrl=''
)
$ErrorActionPreference='Stop'
. "$PSScriptRoot\Initialize-WindowsTests.ps1"
$AppDirectory=[IO.Path]::GetFullPath($AppDirectory)
$ArtifactsDirectory=[IO.Path]::GetFullPath($ArtifactsDirectory)
$FixtureDirectory=[IO.Path]::GetFullPath($FixtureDirectory)
New-Item -ItemType Directory $ArtifactsDirectory -Force | Out-Null
New-Item -ItemType Directory "$ArtifactsDirectory\empty" -Force | Out-Null
$pipeName='VideoPlay-test-'+[Guid]::NewGuid().ToString('N')
$results=New-Object Collections.Generic.List[object]
function Check($name,$ok,$detail=$null) {
 $results.Add([pscustomobject]@{test=$name;passed=[bool]$ok;detail=$detail})
 Write-Host "$ok : $name"
}
function Send-Command($command) {
 $pipe=New-Object IO.Pipes.NamedPipeClientStream('.', $pipeName, [IO.Pipes.PipeDirection]::InOut)
 try {
  $pipe.Connect(30000)
  $w=New-Object IO.StreamWriter($pipe);$w.AutoFlush=$true
  $r=New-Object IO.StreamReader($pipe)
  $w.WriteLine(($command|ConvertTo-Json -Compress))
  $answer=$r.ReadLine() | ConvertFrom-Json
  if($answer.failure){throw $answer.failure}
  $answer
 } finally { $pipe.Dispose() }
}
function State { Send-Command @{action='state'} }
function Wait-State($expected,$timeout=10000) {
 $stop=[Diagnostics.Stopwatch]::StartNew()
 do { $s=State; if($s.state -in $expected){return $s};Start-Sleep -Milliseconds 150 } while($stop.ElapsedMilliseconds -lt $timeout)
 throw "Timed out waiting for $expected : $($s|ConvertTo-Json -Compress)"
}
function Capture($name) { [PlayerWindows]::Screenshot($script:hwnd,"$ArtifactsDirectory\$name.png") }
function Dialog {
 for($i=0;$i -lt 50;$i++) {
  $d=@([PlayerWindows]::Windows($script:pidApp)|Where-Object {[PlayerWindows]::Class($_) -eq '#32770'})
  if($d.Count){return $d[0]};Start-Sleep -Milliseconds 100
 }
 throw 'Native picker did not appear'
}
function Wait-PickerClosed {
 for($i=0;$i -lt 60;$i++) {
  $s=State
  $dialogs=@([PlayerWindows]::Windows($script:pidApp)|Where-Object {[PlayerWindows]::Class($_) -eq '#32770'})
  if(!$s.pickerOpen -and $dialogs.Count -eq 0){return}
  Start-Sleep -Milliseconds 100
 }
 throw 'Picker did not finish closing'
}
function Audio($name) {
 $helper="$PSScriptRoot\AudioProbe\bin\Release\net9.0-windows10.0.19041.0\AudioProbe.dll"
 $raw=& $Dotnet $helper "$ArtifactsDirectory\$name.wav" 2 "--pid=$pidApp"
 if($LASTEXITCODE -ne 0){throw "Audio helper failed: $raw"}
 $raw | ConvertFrom-Json
}
function PixelDifference($first,$second) {
 $a=New-Object Drawing.Bitmap($first);$b=New-Object Drawing.Bitmap($second)
 try {
  $count=0;$changed=0;$bright=0
  for($y=150;$y -lt [Math]::Min($a.Height-120,$b.Height-120);$y+=6) {
   for($x=60;$x -lt [Math]::Min($a.Width-60,$b.Width-60);$x+=6) {
    $p=$a.GetPixel($x,$y);$q=$b.GetPixel($x,$y);$count++
    if(([Math]::Abs($p.R-$q.R)+[Math]::Abs($p.G-$q.G)+[Math]::Abs($p.B-$q.B)) -gt 50){$changed++}
    if(($p.R+$p.G+$p.B) -gt 240){$bright++}
   }
  }
  @{changedRatio=$changed/[double]$count;litRatio=$bright/[double]$count}
 } finally {$a.Dispose();$b.Dispose()}
}
try {
 $pidApp=[PlayerWindows]::Launch("$AppDirectory\VideoPlay.exe", "--test-pipe=$pipeName", "$ArtifactsDirectory\empty")
 $hwnd=[PlayerWindows]::Find($pidApp,'VideoPlay',30000)
 Start-Sleep -Seconds 2
 $s=State;Check 'Independent published layout starts from empty working directory' ($s.state -eq 'Idle') $s
 Capture 'idle'
 Send-Command @{action='pick'}|Out-Null
 $d=Dialog
 Send-Command @{action='pick'}|Out-Null
 Check 'Repeated open creates one picker' (@([PlayerWindows]::Windows($pidApp)|Where-Object {[PlayerWindows]::Class($_) -eq '#32770'}).Count -eq 1)
 [PlayerWindows]::Command($d,2)
 Wait-PickerClosed
 Check 'Cancel picker preserves idle' ((State).state -eq 'Idle')
 $unicode=Join-Path $ArtifactsDirectory ([string][char]0x89C6+[char]0x9891+' sample '+[char]0x4E2D+[char]0x6587+'.mp4')
 Copy-Item "$FixtureDirectory\h264-aac.mp4" $unicode -Force
 Send-Command @{action='pick'}|Out-Null
 $d=Dialog
 Start-Sleep -Milliseconds 500
 $edit=@([PlayerWindows]::Children($d)|Where-Object {[PlayerWindows]::Class($_) -eq 'Edit' -and [PlayerWindows]::GetDlgCtrlID($_) -eq 1148})[0]
 if(!$edit){throw 'Native picker filename field not found'}
 [PlayerWindows]::SetText($edit,$unicode)
 Start-Sleep -Milliseconds 200
 if([PlayerWindows]::ReadControlText($edit) -ne $unicode){throw 'Picker filename did not retain the selected path'}
 [PlayerWindows]::Screenshot($d,"$ArtifactsDirectory\native-file-picker.png")
 [PlayerWindows]::Command($d,1)
 $s=Wait-State @('Playing','Error')
 Check 'Native file picker opens Unicode and spaces / H264 AAC' ($s.state -eq 'Playing' -and $s.width -eq 1280 -and $s.audioTracks -eq 1) $s
 Start-Sleep -Seconds 1;Capture 'h264-frame-a';Start-Sleep -Seconds 2;Capture 'h264-frame-b'
 $pixels=PixelDifference "$ArtifactsDirectory\h264-frame-a.png" "$ArtifactsDirectory\h264-frame-b.png"
 Check 'H264 renders changing nonblack video pixels' ($pixels.changedRatio -gt 0.02 -and $pixels.litRatio -gt 0.3) $pixels
 Send-Command @{action='seek';seconds=0}|Out-Null
 $audio=Audio 'h264-audio'
 Check 'AAC reaches Windows output as the expected 523 Hz tone' ($audio.rms -gt 0.003 -and $audio.tone523Amplitude -gt 0.003) $audio
 Send-Command @{action='mute';enabled=$true}|Out-Null;Start-Sleep -Milliseconds 400
 $silent=Audio 'muted-audio'
 Check 'Mute removes the player tone from actual output' ($silent.tone523Amplitude -lt $audio.tone523Amplitude/10) $silent
 Send-Command @{action='mute';enabled=$false}|Out-Null
 Send-Command @{action='pause'}|Out-Null;$s=Wait-State @('Paused');$position=$s.position
 Start-Sleep -Seconds 1;$s=State
 Check 'Pause stops timeline' ([Math]::Abs($s.position-$position) -lt 0.15) $s
 Send-Command @{action='pick'}|Out-Null;$d=Dialog;[PlayerWindows]::Command($d,2);Wait-PickerClosed
 Check 'Cancel replacement preserves paused media' ((State).state -eq 'Paused')
 Send-Command @{action='seek';seconds=7}|Out-Null;Start-Sleep -Milliseconds 600;$s=State
 Check 'Seek while paused reaches requested time' ([Math]::Abs($s.position-7) -lt 0.5) $s
 Capture 'paused-after-seek'
 Send-Command @{action='play'}|Out-Null;$s=Wait-State @('Playing');Check 'Resume works' ($s.state -eq 'Playing')
 Send-Command @{action='fullscreen'}|Out-Null;Check 'Enter full screen' ((State).fullScreen)
 Send-Command @{action='fullscreen'}|Out-Null;Check 'Exit full screen' (!(State).fullScreen)
 Send-Command @{action='seek';seconds=11}|Out-Null;$s=Wait-State @('Ended')
 Check 'Natural end exposes replay' ($s.state -eq 'Ended');Capture 'ended'
 Send-Command @{action='play'}|Out-Null;$s=Wait-State @('Playing');Check 'Replay restarts from beginning' ($s.position -lt 2) $s
 Send-Command @{action='pause'}|Out-Null;Wait-State @('Paused')|Out-Null
 $paused=Audio 'paused-audio';Check 'Pause silences actual audio' ($paused.tone523Amplitude -lt $audio.tone523Amplitude/10) $paused
 foreach($name in @('h264-aac.mkv','vp9-opus.webm','hevc-aac.mp4','mpeg4-pcm.avi','audio.flac')) {
  if(!(Test-Path "$FixtureDirectory\$name")){continue}
  Send-Command @{action='open';path="$FixtureDirectory\$name"}|Out-Null;$s=Wait-State @('Playing','Error')
  Check "Plays $name" ($s.state -eq 'Playing' -and $s.audioTracks -gt 0) $s
  Start-Sleep -Milliseconds 800;Capture ($name.Replace('.','-')+'-a');Start-Sleep -Seconds 1;Capture ($name.Replace('.','-')+'-b')
  if($name -ne 'audio.flac') {
   $p=PixelDifference "$ArtifactsDirectory\$($name.Replace('.','-'))-a.png" "$ArtifactsDirectory\$($name.Replace('.','-'))-b.png"
   Check "Moving rendered pixels for $name" ($p.changedRatio -gt 0.01 -and $p.litRatio -gt 0.3) $p
  }
 }
 $corrupt="$ArtifactsDirectory\corrupt.mp4";[IO.File]::WriteAllBytes($corrupt,[byte[]](1,7,23,42,15,0))
 Send-Command @{action='open';path=$corrupt}|Out-Null;$s=Wait-State @('Error')
 Check 'Corrupt file produces actionable error' ($s.error.Length -gt 5) $s;Capture 'corrupt-file'
 Send-Command @{action='open';path="$FixtureDirectory\missing.mp4"}|Out-Null
 Check 'Missing file produces error' ((State).state -eq 'Error')
 Send-Command @{action='open';path='not-a-url';network=$true}|Out-Null
 Check 'Invalid network address rejected' ((State).state -eq 'Error')
 Send-Command @{action='software';enabled=$true}|Out-Null
 Send-Command @{action='open';path="$FixtureDirectory\h264-aac.mp4"}|Out-Null;$s=Wait-State @('Playing','Error')
 Check 'Software compatibility mode recovers after errors' ($s.state -eq 'Playing' -and $s.software) $s
 Start-Sleep -Seconds 1;Capture 'software-a';Start-Sleep -Seconds 1;Capture 'software-b'
 $p=PixelDifference "$ArtifactsDirectory\software-a.png" "$ArtifactsDirectory\software-b.png"
 Check 'Software mode produces moving frames' ($p.changedRatio -gt 0.01) $p
 Send-Command @{action='startOpen';path='http://127.0.0.1:1/unreachable.mp4';network=$true}|Out-Null
 Send-Command @{action='cancel'}|Out-Null
 Send-Command @{action='open';path="$FixtureDirectory\h264-aac.mp4"}|Out-Null;$s=Wait-State @('Playing','Error')
 Check 'Interrupted open can immediately recover' ($s.state -eq 'Playing') $s
 for($i=0;$i -lt 8;$i++) {
  Send-Command @{action='startOpen';path="$FixtureDirectory\h264-aac.mp4"}|Out-Null
  Send-Command @{action='startOpen';path="$FixtureDirectory\vp9-opus.webm"}|Out-Null
 }
 $s=Wait-State @('Playing','Error')
 Check 'Rapid replacements retain latest requested source' ($s.state -eq 'Playing' -and $s.title -eq 'vp9-opus.webm') $s
 for($i=0;$i -lt 10;$i++){Send-Command @{action='pause'}|Out-Null;Send-Command @{action='play'}|Out-Null;Send-Command @{action='seek';seconds=2}|Out-Null}
 $s=Wait-State @('Playing','Error');Check 'Repeated pause/resume/seek remains responsive' ($s.state -eq 'Playing') $s
 if($NetworkUrl) {
  Send-Command @{action='open';path=$NetworkUrl;network=$true}|Out-Null;$s=Wait-State @('Playing','Error')
  Check 'Safe local HTTP video playback' ($s.state -eq 'Playing') $s
  Start-Sleep -Seconds 1;Capture 'network-a';Start-Sleep -Seconds 1;Capture 'network-b'
  $p=PixelDifference "$ArtifactsDirectory\network-a.png" "$ArtifactsDirectory\network-b.png"
  Check 'HTTP stream renders moving frames' ($p.changedRatio -gt 0.01) $p
  $stall=$NetworkUrl.Replace('/sample.mp4','/stall')
  Send-Command @{action='startOpen';path=$stall;network=$true}|Out-Null
  Start-Sleep -Milliseconds 500
  Send-Command @{action='cancel'}|Out-Null
  Check 'Cancel interrupts a genuinely stalled network open' ((State).state -eq 'Idle')
  Send-Command @{action='open';path=$stall;network=$true}|Out-Null
  $s=Wait-State @('Error') 25000
  Check 'Stalled network request times out with an error' ($s.error.Length -gt 5) $s
  Send-Command @{action='open';path="$FixtureDirectory\h264-aac.mp4"}|Out-Null
  $s=Wait-State @('Playing','Error')
  Check 'Local playback recovers after stalled network timeout' ($s.state -eq 'Playing') $s
 }
 [PlayerWindows]::MoveWindow($hwnd,80,80,720,520,$true)|Out-Null;Start-Sleep -Seconds 1;Capture 'compact-window'
 Check 'Compact window remains responsive' ((State).state -in @('Playing','Ended'))
 Send-Command @{action='startOpen';path='http://127.0.0.1:1/close.mp4';network=$true}|Out-Null
 [PlayerWindows]::PostMessage($hwnd,0x10,[IntPtr]::Zero,[IntPtr]::Zero)|Out-Null
 Start-Sleep -Seconds 1
 Check 'Close during pending open exits process' (!(Get-Process -Id $pidApp -ErrorAction SilentlyContinue))
} catch {
 Check 'Harness completed' $false $_.Exception.ToString()
 throw
} finally {
 $results|ConvertTo-Json -Depth 8|Set-Content "$ArtifactsDirectory\results.json" -Encoding UTF8
 [PlayerWindows]::Cleanup()
}
if(@($results|Where-Object {!$_.passed}).Count){throw 'Some WinUI checks failed; inspect results.json'}
