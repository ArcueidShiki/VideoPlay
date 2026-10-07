param(
 [Parameter(Mandatory=$true)][string]$AppDirectory,
 [Parameter(Mandatory=$true)][string]$ArtifactsDirectory,
 [Parameter(Mandatory=$true)][string]$FixtureDirectory,
 [Parameter(Mandatory=$true)][string]$Dotnet,
 [Parameter(Mandatory=$true)][string]$SyncFixtureDirectory,
 [double[]]$Rates=@(0.25,1,2,5),
 [switch]$NoSeek,
 [switch]$PauseForRate,
 [switch]$Software,
 [switch]$RateOnly,
 [switch]$ScreensOnly,
 [double]$StartRate=1,
 [string]$SyncFile="sync-pulses-exact.mp4"
)
$ErrorActionPreference='Stop'
. "$PSScriptRoot\Initialize-WindowsTests.ps1"
$AppDirectory=[IO.Path]::GetFullPath($AppDirectory)
$ArtifactsDirectory=[IO.Path]::GetFullPath($ArtifactsDirectory)
$FixtureDirectory=[IO.Path]::GetFullPath($FixtureDirectory)
$SyncFixtureDirectory=[IO.Path]::GetFullPath($SyncFixtureDirectory)
New-Item -ItemType Directory "$ArtifactsDirectory\empty" -Force|Out-Null
$pipeName='VideoPlay-test-'+[Guid]::NewGuid().ToString('N')
$results=New-Object Collections.Generic.List[object]
function Check($name,$passed,$detail=$null){$results.Add([pscustomobject]@{test=$name;passed=[bool]$passed;detail=$detail});Write-Host "$passed : $name"}
function Command($request){
 $pipe=New-Object IO.Pipes.NamedPipeClientStream('.', $pipeName, [IO.Pipes.PipeDirection]::InOut)
 try{$pipe.Connect(30000);$w=New-Object IO.StreamWriter($pipe);$w.AutoFlush=$true;$r=New-Object IO.StreamReader($pipe);$w.WriteLine(($request|ConvertTo-Json -Compress));$r.ReadLine()|ConvertFrom-Json}finally{$pipe.Dispose()}
}
# The opt-in pipe only observes state; all actions use actual UI providers.
function State { Command @{action='state'} }
function Invoke-Control($id){
 for($i=0;$i -lt 50;$i++){
  if([WinUIAccessibility]::Exists($hwnd,[PlayerWindows]::Desktop,$id)){
   try{[WinUIAccessibility]::Invoke($hwnd,[PlayerWindows]::Desktop,$id)}catch{throw "Invoke $id failed: $($_.Exception.Message)"};return
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
function Capture($name){
 if($ScreensOnly){Start-Sleep -Milliseconds 600}
 [PlayerWindows]::Screenshot($hwnd,"$ArtifactsDirectory\$name.png")
}

function Set-Rate($rate) {
 Invoke-Control 'PlaybackRate'
 [WinUIAccessibility]::SetRange($hwnd,[PlayerWindows]::Desktop,'RateSlider',$rate)
 if([WinUIAccessibility]::Exists($hwnd,[PlayerWindows]::Desktop,'Light Dismiss')){Invoke-Control 'Light Dismiss'}
 Start-Sleep -Milliseconds 400
}

function Wait-Title($title,$timeout=12000) {
 $timer=[Diagnostics.Stopwatch]::StartNew()
 do { $s=State;if($s.title -eq $title -and $s.state -eq 'Playing'){return $s};Start-Sleep -Milliseconds 100 } while($timer.ElapsedMilliseconds -lt $timeout)
 throw "Expected playback of $title : $($s|ConvertTo-Json -Compress)"
}
function Add-Files($paths) {
 $before=(State).playlist.Count
 Invoke-Control 'AddFiles'
 $dialog=[IntPtr]::Zero
 for($i=0;$i -lt 60;$i++) {
  $all=@([PlayerWindows]::Windows($pidApp)|Where-Object {[PlayerWindows]::Class($_) -eq '#32770'})
  if($all.Count){$dialog=$all[0];break};Start-Sleep -Milliseconds 100
 }
 if($dialog -eq [IntPtr]::Zero){throw 'Multi-file picker missing'}
 Start-Sleep -Milliseconds 300
 $edit=@([PlayerWindows]::Children($dialog)|Where-Object {[PlayerWindows]::Class($_) -eq 'Edit' -and [PlayerWindows]::GetDlgCtrlID($_) -eq 1148})[0]
 [PlayerWindows]::SetText($edit,(($paths|ForEach-Object {'"'+$_+'"'}) -join ' '))
 [PlayerWindows]::Command($dialog,1)
 for($i=0;$i -lt 100;$i++){if(!(State).pickerOpen -and (State).playlist.Count -eq $before+$paths.Count){return};Start-Sleep -Milliseconds 100}
 throw 'Multi-file picker did not append requested entries'
}
function Select-Item($index) {[WinUIAccessibility]::SelectIndex($hwnd,[PlayerWindows]::Desktop,'Playlist',$index);Start-Sleep -Milliseconds 300}
function Check-Output($name,[double]$seconds,[bool]$audible,[bool]$synchronized=$false) {
 $raw=& $Dotnet "$PSScriptRoot\AudioProbe\bin\Release\net9.0-windows\AudioProbe.dll" "$ArtifactsDirectory\$name.wav" $seconds ($hwnd.ToInt64().ToString()) ([PlayerWindows]::DesktopName) "--pid=$pidApp"
 if($LASTEXITCODE){throw "Process audio observation failed: $name"}
 $audio=$raw|ConvertFrom-Json
 $valid=if($audible){$audio.rms -gt 0.005}else{$audio.rms -lt 0.0001}
 Check "Actual process audio: $name" $valid $audio
 if($synchronized){
  $sync=& node "$PSScriptRoot\Analyze-Sync.mjs" "$ArtifactsDirectory\$name.sync.json" | ConvertFrom-Json
  Check "Rendered A/V synchronization: $name" $sync.passed $sync
 }
}

try {
 $pidApp=[PlayerWindows]::Launch("$AppDirectory\VideoPlay.exe","--test-pipe=$pipeName","$ArtifactsDirectory\empty")
 $hwnd=[PlayerWindows]::Find($pidApp,'VideoPlay',30000)
 Start-Sleep -Seconds 2
 if($ScreensOnly) {
  Pick "$FixtureDirectory\h264-aac.mp4";Wait-State @('Playing')|Out-Null
  Invoke-Control 'PlayPauseButton';Wait-State @('Paused')|Out-Null
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'TogglePlaylist')
  Add-Files @("$FixtureDirectory\h264-aac.mkv","$FixtureDirectory\vp9-opus.webm")
  Capture 'external-playlist'
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'TogglePlaylist');Capture 'external-player'
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'ToggleControls');Capture 'controls-hidden-player'
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'ToggleControls')
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'TogglePlaylist')
  [PlayerWindows]::MoveWindow($hwnd,80,80,720,520,$true)|Out-Null;Start-Sleep -Milliseconds 400;Capture 'compact-player'
  return
 }
 if($StartRate -ne 1){Set-Rate $StartRate}
 if($Software){Invoke-Control 'MoreOptions';Start-Sleep -Milliseconds 300;[WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'SoftwareDecode')}
 Pick "$SyncFixtureDirectory\$SyncFile";Wait-State @('Playing')|Out-Null
 Capture 'external-controls'
 $rateIteration=0
 foreach($rate in $Rates) {
  $rateIteration++
  # Park the short fixture while operating the flyout; otherwise a preceding
  # high-rate case can reach EOF before the next observation starts.
  if((!$NoSeek -or $PauseForRate) -and (State).state -eq 'Playing') {
   Invoke-Control 'PlayPauseButton';Wait-State @('Paused')|Out-Null
  }
  Set-Rate $rate
  if(!$NoSeek){
   [WinUIAccessibility]::SetRange($hwnd,[PlayerWindows]::Desktop,'ProgressSlider',4)
   Invoke-Control 'PlayPauseButton';Wait-State @('Playing')|Out-Null
  }
  elseif($PauseForRate){Invoke-Control 'PlayPauseButton';Wait-State @('Playing')|Out-Null}
  Start-Sleep -Seconds 1
  $s1=State;$timer=[Diagnostics.Stopwatch]::StartNew();Start-Sleep -Seconds 2;$s2=State;$elapsed=$timer.Elapsed.TotalSeconds
  Check "Clock advances at $rate x" ([Math]::Abs(($s2.position-$s1.position)/$elapsed-$rate) -lt [Math]::Max(0.15,$rate*0.12)) @{rate=$rate;actual=($s2.position-$s1.position)/$elapsed;state=$s2}
  $captureName="rate-$rateIteration-$rate"
  $captureSeconds=[Math]::Max(10,6/$rate)
  $raw=& $Dotnet "$PSScriptRoot\AudioProbe\bin\Release\net9.0-windows\AudioProbe.dll" "$ArtifactsDirectory\$captureName.wav" $captureSeconds ($hwnd.ToInt64().ToString()) ([PlayerWindows]::DesktopName) "--pid=$pidApp" "--pipe=$pipeName"
  if($LASTEXITCODE){throw 'Audio sync observer failed'}
  $audio=$raw|ConvertFrom-Json
  Check "Real audio exists at $rate x" ($audio.rms -gt 0.005) $audio
  $sync=& node "$PSScriptRoot\Analyze-Sync.mjs" "$ArtifactsDirectory\$captureName.sync.json" | ConvertFrom-Json
  Check "Rendered video and process audio stay synchronized at $rate x" $sync.passed $sync
 }

 if(!$RateOnly) {
  Set-Rate 1
  if((State).state -eq 'Playing'){Invoke-Control 'PlayPauseButton'}
  $initial=State
  Check 'Controls occupy a separate row below video' ($initial.videoBounds.y+$initial.videoBounds.height -le $initial.controlsBounds.y) $initial
  for($i=0;$i -lt 3;$i++) {
   [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'ToggleControls');Start-Sleep -Milliseconds 200
   $hidden=State
   Check "Hidden controls return space to video $i" (!$hidden.controlsVisible -and $hidden.videoBounds.height -gt $initial.videoBounds.height -and [WinUIAccessibility]::Exists($hwnd,[PlayerWindows]::Desktop,'ToggleControls')) $hidden
   if($i -eq 0){Capture 'controls-hidden'}
   [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'ToggleControls');Start-Sleep -Milliseconds 200
   Check "Header restores controls $i" ((State).controlsVisible) (State)
  }
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'ToggleControls')
  Invoke-Control 'MoreOptions';Invoke-Control 'FullScreen';Start-Sleep -Milliseconds 400
  Check 'Fullscreen retains the controls recovery entry' ((State).fullScreen -and [WinUIAccessibility]::Exists($hwnd,[PlayerWindows]::Desktop,'ToggleControls')) (State)
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'ToggleControls')
  Invoke-Control 'MoreOptions';Invoke-Control 'FullScreen'
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'TogglePlaylist')
  Add-Files @("$SyncFixtureDirectory\sync-pulses-pcm.mkv","$FixtureDirectory\vp9-opus.webm")
  $s=State
  Check 'Adding multiple local files preserves paused current playback' ($s.playlist.Count -eq 3 -and $s.playlistIndex -eq 0 -and $s.state -eq 'Paused') $s
  [WinUIAccessibility]::InvokeItemChild($hwnd,[PlayerWindows]::Desktop,'Playlist',2,'RemoveEntry');Start-Sleep -Milliseconds 200
  $s=State;Check 'Remove a queued item without interrupting current playback or deleting media' ($s.playlist.Count -eq 2 -and $s.playlistIndex -eq 0 -and $s.state -eq 'Paused' -and (Test-Path "$FixtureDirectory\vp9-opus.webm")) $s
  Add-Files @("$SyncFixtureDirectory\$SyncFile")
  Select-Item 2;$s=Wait-Title $SyncFile
  Check 'Duplicate paths remain independently selectable playlist entries' ($s.playlistIndex -eq 2 -and $s.playlist.Count -eq 3) $s
  Select-Item 1;Wait-Title 'sync-pulses-pcm.mkv'|Out-Null
  Set-Rate 5
  Invoke-Control 'Next';$s=Wait-Title $SyncFile
  Check 'Next changes media and preserves 5x speed' ($s.playlistIndex -eq 2 -and $s.rate -eq 5) $s
  Check-Output 'next-at-5x' 3 $true $true
  Invoke-Control 'Previous';$s=Wait-Title 'sync-pulses-pcm.mkv'
  Check 'Previous changes media and preserves 5x speed' ($s.playlistIndex -eq 1 -and $s.rate -eq 5) $s
  [WinUIAccessibility]::SetRange($hwnd,[PlayerWindows]::Desktop,'ProgressSlider',89)
  $s=Wait-Title $SyncFile
  Check 'EOF automatically advances to the next playlist item' ($s.playlistIndex -eq 2 -and $s.rate -eq 5) $s
  [WinUIAccessibility]::SetRange($hwnd,[PlayerWindows]::Desktop,'ProgressSlider',89)
  $s=Wait-State @('Ended')
  Check 'Last playlist item stops at EOF without wrapping' ($s.playlistIndex -eq 2 -and $s.replayVisible) $s
  Invoke-Control 'PlayPauseButton';$s=Wait-State @('Playing')
  Check 'Real play button replays last item at retained speed' (!$s.replayVisible -and $s.rate -eq 5 -and $s.position -lt 8) $s
  Check-Output 'replay-at-5x' 3 $true $true
  Invoke-Control 'PlayPauseButton';Wait-State @('Paused')|Out-Null
  [WinUIAccessibility]::SetRange($hwnd,[PlayerWindows]::Desktop,'ProgressSlider',20);Start-Sleep -Milliseconds 400
  $first=State;Start-Sleep -Milliseconds 700;$s=State
  Check 'Seek while paused holds position and keeps selected speed' ($s.state -eq 'Paused' -and $s.rate -eq 5 -and [Math]::Abs($s.position-$first.position) -lt 0.1) $s
  Check-Output 'paused-seek-at-5x' 2 $false
  Invoke-Control 'PlayPauseButton';Wait-State @('Playing')|Out-Null
  Check-Output 'resume-after-seek-at-5x' 3 $true $true
  Invoke-Control 'PlayPauseButton';Wait-State @('Paused')|Out-Null
  [PlayerWindows]::MoveWindow($hwnd,80,80,720,520,$true)|Out-Null;Start-Sleep -Milliseconds 400;Capture 'compact-playlist'
  $s=State;Check 'Compact playlist layout keeps controls outside video' ($s.videoBounds.y+$s.videoBounds.height -le $s.controlsBounds.y) $s
  [PlayerWindows]::MoveWindow($hwnd,80,80,1120,760,$true)|Out-Null;Capture 'playlist'
  Invoke-Control 'RemoveItem';$s=State
  Check 'Removing current item stops playback without deleting its file' ($s.state -eq 'Idle' -and $s.playlistIndex -eq -1 -and $s.playlist.Count -eq 2 -and (Test-Path "$SyncFixtureDirectory\$SyncFile")) $s
  Select-Item 0;Wait-Title $SyncFile|Out-Null
  $broken=Join-Path $ArtifactsDirectory 'broken.mp4';[IO.File]::WriteAllText($broken,'invalid media fixture')
  Add-Files @($broken);Select-Item 2;$s=Wait-State @('Error')
  Check 'A failed playlist item stops with an error and retains the queue' ($s.playlist.Count -eq 3 -and $s.playlistIndex -eq 2 -and $s.error) $s
  Select-Item 0;$s=Wait-Title $SyncFile
  Check 'Selecting a valid item recovers from playlist error' ($s.error -eq '') $s
  Pick "$SyncFixtureDirectory\silent-video.mp4";$s=Wait-State @('Playing')
  Check 'Open replaces the list and preserves speed for video without audio' ($s.playlist.Count -eq 1 -and $s.rate -eq 5 -and $s.audioTracks -eq 0) $s
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'TogglePlaylist')
  $raw=& $Dotnet "$PSScriptRoot\AudioProbe\bin\Release\net9.0-windows\AudioProbe.dll" "$ArtifactsDirectory\silent-video.wav" 4 ($hwnd.ToInt64().ToString()) ([PlayerWindows]::DesktopName) "--pid=$pidApp"
  if($LASTEXITCODE){throw 'Silent media capture failed'}
  $audio=$raw|ConvertFrom-Json;$observed=Get-Content "$ArtifactsDirectory\silent-video.sync.json" -Raw|ConvertFrom-Json
  Check 'Silent video renders changing pixels at 5x without creating audio' ($audio.rms -lt 0.0001 -and @($observed.videoObservations|Where-Object bright).Count -gt 0 -and @($observed.videoObservations|Where-Object {!$_.bright}).Count -gt 0) $audio
  Set-Rate 0.25
  Invoke-Control 'PlayPauseButton';Wait-State @('Paused')|Out-Null
  [WinUIAccessibility]::SetRange($hwnd,[PlayerWindows]::Desktop,'ProgressSlider',89.5)
  Invoke-Control 'PlayPauseButton';Wait-State @('Ended') 8000|Out-Null
  Check 'Audio-free video seeks, resumes and reaches EOF at 0.25x' ((State).rate -eq 0.25) (State)
  [WinUIAccessibility]::Toggle($hwnd,[PlayerWindows]::Desktop,'TogglePlaylist');Invoke-Control 'RemoveItem'
  $s=State;Check 'Removing last item yields an empty, stopped player' ($s.playlist.Count -eq 0 -and $s.state -eq 'Idle' -and ![WinUIAccessibility]::Enabled($hwnd,[PlayerWindows]::Desktop,'PlayPauseButton')) $s
  Add-Files @("$FixtureDirectory\h264-aac.mp4","$FixtureDirectory\vp9-opus.webm");$s=Wait-Title 'h264-aac.mp4'
  Check 'Adding to an empty list starts its first item at retained speed' ($s.playlist.Count -eq 2 -and $s.rate -eq 0.25) $s
  Check-Output 'new-file-at-retained-quarter-speed' 3 $true
 }
} catch { Check 'Playback interaction harness completed' $false $_.Exception.ToString();throw }
finally { $results|ConvertTo-Json -Depth 8|Set-Content "$ArtifactsDirectory\results.json" -Encoding UTF8;[PlayerWindows]::Cleanup() }
if(@($results|Where-Object {!$_.passed}).Count){throw 'Playback interaction checks failed; see results.json'}
