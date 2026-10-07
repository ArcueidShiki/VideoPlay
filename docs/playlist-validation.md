# 2.1 playback controls and playlist validation

**Draft: 0.25x audio/video synchronization remains blocked.** Independent review found that the original identical periodic pulses could alias by one whole period. All original synchronization passes are withdrawn. Their raw WAV/JSON files remain in the local artifacts directory; the uniquely identified pulse results below replace that evidence.

## User-visible behavior

The transport bar occupies its own row below the video. The header toggle and Ctrl+H hide or restore it, including full screen. Volume, speed, audio tracks and subtitles are grouped in flyouts. The optional playlist supports adding multiple local files, independent duplicate entries, selection, removal, previous/next and automatic advance. The last item stops without wrapping. Open replaces the queue; Add preserves current playback. Removing an item never deletes its file.

Speed is retained through pause, seeking and source replacement. Mono tracks are duplicated into left/right channels to work around silence during rate changes on the test machine; other channel layouts remain intact. Accurate seeking avoids keyframe snapping. Escape now has a global accelerator before the interactive-control guard, so focused buttons, toggles, sliders, playlist items and the speed input cannot trap the player in full screen. Modal dialogs and file pickers retain their own Escape behavior.

## Corrected measurement method

Tests use generated fixtures only. `sync-pulses-coded.mp4` gives pulse n a unique RGB base-4 code and tone frequency 400 + 100*n Hz, starting at source time 2*n seconds. Two consecutive captured observations must agree on the identity. Video/audio are paired only by that identity, one-to-one, never by nearest time. The playback clock determines expected coverage only: at least three complete expected pulses and 80% within the unchanged 150-ms tolerance are required. Legacy recordings without identity cannot pass. Six analyzer regressions include the reviewer's 5x / 400-ms whole-period delay, missing coverage, identity reuse and legacy recordings.

`AudioProbe` requires a test PID and verifies that the supplied capture HWND belongs to it. Process-loopback audio includes only that PID and children; there is no desktop-mix or microphone fallback. Audio packets have QPC timestamps. A read-only private pipe samples the media clock for coverage and diagnostic correlation.

Private-desktop tests use `PrintWindow` on that HWND. The visible-desktop comparison also uses Windows Graphics Capture `CreateForWindow`, keeps the system capture border, disables cursor capture and records `SystemRelativeTime` QPC timestamps. Only the synthetic test window is captured. No monitor capture, permission-prompt acceptance or user setting changes are involved. The sandbox lacked the per-user capture service; the same authorized test worked in the desktop user's context.

**Window capture is not physical input-to-photon or speaker-to-display timing.** No external measurement hardware was used. PrintWindow timestamps represent application rendering; Windows Graphics Capture supplies composed-window frame timestamps. See Microsoft's [PrintWindow documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-printwindow), [single-window capture API](https://learn.microsoft.com/en-us/windows/win32/api/windows.graphics.capture.interop/nf-windows-graphics-capture-interop-igraphicscaptureiteminterop-createforwindow), [frame timing guidance](https://learn.microsoft.com/en-us/windows/apps/develop/media-authoring-processing/screen-capture) and [process-loopback sample](https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/ApplicationLoopback).

## Fresh unique-identity results

Windows 11 x64 build 26200; .NET SDK 9.0.318; Windows App SDK 1.8.260921001; FFmpegInteropX 2.1.0.81200; FFmpeg 8.1.2. Both capture methods observe the same player and PID audio in each visible-desktop run. Values are median absolute offsets in milliseconds. Every listed expected pulse was identified in both streams.

| Rate / flow | PrintWindow | Window capture QPC | Within 150 ms, window capture |
| --- | ---: | ---: | --- |
| 0.25x, live change from 1x | 171.2 ms | 178.0 ms | **0/4, fail** |
| 0.5x | 107.5 ms | 103.8 ms | 4/4, pass |
| 0.75x | 96.9 ms | 93.0 ms | 4/4, pass |
| 1x | 26.0 ms | 29.0 ms | 6/6, pass |
| 2x | 47.8 ms | 41.7 ms | 12/12, pass |
| 3x | 36.8 ms | 35.8 ms | 17/17, pass |
| 4x | 31.0 ms | 37.5 ms | 23/23, pass |
| 5x | 19.2 ms | 17.9 ms | 29/29, pass |

The 0.25x window-capture offsets were 162.3, 184.1, 178.0 and 158.1 ms, so the failure survives unique identity and independent window capture. The same run's 1x control passed 5/5 pulses. A separate uniquely identified 1x private-desktop baseline passed 5/5. The rate matrix pauses, changes speed, seeks and resumes between samples to prevent EOF during setup; the 0.25x comparison changes speed while playing without seeking.

