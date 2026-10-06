# VideoPlay

一个专注于播放的 Windows 桌面播放器，使用真正的 **WinUI 3** 界面、`MediaPlayerElement` 和随应用提供的 **FFmpegInteropX / FFmpeg** 解码器。

## 使用

- 打开文件、拖入文件，或把文件路径作为命令行参数传给 `VideoPlay.exe`。
- 支持中文和包含空格的路径。主界面保持简洁，播放时显示原生播放控件。
- 播放控件提供暂停、进度、音量、字幕/音轨（媒体包含时）和速度设置。
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

生成 `dist\VideoPlay-2.0.0-preview-win-x64-setup.exe`。安装到当前用户目录，无需管理员权限，创建开始菜单快捷方式，可选桌面快捷方式，并提供标准卸载入口。已有无关目录不会被覆盖，卸载仅移除安装器跟踪的文件，保留用户后来加入的文件。预览安装包未进行发布者代码签名。

可用 Visual Studio 打开 `VideoPlay.WinUI.sln`。原有 C++ 项目继续保留，旧构建说明位于 [legacy-configuration.md](docs/legacy-configuration.md)，不参与新播放器发布。

## 验证

`tests\New-MediaFixtures.ps1` 可使用 FFmpeg 生成原创测试图案与 523.25 Hz 音频，包括 H.264/AAC MP4、MKV、VP9/Opus WebM、HEVC、AVI、FLAC。

```powershell
.\tests\New-MediaFixtures.ps1 -FFmpeg 'C:\tools\ffmpeg.exe' -OutputDirectory '.\TestResults\fixtures'
dotnet build .\tests\AudioProbe\AudioProbe.csproj -c Release
powershell.exe -NoProfile -File .\tests\WinUI-Smoke.ps1 `
  -AppDirectory '.\dist\win-x64' -ArtifactsDirectory '.\TestResults\winui' `
  -FixtureDirectory '.\TestResults\fixtures' -Dotnet (Get-Command dotnet).Source
```

测试在独立、非输入桌面启动真实应用，从空工作目录打开原生选择器，检查视频区域变化像素，并通过 WASAPI 回环捕获实际音频输出；覆盖取消、暂停、拖动、重播、损坏文件和连续操作。测试音会短暂从系统默认输出设备播放。`tests\MediaServer.mjs` 提供仅监听本机的 HTTP 视频与停滞连接夹具，可将其 `/sample.mp4` URL 传给 `-NetworkUrl`。

`--test-pipe=VideoPlay-test-<unique-id>` 仅在显式传入时启用当前用户可访问的本地测试通道；正常启动不会创建测试服务器。

已验证的格式和限制记录在 [WinUI 验证说明](docs/winui-validation.md)。第三方许可随安装目录的 `licenses` 和 `ThirdPartyNotices.txt` 提供。
