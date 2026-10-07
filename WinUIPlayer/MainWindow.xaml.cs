using FFmpegInteropX;
using Microsoft.UI;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using System.IO;
using Windows.ApplicationModel.DataTransfer;
using Windows.Media.Playback;
using Microsoft.Windows.Storage.Pickers;
using Windows.System;

namespace VideoPlay.WinUI;

public sealed partial class MainWindow : Window
{
    private MediaPlayer? player;
    // Keep the FFmpeg source alive for the entire playback session.
    private FFmpegMediaSource? source;
    private CancellationTokenSource? opening;
    private int generation;
    private bool closed, pickerOpen, dialogOpen, ended;
    private string? currentLocation;
    private bool currentIsNetwork;
    private string error = "";
    private double preferredVolume = 0.65;
    private bool preferredMute;
    private readonly DispatcherTimer refresh = new() { Interval = TimeSpan.FromMilliseconds(500) };
    private readonly CancellationTokenSource lifetime = new();

    public MainWindow()
    {
        InitializeComponent();
        PlaylistView.ItemsSource = playlist;
        controlsReady = true;
        ProgressSlider.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler((_, _) => scrubbing = true), true);
        ProgressSlider.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler((_, _) => scrubbing = false), true);
        ProgressSlider.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler((_, _) => scrubbing = false), true);
        RefreshControls();
        ExtendsContentIntoTitleBar = true;
        SetTitleBar(TitleBar);
        AppWindow.Resize(new Windows.Graphics.SizeInt32(1120, 760));
        AppWindow.SetIcon(Path.Combine(AppContext.BaseDirectory, "Assets", "AppIcon.ico"));
        AppWindow.TitleBar.ButtonBackgroundColor = Colors.Transparent;
        AppWindow.TitleBar.ButtonForegroundColor = Colors.White;
        AppWindow.TitleBar.ButtonInactiveBackgroundColor = Colors.Transparent;
        AppWindow.TitleBar.ButtonInactiveForegroundColor = Colors.Gray;
        refresh.Tick += (_, _) => RefreshState();
        refresh.Start();
        Closed += (_, _) =>
        {
            closed = true;
            generation++;
            opening?.Cancel();
            refresh.Stop();
            lifetime.Cancel();
            ReleaseMedia();
        };
        Root.Loaded += OnLoaded;
    }

    private async void OnLoaded(object sender, RoutedEventArgs e)
    {
        Root.Loaded -= OnLoaded;
        string[] args = Environment.GetCommandLineArgs().Skip(1).ToArray();
        string? pipe = args.FirstOrDefault(a => a.StartsWith("--test-pipe=", StringComparison.Ordinal));
        if (pipe is not null) _ = TestBridge.RunAsync(this, pipe[12..], lifetime.Token);
        string? file = args.FirstOrDefault(a => !a.StartsWith("--", StringComparison.Ordinal));
        if (file is not null) await OpenMediaAsync(file, false);
    }

    private async void OpenClicked(object sender, RoutedEventArgs e) => await PickFileAsync();
    private async void OpenAccelerator(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
    {
        args.Handled = true;
        await PickFileAsync();
    }

    internal async Task PickFileAsync()
    {
        if (pickerOpen || dialogOpen || closed) return;
        pickerOpen = true;
        OpenButton.IsEnabled = false;
        AddFilesButton.IsEnabled = false;
        PickFileResult? file = null;
        try
        {
            var picker = new FileOpenPicker(AppWindow.Id) { SuggestedStartLocation = PickerLocationId.VideosLibrary };
            picker.FileTypeFilter.Add("*");
            file = await picker.PickSingleFileAsync();
        }
        catch (Exception ex) { ShowError("无法打开文件选择器。也可以将文件拖入窗口。", ex); }
        finally { pickerOpen = false; if (!closed) OpenButton.IsEnabled = AddFilesButton.IsEnabled = true; }
        // The modal guard belongs to the picker, not the potentially slow decoder.
        if (file is not null && !closed) await OpenMediaAsync(file.Path, false);
    }

    internal Task OpenMediaAsync(string location, bool network)
        => ReplacePlaylistAsync(new[] { new PlaylistEntry(location, network) });

    private async Task LoadMediaAsync(string location, bool network)
    {
        if (closed) return;
        int request = ++generation;
        opening?.Cancel();
        opening?.Dispose();
        opening = new CancellationTokenSource(TimeSpan.FromSeconds(20));
        CancellationToken token = opening.Token;
        ReleaseMedia();
        currentLocation = location;
        currentIsNetwork = network;
        RefreshControls();
        error = "";
        ErrorBar.IsOpen = false;
        Welcome.Visibility = Visibility.Collapsed;
        ReplayButton.Visibility = Visibility.Collapsed;
        AudioArtwork.Visibility = Visibility.Collapsed;
        LoadingPanel.Visibility = Visibility.Visible;
        LoadingRing.IsActive = true;
        StatusLabel.Text = "正在打开…";
        MediaTitle.Text = network ? "网络视频" : Path.GetFileName(location);
        MediaSubtitle.Text = network ? SafeNetworkLabel(location) : "本地文件";
        try
        {
            if (!network && !File.Exists(location)) throw new FileNotFoundException("文件不存在，或已被移动。", location);
            if (network && (!Uri.TryCreate(location, UriKind.Absolute, out var uri) ||
                !(uri.Scheme is "http" or "https" or "rtsp")))
                throw new ArgumentException("请输入 http、https 或 rtsp 视频地址。");

            var config = new MediaSourceConfig();
            config.General.MaxSupportedPlaybackRate = 5;
            // Honor the chosen timeline position instead of snapping to a keyframe.
            config.General.FastSeek = false;
            config.Video.VideoDecoderMode = SoftwareDecode.IsChecked
                ? VideoDecoderMode.ForceFFmpegSoftwareDecoder : VideoDecoderMode.Automatic;
            config.FFmpegOptions["rw_timeout"] = "15000000";
            config.FFmpegOptions["timeout"] = "15000000";
            if (network && location.StartsWith("rtsp:", StringComparison.OrdinalIgnoreCase)) config.FFmpegOptions["rtsp_transport"] = "tcp";
            var operation = network
                ? FFmpegMediaSource.CreateFromUriAsync(location, config, AppWindow.Id.Value)
                : FFmpegMediaSource.CreateFromFileAsync(Path.GetFullPath(location), config, AppWindow.Id.Value);
            FFmpegMediaSource next = await operation.AsTask(token);
            if (closed || request != generation) { next.Dispose(); return; }
            source = next;
            // Windows' rate processor can silence mono streams at non-1x rates.
            // Duplicate mono into left/right before playback; preserve other layouts.
            foreach (var audio in next.AudioStreams)
                if (audio.Channels == 1) next.SetFFmpegAudioFilters("pan=stereo|c0=c0|c1=c0", audio);
            var nextPlayer = new MediaPlayer
            {
                AutoPlay = false, AudioCategory = MediaPlayerAudioCategory.Movie,
                Volume = preferredVolume, IsMuted = preferredMute
            };
            player = nextPlayer;
            bool initialRatePending = true;
            nextPlayer.PlaybackSession.PlaybackStateChanged += (_, _) => DispatcherQueue.TryEnqueue(() =>
            {
                if (player != nextPlayer || closed) return;
                // Apply the retained speed when the newly opened session starts.
                if (initialRatePending && nextPlayer.PlaybackSession.PlaybackState == MediaPlaybackState.Playing)
                {
                    initialRatePending = false;
                    ApplyPlaybackRate(preferredRate);
                }
                RefreshState();
            });
            nextPlayer.PlaybackSession.SeekCompleted += (_, _) => DispatcherQueue.TryEnqueue(() =>
            {
                if (player != nextPlayer || closed) return;
                if (nextPlayer.PlaybackSession.Position < nextPlayer.PlaybackSession.NaturalDuration)
                    ClearEndedState();
                RefreshState();
            });
            nextPlayer.MediaOpened += (_, _) => DispatcherQueue.TryEnqueue(() =>
            {
                if (player != nextPlayer || closed) return;
                // FFmpeg's fast-seek/stream-switch logic needs the live session
                // clock; OpenWithMediaPlayerAsync does not attach it for us.
                next.PlaybackSession = nextPlayer.PlaybackSession;
                LoadingPanel.Visibility = Visibility.Collapsed;
                LoadingRing.IsActive = false;
                AudioArtwork.Visibility = nextPlayer.PlaybackSession.NaturalVideoWidth == 0 ? Visibility.Visible : Visibility.Collapsed;
                RefreshState();
            });
            nextPlayer.MediaEnded += (_, _) => DispatcherQueue.TryEnqueue(() =>
            {
                if (player != nextPlayer || closed) return;
                HandleMediaEnded();
            });
            nextPlayer.MediaFailed += (_, args) => DispatcherQueue.TryEnqueue(() =>
            {
                if (player != nextPlayer || closed) return;
                ShowError("文件可能损坏，或解码器无法读取。请尝试“更多选项 → 兼容解码模式”，或打开其他文件。", args.ExtendedErrorCode);
                ReleaseMedia();
            });
            Video.SetMediaPlayer(nextPlayer);
            Video.Visibility = Visibility.Visible;
            await next.OpenWithMediaPlayerAsync(nextPlayer);
            if (closed || request != generation) return;
            nextPlayer.Play();
        }
        catch (OperationCanceledException)
        {
            if (!closed && request == generation) ShowError("打开超时，请检查文件或网络连接后重试。");
        }
        catch (Exception ex)
        {
            if (!closed && request == generation)
            {
                ReleaseMedia();
                ShowError(ex is FileNotFoundException ? "找不到文件，请重新选择。" :
                    ex is ArgumentException ? ex.Message : "无法读取视频。文件可能损坏，或格式无法识别。请尝试其他文件。", ex);
            }
        }
        finally
        {
            if (!closed && request == generation && player is null)
            {
                LoadingPanel.Visibility = Visibility.Collapsed;
                LoadingRing.IsActive = false;
            }
        }
    }

    private static string SafeNetworkLabel(string location) => Uri.TryCreate(location, UriKind.Absolute, out var uri) ? uri.Host : "网络地址";

    private void ReleaseMedia()
    {
        ended = false;
        var oldPlayer = player;
        var oldSource = source;
        if (oldPlayer is not null)
        {
            preferredVolume = oldPlayer.Volume;
            preferredMute = oldPlayer.IsMuted;
        }
        player = null;
        source = null;
        Video.SetMediaPlayer(null);
        Video.Visibility = Visibility.Collapsed;
        AudioArtwork.Visibility = Visibility.Collapsed;
        ReplayButton.Visibility = Visibility.Collapsed;
        if (oldPlayer is not null) { oldPlayer.Pause(); oldPlayer.Source = null; oldPlayer.Dispose(); }
        oldSource?.Dispose();
        RefreshControls();
    }

    private void ShowError(string message, Exception? ex = null)
    {
        if (closed) return;
        error = message;
        ErrorBar.Message = message + (ex is null ? "" : $" (0x{ex.HResult:X8})");
        ErrorBar.IsOpen = true;
        LoadingPanel.Visibility = Visibility.Collapsed;
        LoadingRing.IsActive = false;
        Welcome.Visibility = Visibility.Visible;
        ReplayButton.Visibility = Visibility.Collapsed;
        AudioArtwork.Visibility = Visibility.Collapsed;
        StatusLabel.Text = "无法播放";
        DetailLabel.Text = "可重新选择文件或重试";
    }

    internal void CancelOpen()
    {
        generation++;
        opening?.Cancel();
        ReleaseMedia();
        LoadingPanel.Visibility = Visibility.Collapsed;
        LoadingRing.IsActive = false;
        Welcome.Visibility = Visibility.Visible;
        StatusLabel.Text = "已取消";
    }
    private void CancelClicked(object sender, RoutedEventArgs e) => CancelOpen();
    private async void RetryClicked(object sender, RoutedEventArgs e) { if (currentLocation is not null) await LoadMediaAsync(currentLocation, currentIsNetwork); }
    private async void DecoderChanged(object sender, RoutedEventArgs e) { if (currentLocation is not null) await LoadMediaAsync(currentLocation, currentIsNetwork); }
    private void ReplayClicked(object sender, RoutedEventArgs e) => Play();

    internal void Play()
    {
        if (player is null) return;
        if (ended) { player.PlaybackSession.Position = TimeSpan.Zero; ended = false; }
        ReplayButton.Visibility = Visibility.Collapsed;
        player.Play();
    }
    internal void Pause() => player?.Pause();
    internal void Seek(double seconds)
    {
        if (player?.PlaybackSession.CanSeek != true) return;
        player.PlaybackSession.Position = TimeSpan.FromSeconds(Math.Clamp(seconds, 0, player.PlaybackSession.NaturalDuration.TotalSeconds));
        ended = false;
        ReplayButton.Visibility = Visibility.Collapsed;
    }

    private void RefreshState()
    {
        RefreshControls();
        if (player is null || closed) return;
        var s = player.PlaybackSession;
        // Native transport controls and system media keys also change the player.
        if (s.PlaybackState == MediaPlaybackState.Playing) ClearEndedState();
        StatusLabel.Text = ended ? "播放结束" : s.PlaybackState switch
        {
            MediaPlaybackState.Playing => "正在播放",
            MediaPlaybackState.Paused => "已暂停",
            MediaPlaybackState.Buffering => "正在缓冲…",
            MediaPlaybackState.Opening => "正在打开…",
            _ => "就绪"
        };
        DetailLabel.Text = s.NaturalVideoWidth > 0 ? $"{s.NaturalVideoWidth} × {s.NaturalVideoHeight}  ·  {s.Position:mm\\:ss} / {s.NaturalDuration:mm\\:ss}" : $"音频  ·  {s.Position:mm\\:ss} / {s.NaturalDuration:mm\\:ss}";
    }

    private void ClearEndedState()
    {
        ended = false;
        ReplayButton.Visibility = Visibility.Collapsed;
    }

    private void OnKeyDown(object sender, KeyRoutedEventArgs e)
    {
        if (e.Key == VirtualKey.Escape && ExitFullScreen()) { e.Handled = true; return; }
        if (dialogOpen || pickerOpen || IsInteractiveKeySource(e.OriginalSource as DependencyObject)) return;
        switch (e.Key)
        {
            case VirtualKey.Space:
                if (e.OriginalSource is Button) return;
                if (player?.PlaybackSession.PlaybackState == MediaPlaybackState.Playing) Pause(); else Play();
                e.Handled = true; break;
            case VirtualKey.Left: Seek((player?.PlaybackSession.Position.TotalSeconds ?? 0) - 5); e.Handled = true; break;
            case VirtualKey.Right: Seek((player?.PlaybackSession.Position.TotalSeconds ?? 0) + 5); e.Handled = true; break;
        }
    }
    private bool ExitFullScreen()
    {
        if (dialogOpen || pickerOpen) return false;
        if (AppWindow.Presenter.Kind == AppWindowPresenterKind.FullScreen)
        { AppWindow.SetPresenter(AppWindowPresenterKind.Overlapped); return true; }
        if (Video.IsFullWindow) { Video.IsFullWindow = false; return true; }
        return false;
    }
    private void ExitFullScreenAccelerator(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
        => args.Handled = ExitFullScreen();
    internal void ToggleFullScreen() => AppWindow.SetPresenter(AppWindow.Presenter.Kind == AppWindowPresenterKind.FullScreen ? AppWindowPresenterKind.Overlapped : AppWindowPresenterKind.FullScreen);
    private void FullScreenClicked(object sender, RoutedEventArgs e) => ToggleFullScreen();
    private void FullScreenAccelerator(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args) { ToggleFullScreen(); args.Handled = true; }

    private async void NetworkClicked(object sender, RoutedEventArgs e)
    {
        if (dialogOpen || pickerOpen) return;
        dialogOpen = true;
        var input = new TextBox { PlaceholderText = "https://… 或 rtsp://…", Header = "视频地址", MinWidth = 360 };
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetAutomationId(input, "NetworkAddress");
        var dialog = new ContentDialog { XamlRoot = Root.XamlRoot, RequestedTheme = ElementTheme.Dark, Title = "打开网络视频", Content = input,
            PrimaryButtonText = "播放", CloseButtonText = "取消", DefaultButton = ContentDialogButton.Primary };
        ContentDialogResult result;
        try
        {
            result = await dialog.ShowAsync();
        }
        finally { dialogOpen = false; }
        if (result == ContentDialogResult.Primary && !closed) await OpenMediaAsync(input.Text.Trim(), true);
    }

    private async void AboutClicked(object sender, RoutedEventArgs e)
    {
        if (dialogOpen || pickerOpen) return;
        dialogOpen = true;
        try
        {
            await new ContentDialog { XamlRoot = Root.XamlRoot, RequestedTheme = ElementTheme.Dark, Title = "VideoPlay", CloseButtonText = "知道了",
                Content = "简单、专注的 Windows 播放器。\n\nCtrl+O  打开文件\nCtrl+H  隐藏 / 显示播放控件\n空格  暂停 / 继续\n← / →  后退 / 前进 5 秒\nF11  全屏\nEsc  退出全屏\n\n播放列表按顺序播放，到末尾停止。移除列表项不会删除原文件。\n\n兼容解码模式使用软件解码，适合显卡驱动导致的黑屏。\n\nWinUI 3 · FFmpegInteropX / FFmpeg\n第三方许可见安装目录 ThirdPartyNotices.txt。" }.ShowAsync();
        }
        finally { dialogOpen = false; }
    }

    private void OnDragOver(object sender, DragEventArgs e)
    {
        e.AcceptedOperation = e.DataView.Contains(StandardDataFormats.StorageItems) ? DataPackageOperation.Copy : DataPackageOperation.None;
        e.DragUIOverride.Caption = "播放文件";
    }
    private async void OnDrop(object sender, DragEventArgs e)
    {
        if (!e.DataView.Contains(StandardDataFormats.StorageItems)) return;
        try
        {
            var items = await e.DataView.GetStorageItemsAsync();
            var files = items.OfType<Windows.Storage.StorageFile>().Select(f => new PlaylistEntry(f.Path, false)).ToArray();
            if (files.Length > 0) await ReplacePlaylistAsync(files);
        }
        catch (Exception ex) { ShowError("无法读取拖入的文件，请使用“打开文件”。", ex); }
    }

    internal object Snapshot() => new
    {
        state = ended ? "Ended" : player?.PlaybackSession.PlaybackState.ToString() ?? (error.Length > 0 ? "Error" : "Idle"),
        position = player?.PlaybackSession.Position.TotalSeconds ?? 0,
        duration = player?.PlaybackSession.NaturalDuration.TotalSeconds ?? 0,
        width = player?.PlaybackSession.NaturalVideoWidth ?? 0,
        height = player?.PlaybackSession.NaturalVideoHeight ?? 0,
        audioTracks = source?.PlaybackItem?.AudioTracks.Count ?? 0,
        error, title = MediaTitle.Text, pickerOpen, dialogOpen,
        playbackState = player?.PlaybackSession.PlaybackState.ToString(),
        replayVisible = ReplayButton.Visibility == Visibility.Visible,
        openEnabled = OpenButton.IsEnabled,
        loading = LoadingPanel.Visibility == Visibility.Visible,
        volume = player?.Volume ?? preferredVolume, muted = player?.IsMuted ?? preferredMute,
        rate = player?.PlaybackSession.PlaybackRate ?? preferredRate, preferredRate,
        controlsVisible = TransportPanel.Visibility == Visibility.Visible,
        playlistVisible = PlaylistPanel.Visibility == Visibility.Visible,
        playlist = playlist.Select(p => p.Title).ToArray(), playlistIndex = playlist.IndexOf(currentEntry!),
        videoBounds = ElementBounds(VideoFrame), controlsBounds = ElementBounds(TransportPanel),
        fullScreen = AppWindow.Presenter.Kind == AppWindowPresenterKind.FullScreen,
        software = SoftwareDecode.IsChecked
    };

    internal async Task<object> TestCommandAsync(System.Text.Json.JsonElement command)
    {
        switch (command.GetProperty("action").GetString())
        {
            case "open": await OpenMediaAsync(command.GetProperty("path").GetString()!, command.TryGetProperty("network", out var network) && network.GetBoolean()); break;
            case "startOpen": _ = OpenMediaAsync(command.GetProperty("path").GetString()!, command.TryGetProperty("network", out var n) && n.GetBoolean()); break;
            case "pick": _ = PickFileAsync(); break;
            case "play": Play(); break;
            case "pause": Pause(); break;
            case "seek": Seek(command.GetProperty("seconds").GetDouble()); break;
            case "cancel": CancelOpen(); break;
            case "fullscreen": ToggleFullScreen(); break;
            case "software": SoftwareDecode.IsChecked = command.GetProperty("enabled").GetBoolean(); break;
            case "mute": if (player is not null) player.IsMuted = command.GetProperty("enabled").GetBoolean(); break;
            case "controls": SetControlsVisible(true); break;
            case "close": Close(); break;
        }
        return closed ? new { state = "Closed" } : Snapshot();
    }
}
