param([Parameter(Mandatory=$true)][string]$FFmpeg,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory $OutputDirectory -Force|Out-Null
$mp4=Join-Path $OutputDirectory 'h264-aac.mp4'
& $FFmpeg -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=1280x720:rate=30' -f lavfi -i 'sine=frequency=523.25:sample_rate=48000' -t 12 -c:v libx264 -pix_fmt yuv420p -profile:v high -crf 20 -c:a aac -b:a 160k -movflags +faststart -y $mp4
if($LASTEXITCODE){throw 'H264 fixture encoding failed'}
& $FFmpeg -hide_banner -loglevel error -i $mp4 -c copy -y (Join-Path $OutputDirectory 'h264-aac.mkv')
& $FFmpeg -hide_banner -loglevel error -i $mp4 -c:v libvpx-vp9 -b:v 1M -c:a libopus -y (Join-Path $OutputDirectory 'vp9-opus.webm')
& $FFmpeg -hide_banner -loglevel error -i $mp4 -c:v libx265 -preset fast -crf 24 -tag:v hvc1 -c:a copy -y (Join-Path $OutputDirectory 'hevc-aac.mp4')
& $FFmpeg -hide_banner -loglevel error -i $mp4 -c:v mpeg4 -q:v 4 -c:a pcm_s16le -y (Join-Path $OutputDirectory 'mpeg4-pcm.avi')
& $FFmpeg -hide_banner -loglevel error -i $mp4 -vn -c:a flac -y (Join-Path $OutputDirectory 'audio.flac')
foreach($name in @('h264-aac.mkv','vp9-opus.webm','hevc-aac.mp4','mpeg4-pcm.avi','audio.flac')) {
 if(!(Test-Path (Join-Path $OutputDirectory $name))){throw "Missing fixture: $name"}
}