The focused-key regression passed 20 checks: F11 entry and Escape exit from Open, control/playlist toggles, Play/Pause, progress, playlist item, nested remove button, volume, speed input and speed slider. The six analyzer counterexample tests pass. Earlier non-synchronization evidence includes 40 installed smoke checks and 17 native-control regressions (six formats, actual audio/mute/pause, cancellation/replacement, stalled network recovery and accurate seeking). Earlier synchronization assertions are superseded regardless of those suite totals.

The fresh interaction run passed all 34 checks, including uniquely identified synchronization after Next, EOF replay and paused-seek/resume at 5x; the raw output is in `coded-interactions-final`. The rebuilt preview installer passed install/uninstall, all 556 published files matched by SHA256, and the installed application passed all 20 focused-key checks. Shortcut/registration cleanup succeeded and a user-added canary survived uninstall. Installer SHA256: `1BEE7EEB3FB45AB2729211986154DFF0E908164B744045D6D3CFC2DCE9C38098`. The helper build succeeded with a local NU1900 warning because NuGet vulnerability metadata was unreachable; audit settings were not disabled.

Reproduction:

```powershell
.\tests\New-MediaFixtures.ps1 -FFmpeg 'C:\tools\ffmpeg.exe' -OutputDirectory '.\TestResults\fixtures'
.\tests\New-SpeedFixtures.ps1 -FFmpeg 'C:\tools\ffmpeg.exe' -OutputDirectory '.\TestResults\speed-fixtures'
dotnet build .\tests\AudioProbe\AudioProbe.csproj -c Release
node --test .\tests\Analyze-Sync.test.mjs
.\tests\WinUI-PlaylistControls.ps1 -AppDirectory '.\dist\win-x64' `
  -ArtifactsDirectory '.\TestResults\quarter' -FixtureDirectory '.\TestResults\fixtures' `
  -SyncFixtureDirectory '.\TestResults\speed-fixtures' -Dotnet (Get-Command dotnet).Source `
  -Rates 1,0.25 -RateOnly -NoSeek -VisibleDesktop -CompareWindowCapture
```

Visible-desktop tests require coordinated use of the test window and a context with Windows capture support. They never switch from window capture to monitor capture. Use `-Rates 0.5,0.75,1,2,3,4,5 -RateOnly -VisibleDesktop -CompareWindowCapture` for the remaining matrix, `-KeyboardOnly` for focused-key checks, `-Rates @()` for interactions, and `-ScreensOnly` for screenshots. Use a policy-permitted PowerShell environment without changing execution policy.

Raw captures and results are under local `artifacts-playlist/coded-window-quarter-final`, `coded-window-rate-matrix` and `coded-private-baseline`. [Compact review evidence](sync-review-evidence.json) retains pulse identities and measured offsets. Original `rate-*` and `clock-*` artifacts are retained as superseded diagnostics.

## Remaining fix scope

No timing offset, tolerance relaxation or replacement playback engine has been introduced. Earlier packet-size and real-time-mode experiments did not establish a fix and were reverted. The application-rendered/composed-window agreement means the current 0.25x failure cannot be dismissed as just private-desktop PrintWindow timing.

The smallest next implementation experiment is at the media-source rate-processing boundary: stretch decoded audio with FFmpeg's [atempo](https://ffmpeg.org/ffmpeg-filters.html#atempo), rescale both audio and video timestamps from the same source timeline, and play the resulting stream at the Windows player's normal rate. This avoids a guessed delay while retaining the WinUI controls and FFmpeg decoding. It requires explicit duration/seek/position mapping and verification of source replacement, EOF, repeated rate changes and track switching; simply applying an audio filter is insufficient. FFmpegInteropX may need a scoped source-timestamp extension. This is a proposed prototype, not an implemented or verified fix. A backend replacement would be a separate, larger decision if that approach cannot meet the contract.

Do not merge as completed 0.25-5x synchronized playback. Independent re-review remains required. Narrator, high-contrast mode, additional audio endpoints, surround output and production RTSP services also require manual coverage.

## Actual application screenshots

| Previous WinUI player | Controls below the video |
| --- | --- |
| ![Previous native transport layout](screenshots/winui-playing.png) | ![External controls](screenshots/external-player.png) |

| Optional playlist | Compact 720 x 520 layout |
| --- | --- |
| ![Playlist](screenshots/external-playlist.png) | ![Compact playlist](screenshots/compact-player.png) |

![Hidden controls with a persistent recovery button](screenshots/controls-hidden-player.png)
