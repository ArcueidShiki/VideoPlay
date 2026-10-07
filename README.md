# VideoPlay

一个专注于播放的 Windows 桌面播放器，使用真正的 **WinUI 3** 界面、`MediaPlayerElement` 和随应用提供的 **FFmpegInteropX / FFmpeg** 解码器。

## 使用

- 打开文件、拖入文件，或把文件路径作为命令行参数传给 `VideoPlay.exe`。
- 支持中文和包含空格的路径。播放控件位于视频下方，不遮挡画面；顶部按钮或 `Ctrl+H` 可隐藏、恢复控件。
- 播放控件提供暂停、进度、音量、字幕/音轨（媒体包含时）和 0.25–5 倍速设置。
- 当前预览版的 0.25 倍速仍有音画延迟，尚未通过同步验证；见 [2.1 验证记录](docs/playlist-validation.md)。
- 播放列表支持添加多个本地文件、选择播放、移除、上一项和下一项。顺序播放到列表末尾后停止；移除当前项会停止播放，原文件不受影响。打开文件会替换列表，添加文件会保留当前播放。
- “更多选项”提供网络地址、全屏、兼容解码模式和快捷键。
- `Ctrl+O` 打开文件，空格暂停/继续，左右键跳转 5 秒，`F11` 全屏，`Esc` 退出全屏。
- 文件损坏、丢失或打开超时会显示可操作的错误提示，可重试或选择另一个文件。

需要 Windows 10 2004（19041）或更新版本，x64。程序自带 .NET、Windows App SDK、FFmpeg 和 C++ 运行库，无需额外安装 VLC 或系统编解码器包。Windows N/KN 仍需要系统的 Media Feature Pack。

## 构建与安装包

安装 .NET 9 SDK，在 Windows 上运行：

```powershell
.\scripts\Build-WinUI.ps1
```

输出为 `dist\win-x64\VideoPlay.exe`。必须保留整个目录，不能单独复制 EXE。脚本验证必需的 WinUI PRI/XBF 资源及解码器 DLL，并从微软官方固定校验值的包中提取应用本地 C++ 运行库；不会安装系统组件。

使用 Inno Setup 编译器生成安装包：

```powershell
.\scripts\Build-WinUI.ps1 -InnoCompiler 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
```

生成 `dist\VideoPlay-2.1.0-preview-win-x64-setup.exe`。安装到当前用户目录，无需管理员权限，创建开始菜单快捷方式，可选桌面快捷方式，并提供标准卸载入口。已有无关目录不会被覆盖，卸载仅移除安装器跟踪的文件，保留用户后来加入的文件。预览安装包未进行发布者代码签名。倍速同步验证的剩余问题见 [2.1 验证记录](docs/playlist-validation.md)。

可用 Visual Studio 打开 `VideoPlay.WinUI.sln`。原有 C++ 项目继续保留，旧构建说明位于 [legacy-configuration.md](docs/legacy-configuration.md)，不参与新播放器发布。

## 验证

`tests\New-MediaFixtures.ps1` 可使用 FFmpeg 生成原创测试图案与 523.25 Hz 音频，包括 H.264/AAC MP4、MKV、VP9/Opus WebM、HEVC、AVI、FLAC。

```powershell
.\tests\New-MediaFixtures.ps1 -FFmpeg 'C:\tools\ffmpeg.exe' -OutputDirectory '.\TestResults\fixtures'
dotnet build .\tests\AudioProbe\AudioProbe.csproj -c Release
.\tests\WinUI-Smoke.ps1 `
  -AppDirectory '.\dist\win-x64' -ArtifactsDirectory '.\TestResults\winui' `
  -FixtureDirectory '.\TestResults\fixtures' -Dotnet (Get-Command dotnet).Source
```

在支持本地脚本的 PowerShell 中运行测试，无需修改执行策略。测试在独立、非输入桌面启动真实应用，从空工作目录打开原生选择器，检查视频区域变化像素，并通过 Windows 进程回环仅捕获测试播放器 PID 及其子进程的音频；不录制其他应用或麦克风。音频测试需要 Windows build 20348 或更新版本。覆盖取消、暂停、拖动、重播、损坏文件和连续操作。原创测试音会短暂从系统默认输出设备播放。`tests\MediaServer.mjs` 提供仅监听本机的 HTTP 视频与停滞连接夹具，可将其 `/sample.mp4` URL 传给 `-NetworkUrl`。

`--test-pipe=VideoPlay-test-<unique-id>` 仅在显式传入时启用当前用户可访问的本地测试通道；正常启动不会创建测试服务器。

`tests\WinUI-NativeControls.ps1` 使用真实原生控件验证 EOF 后重播/拖动、静音与音量在换文件和切换解码模式后保持，以及慢文件打开期间的取消和替换。参数与上面的测试类似；提供 `-Dotnet` 可同时检查实际声音输出，提供 `-NetworkUrl` 可验证网络对话框取消/替换流程。

已验证的格式和限制记录在 [WinUI 验证说明](docs/winui-validation.md)。第三方许可随安装目录的 `licenses` 和 `ThirdPartyNotices.txt` 提供。
