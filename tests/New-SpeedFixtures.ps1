param([Parameter(Mandatory=$true)][string]$FFmpeg,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$CodedOnly)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory $OutputDirectory -Force | Out-Null
if(!$CodedOnly){
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
}

# Give each pulse a unique RGB code and tone frequency. Periodic identical
# pulses cannot distinguish a whole-period A/V delay, particularly at 5x.
$boxes=for($id=1;$id -le 44;$id++){
 $red=48+64*($id%4);$green=48+64*([int][Math]::Floor($id/4)%4);$blue=48+64*[int][Math]::Floor($id/16)
 $color='0x{0:X2}{1:X2}{2:X2}' -f $red,$green,$blue
 "drawbox=x=0:y=0:w=iw:h=ih:color=${color}:t=fill:enable='gte(t,$($id*2))*lt(t,$($id*2+0.6))'"
}
$coded=Join-Path $OutputDirectory 'sync-pulses-coded.mp4'
& $FFmpeg -hide_banner -loglevel error -f lavfi -i 'color=c=black:s=640x360:r=60:d=90' -f lavfi -i "aevalsrc='0.125*sin(2*PI*(400+100*floor(t/2))*t)*gte(t,2)*lt(mod(t,2),0.6)':s=48000:d=90" -vf ($boxes -join ',') -c:v libx264 -preset fast -crf 18 -pix_fmt yuv420p -c:a aac -b:a 192k -movflags +faststart -y $coded
if($LASTEXITCODE){throw 'Identified sync fixture generation failed'}
