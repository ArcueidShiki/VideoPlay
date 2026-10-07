param([Parameter(Mandatory=$true)][string]$FFmpeg,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory $OutputDirectory -Force | Out-Null
# White frame and sample-accurate 523.25-Hz pulse begin together every 2 seconds.
$sync=Join-Path $OutputDirectory 'sync-pulses-exact.mp4'
& $FFmpeg -hide_banner -loglevel error -f lavfi -i 'color=c=0x202020:s=640x360:r=60:d=90' -f lavfi -i "aevalsrc='0.125*sin(2*PI*523.25*t)*lt(mod(t,2),0.6)':s=48000:d=90" -vf "drawbox=x=0:y=0:w=iw:h=ih:color=white:t=fill:enable='lt(mod(t,2),0.6)'" -c:v libx264 -preset fast -crf 20 -pix_fmt yuv420p -c:a aac -b:a 160k -movflags +faststart -y $sync
if($LASTEXITCODE){throw 'Sync fixture generation failed'}
& $FFmpeg -hide_banner -loglevel error -i $sync -c:v copy -an -y (Join-Path $OutputDirectory 'silent-video.mp4')
if($LASTEXITCODE){throw 'Silent fixture generation failed'}
& $FFmpeg -hide_banner -loglevel error -i $sync -c:v copy -c:a pcm_s16le -y (Join-Path $OutputDirectory 'sync-pulses-pcm.mkv')
if($LASTEXITCODE){throw 'PCM fixture generation failed'}
& $FFmpeg -hide_banner -loglevel error -i $sync -c:v copy -ac 2 -c:a aac -b:a 192k -y (Join-Path $OutputDirectory 'sync-stereo.mp4')
if($LASTEXITCODE){throw 'Stereo comparison fixture generation failed'}
