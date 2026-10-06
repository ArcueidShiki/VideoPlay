param(
 [Parameter(Mandatory=$true)][string]$AppDirectory,
 [Parameter(Mandatory=$true)][string]$ArtifactsDirectory,
 [Parameter(Mandatory=$true)][string]$FixtureDirectory,
 [string]$Dotnet='',
 [string]$NetworkUrl='',
 [switch]$Baseline
)
$ErrorActionPreference='Stop'
Add-Type -Path "$PSScriptRoot\PlayerWindows.cs" -ReferencedAssemblies System.Drawing
Add-Type -AssemblyName UIAutomationClient,UIAutomationTypes,WindowsBase
Add-Type -Path "$PSScriptRoot\WinUIAccessibility.cs" -ReferencedAssemblies @([System.Windows.Automation.AutomationElement].Assembly.Location,[System.Windows.Automation.AutomationIdentifier].Assembly.Location,[System.Windows.Rect].Assembly.Location)
$AppDirectory=[IO.Path]::GetFullPath($AppDirectory)
$ArtifactsDirectory=[IO.Path]::GetFullPath($ArtifactsDirectory)
$FixtureDirectory=[IO.Path]::GetFullPath($FixtureDirectory)
New-Item -ItemType Directory "$ArtifactsDirectory\empty" -Force|Out-Null
$pipeName='VideoPlay-test-'+[Guid]::NewGuid().ToString('N')
$results=New-Object Collections.Generic.List[object]
$slowPipes=New-Object Collections.Generic.List[object]
function Check($name,$passed,$detail=$null){$results.Add([pscustomobject]@{test=$name;passed=[bool]$passed;detail=$detail});Write-Host "$passed : $name"}
function Command($request){
 $pipe=New-Object IO.Pipes.NamedPipeClientStream('.', $pipeName, [IO.Pipes.PipeDirection]::InOut)
 try{$pipe.Connect(30000);$w=New-Object IO.StreamWriter($pipe);$w.AutoFlush=$true;$r=New-Object IO.StreamReader($pipe);$w.WriteLine(($request|ConvertTo-Json -Compress));$r.ReadLine()|ConvertFrom-Json}finally{$pipe.Dispose()}
}
# Only read state / reveal the native toolbar through the opt-in pipe. All playback,
# mute, volume, open, cancellation and compatibility actions use actual UI providers.
function State { Command @{action='state'} }
function Show-Controls { Command @{action='controls'}|Out-Null }
function Invoke-Control($id){
 for($i=0;$i -lt 50;$i++){
  if([WinUIAccessibility]::Exists($hwnd,[PlayerWindows]::Desktop,$id)){
   [WinUIAccessibility]::Invoke($hwnd,[PlayerWindows]::Desktop,$id);return
  }
  Start-Sleep -Milliseconds 100
 }
 throw "Native control did not appear: $id"
}
function Control-Name($id){[WinUIAccessibility]::Name($hwnd,[PlayerWindows]::Desktop,$id)}
function Wait-State($expected,$timeout=15000){
 $watch=[Diagnostics.Stopwatch]::StartNew()
 do{$s=State;if($s.state -in $expected){return $s};Start-Sleep -Milliseconds 100}while($watch.ElapsedMilliseconds -lt $timeout)
 throw "Expected $expected : $($s|ConvertTo-Json -Compress)"
}
function Pick($path){
 Invoke-Control 'OpenFile'
 $dialog=[IntPtr]::Zero
 for($i=0;$i -lt 60;$i++){
  $all=@([PlayerWindows]::Windows($pidApp)|Where-Object {[PlayerWindows]::Class($_) -eq '#32770'})
  if($all.Count){$dialog=$all[0];break};Start-Sleep -Milliseconds 100
 }
 if($dialog -eq [IntPtr]::Zero){throw 'Picker missing after actual Open button invocation'}
 Start-Sleep -Milliseconds 500
 $edit=@([PlayerWindows]::Children($dialog)|Where-Object {[PlayerWindows]::Class($_) -eq 'Edit' -and [PlayerWindows]::GetDlgCtrlID($_) -eq 1148})[0]
 [PlayerWindows]::SetText($edit,$path)
 Start-Sleep -Milliseconds 150
 [PlayerWindows]::Command($dialog,1)
 # Picker completion and replacement are asynchronous; do not mistake the old
 # source's Playing state for successful playback of the newly chosen file.
 $expected=[IO.Path]::GetFileName($path)
 for($i=0;$i -lt 80;$i++){
  if((State).title -eq $expected -and ![PlayerWindows]::IsWindowVisible($dialog)){return}
  Start-Sleep -Milliseconds 100
 }
 throw "Picker selection did not reach the playback pipeline: $expected"
}
function Capture($name){[PlayerWindows]::Screenshot($hwnd,"$ArtifactsDirectory\$name.png")}
function Audio($name){
 $raw=& $Dotnet "$PSScriptRoot\AudioProbe\bin\Release\net9.0-windows\AudioProbe.dll" "$ArtifactsDirectory\$name.wav" 2
 if($LASTEXITCODE){throw 'WASAPI capture failed'}
 $raw|ConvertFrom-Json
}
function Moving-Pixels($first,$second){
 $a=New-Object Drawing.Bitmap($first);$b=New-Object Drawing.Bitmap($second)
 try{
  $changed=0;$count=0
  for($y=150;$y -lt $a.Height-120;$y+=6){for($x=60;$x -lt $a.Width-60;$x+=6){
   $p=$a.GetPixel($x,$y);$q=$b.GetPixel($x,$y);$count++
   if(([Math]::Abs($p.R-$q.R)+[Math]::Abs($p.G-$q.G)+[Math]::Abs($p.B-$q.B)) -gt 50){$changed++}
  }}
  $changed/[double]$count
 }finally{$a.Dispose();$b.Dispose()}
}
function Start-SlowFile {
 $name='VideoPlay-test-'+[Guid]::NewGuid().ToString('N')+'.ts'
 $pipe=New-Object IO.Pipes.NamedPipeServerStream($name,[IO.Pipes.PipeDirection]::Out,1,[IO.Pipes.PipeTransmissionMode]::Byte,[IO.Pipes.PipeOptions]::Asynchronous)
 $slowPipes.Add($pipe);$waiting=$pipe.WaitForConnectionAsync()
 $path=Join-Path $ArtifactsDirectory ($name+'.m3u8')
 $content="#EXTM3U`n#EXT-X-VERSION:3`n#EXT-X-TARGETDURATION:12`n#EXT-X-MEDIA-SEQUENCE:0`n#EXTINF:12.000,`nfile:\\.\pipe\$name`n#EXT-X-ENDLIST`n"
 [IO.File]::WriteAllText($path,$content,[Text.Encoding]::ASCII)
 Pick $path
 if(!$waiting.Wait(8000)){throw 'FFmpeg did not connect to the genuine slow-file fixture'}
 Start-Sleep -Milliseconds 400
 return $pipe
}
try{
 $pidApp=[PlayerWindows]::Launch("$AppDirectory\VideoPlay.exe","--test-pipe=$pipeName","$ArtifactsDirectory\empty")
 $hwnd=[PlayerWindows]::Find($pidApp,'VideoPlay',30000)
 Start-Sleep -Seconds 2;Wait-State @('Idle')|Out-Null
 Pick "$FixtureDirectory\h264-aac.mp4";Wait-State @('Playing')|Out-Null
 Wait-State @('Ended') 18000|Out-Null
 Show-Controls;Capture 'ended'
 Invoke-Control 'PlayPauseButton'
 Start-Sleep -Seconds 2
 $s=State
 Check 'Native play after EOF clears end state and replay overlay' ($s.state -eq 'Playing' -and !$s.replayVisible) $s
 Capture 'native-replay-a';Start-Sleep -Seconds 1;Capture 'native-replay-b'
 $ratio=Moving-Pixels "$ArtifactsDirectory\native-replay-a.png" "$ArtifactsDirectory\native-replay-b.png"
 Check 'Native replay produces changing rendered video pixels' ($ratio -gt 0.01) @{changedPixelRatio=$ratio}
 $first=$s.position;Start-Sleep -Milliseconds 700;$s=State
 Check 'Native replay advances actual playback position' ($s.position -gt $first+0.2) $s
 # A second natural end followed by an accessible native seek, while paused.
 if(!$Baseline){Wait-State @('Ended') 18000|Out-Null}
 Show-Controls
 [WinUIAccessibility]::SetRangeFraction($hwnd,[PlayerWindows]::Desktop,'ProgressSlider',0.25)
 Start-Sleep -Seconds 1
 $s=State
 Check 'Native seek after EOF reconciles end state' ($s.state -ne 'Ended' -and !$s.replayVisible) $s
 if((Control-Name 'PlayPauseButton') -eq 'Play'){Invoke-Control 'PlayPauseButton'}
 Start-Sleep -Seconds 1
 Show-Controls;Invoke-Control 'VolumeMuteButton'
 [WinUIAccessibility]::SetRange($hwnd,[PlayerWindows]::Desktop,'VolumeSlider',12)
 Invoke-Control 'AudioMuteButton'
 Start-Sleep -Milliseconds 200
 Check 'Native volume and mute controls accepted' ((Control-Name 'AudioMuteButton') -eq 'Unmute' -and [WinUIAccessibility]::RangeValue($hwnd,[PlayerWindows]::Desktop,'VolumeSlider') -eq 12)
 Invoke-Control 'Light Dismiss'
 Pick "$FixtureDirectory\vp9-opus.webm"
 if(!$Baseline){Wait-State @('Playing')|Out-Null}else{Start-Sleep -Seconds 2}
 Show-Controls;Invoke-Control 'VolumeMuteButton'
 $level=[WinUIAccessibility]::RangeValue($hwnd,[PlayerWindows]::Desktop,'VolumeSlider')
 $mute=Control-Name 'AudioMuteButton'
 Check 'Native mute and low volume survive file replacement' ($level -eq 12 -and $mute -eq 'Unmute') @{volume=$level;muteButton=$mute;state=(State)}
 Invoke-Control 'Light Dismiss'
 if($Dotnet -and !$Baseline){$audio=Audio 'muted-after-file';Check 'Actual output stays muted after native file replacement' ($audio.rms -lt 0.0001) $audio}
 Invoke-Control 'MoreOptions';Start-Sleep -Milliseconds 200
 [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'SoftwareDecode')
 Start-Sleep -Seconds 2
 Show-Controls;Invoke-Control 'VolumeMuteButton'
 $level=[WinUIAccessibility]::RangeValue($hwnd,[PlayerWindows]::Desktop,'VolumeSlider')
 $mute=Control-Name 'AudioMuteButton'
 Check 'Native mute and volume survive compatibility reload' ($level -eq 12 -and $mute -eq 'Unmute') @{volume=$level;muteButton=$mute;state=(State)}
 if($Dotnet -and !$Baseline){$audio=Audio 'muted-after-compatibility';Check 'Actual output stays muted after compatibility reload' ($audio.rms -lt 0.0001) $audio}
 Invoke-Control 'AudioMuteButton'
 Invoke-Control 'Light Dismiss'
 if($Dotnet -and !$Baseline){$audio=Audio 'low-volume';Check 'Native unmute retains low output level' ($audio.rms -gt 0.001 -and $audio.rms -lt 0.025) $audio}
 $blocked=Start-SlowFile
 $enabled=[WinUIAccessibility]::Enabled($hwnd,[PlayerWindows]::Desktop,'OpenFile')
 Check 'Picker releases modal guard while real decoder input is blocked' ($enabled -and !(State).pickerOpen) (State)
 Capture 'blocked-open'
 Invoke-Control 'CancelOpen';Start-Sleep -Milliseconds 200
 Check 'Real Cancel button keeps file-open available while old I/O is blocked' ([WinUIAccessibility]::Enabled($hwnd,[PlayerWindows]::Desktop,'OpenFile')) (State)
 if($Baseline){$blocked.Dispose();Start-Sleep -Seconds 1}else{
  Pick "$FixtureDirectory\h264-aac.mp4";$s=Wait-State @('Playing')
  Check 'Actual file picker replaces a canceled source before old I/O returns' ($s.title -eq 'h264-aac.mp4') $s
  $blocked.Dispose();Start-Sleep -Seconds 1
  Check 'Late canceled decoder completion cannot replace or fail current media' ((State).title -eq 'h264-aac.mp4' -and (State).error -eq '') (State)
  $blocked=Start-SlowFile
  Pick "$FixtureDirectory\vp9-opus.webm";$s=Wait-State @('Playing')
  Check 'Actual file picker can replace blocked media without cancel first' ($s.title -eq 'vp9-opus.webm') $s
  $blocked.Dispose();Start-Sleep -Milliseconds 500
  if($NetworkUrl){
   Invoke-Control 'MoreOptions';Invoke-Control 'OpenNetwork';Start-Sleep -Milliseconds 500
   [WinUIAccessibility]::SetValue($hwnd,[PlayerWindows]::Desktop,'NetworkAddress',$NetworkUrl.Replace('/sample.mp4','/stall'))
   Invoke-Control 'PrimaryButton';Start-Sleep -Milliseconds 800
   $s=State;Check 'Network dialog releases its modal guard before stream decoding' (!$s.dialogOpen -and $s.loading) $s
   Pick "$FixtureDirectory\h264-aac.mp4";$s=Wait-State @('Playing')
   Check 'Actual file picker replaces a stalled stream opened through the UI' ($s.title -eq 'h264-aac.mp4') $s
  }
 }
}catch{Check 'Native interaction harness completed' $false $_.Exception.ToString();throw}
finally{
 foreach($pipe in $slowPipes){$pipe.Dispose()}
 $results|ConvertTo-Json -Depth 8|Set-Content "$ArtifactsDirectory\results.json" -Encoding UTF8
 [PlayerWindows]::Cleanup()
}
if(!$Baseline -and @($results|Where-Object {!$_.passed}).Count){throw 'Native interaction regression checks failed'}
