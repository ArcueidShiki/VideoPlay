using System.Collections.ObjectModel;
using System.IO;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.Windows.Storage.Pickers;
using Windows.Media.Playback;
using Windows.System;

namespace VideoPlay.WinUI;

public sealed class PlaylistEntry(string location, bool network)
{
    public string Location { get; } = location;
    public bool Network { get; } = network;
    public string Title => Network ? (Uri.TryCreate(Location, UriKind.Absolute, out var uri) ? uri.Host : "网络媒体") : Path.GetFileName(Location);
}

public sealed partial class MainWindow
{
    private readonly ObservableCollection<PlaylistEntry> playlist = new();
    private PlaylistEntry? currentEntry;
    private bool controlsReady, updatingControls, updatingSelection, scrubbing;
    private double preferredRate = 1;

    private async Task ReplacePlaylistAsync(IEnumerable<PlaylistEntry> entries)
    {
        updatingSelection = true;
        playlist.Clear();
        foreach (var entry in entries) playlist.Add(entry);
        updatingSelection = false;
        if (playlist.Count > 1)
        {
            PlaylistToggle.IsChecked = true;
            PlaylistPanel.Visibility = Visibility.Visible;
        }
        if (playlist.Count > 0) await PlayEntryAsync(playlist[0]);
    }

    private async Task PlayEntryAsync(PlaylistEntry entry)
    {
        if (closed || !playlist.Contains(entry)) return;
        currentEntry = entry;
        updatingSelection = true;
        PlaylistView.SelectedItem = entry;
        updatingSelection = false;
        RefreshControls();
        await LoadMediaAsync(entry.Location, entry.Network);
    }

    private async void PlaylistSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!updatingSelection && PlaylistView.SelectedItem is PlaylistEntry entry)
            await PlayEntryAsync(entry);
    }

    private async void AddFilesClicked(object sender, RoutedEventArgs e)
    {
        if (pickerOpen || dialogOpen || closed) return;
        pickerOpen = true;
        OpenButton.IsEnabled = AddFilesButton.IsEnabled = false;
        var paths = new List<string>();
        try
        {
            var picker = new FileOpenPicker(AppWindow.Id) { SuggestedStartLocation = PickerLocationId.VideosLibrary };
            picker.FileTypeFilter.Add("*");
            var files = await picker.PickMultipleFilesAsync();
            if (files is not null) paths.AddRange(files.Select(f => f.Path));
        }
        catch (Exception ex) { ShowError("无法打开文件选择器，请重试或拖入文件。", ex); }
        finally { pickerOpen = false; if (!closed) OpenButton.IsEnabled = AddFilesButton.IsEnabled = true; }
        if (closed || paths.Count == 0) return;
        var entries = paths.Select(p => new PlaylistEntry(p, false)).ToArray();
        foreach (var entry in entries) playlist.Add(entry);
        RefreshControls();
        if (currentEntry is null) await PlayEntryAsync(entries[0]);
    }

    private void RemoveClicked(object sender, RoutedEventArgs e)
    {
        if (PlaylistView.SelectedItem is not PlaylistEntry entry) return;
        RemoveEntry(entry);
    }
    private void RemoveEntryClicked(object sender, RoutedEventArgs e)
    {
        if (sender is Button { Tag: PlaylistEntry entry }) RemoveEntry(entry);
    }
    private void RemoveEntry(PlaylistEntry entry)
    {
        updatingSelection = true;
        playlist.Remove(entry);
        PlaylistView.SelectedItem = ReferenceEquals(currentEntry, entry) ? null : currentEntry;
        updatingSelection = false;
        if (ReferenceEquals(currentEntry, entry))
        {
            CancelOpen();
            currentEntry = null;
            currentLocation = null;
            error = "";
            ErrorBar.IsOpen = false;
            MediaTitle.Text = playlist.Count == 0 ? "开始播放" : "选择下一项";
            MediaSubtitle.Text = "已从播放列表移除，原文件保持不变";
            StatusLabel.Text = playlist.Count == 0 ? "就绪" : "已停止播放";
            DetailLabel.Text = "Ctrl+H 控制条 · F11 全屏";
        }
        RefreshControls();
    }

    private async void PreviousClicked(object sender, RoutedEventArgs e) => await MovePlaylistAsync(-1);
    private async void NextClicked(object sender, RoutedEventArgs e) => await MovePlaylistAsync(1);
    private async Task MovePlaylistAsync(int step)
    {
        int index = currentEntry is null ? -1 : playlist.IndexOf(currentEntry);
        int target = index + step;
        if (target >= 0 && target < playlist.Count) await PlayEntryAsync(playlist[target]);
    }

    private void HandleMediaEnded()
    {
        int index = currentEntry is null ? -1 : playlist.IndexOf(currentEntry);
        if (index >= 0 && index + 1 < playlist.Count)
        {
            _ = PlayEntryAsync(playlist[index + 1]);
            return;
        }
        ended = true;
        ReplayButton.Visibility = Visibility.Visible;
        StatusLabel.Text = "列表播放完毕";
        RefreshControls();
    }

    private void PlaylistKeyDown(object sender, KeyRoutedEventArgs e)
    {
        if (e.Key == VirtualKey.Delete) { RemoveClicked(sender, new RoutedEventArgs()); e.Handled = true; }
        else if (e.Key == VirtualKey.Enter && PlaylistView.SelectedItem is PlaylistEntry entry)
        { _ = PlayEntryAsync(entry); e.Handled = true; }
    }

    private static bool IsInteractiveKeySource(DependencyObject? source)
    {
        for (var element = source; element is not null; element = VisualTreeHelper.GetParent(element))
            if (element is TextBox or Slider or NumberBox or ButtonBase or ListViewItem or ListView) return true;
        return false;
    }

    private void SetControlsVisible(bool visible)
    {
        ControlsToggle.IsChecked = visible;
        TransportPanel.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
    }
    private void ControlsToggled(object sender, RoutedEventArgs e) => SetControlsVisible(ControlsToggle.IsChecked == true);
    private void ControlsAccelerator(KeyboardAccelerator sender, KeyboardAcceleratorInvokedEventArgs args)
    { SetControlsVisible(TransportPanel.Visibility != Visibility.Visible); args.Handled = true; }
    private void PlaylistToggled(object sender, RoutedEventArgs e)
        => PlaylistPanel.Visibility = PlaylistToggle.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
    private void PlayPauseClicked(object sender, RoutedEventArgs e)
    { if (player?.PlaybackSession.PlaybackState == MediaPlaybackState.Playing && !ended) Pause(); else Play(); }
    private void ProgressChanged(object sender, RangeBaseValueChangedEventArgs e)
    { if (controlsReady && !updatingControls) Seek(e.NewValue); }
    private void VolumeChanged(object sender, RangeBaseValueChangedEventArgs e)
    {
        if (!controlsReady || updatingControls) return;
        preferredVolume = e.NewValue / 100;
        if (player is not null) player.Volume = preferredVolume;
    }
    private void MuteClicked(object sender, RoutedEventArgs e)
    {
        preferredMute = !(player?.IsMuted ?? preferredMute);
        if (player is not null) player.IsMuted = preferredMute;
        RefreshControls();
    }
    private void RateChanged(NumberBox sender, NumberBoxValueChangedEventArgs args)
    { if (controlsReady && !updatingControls) ApplyPlaybackRate(args.NewValue); }
    private void RateSliderChanged(object sender, RangeBaseValueChangedEventArgs e)
    { if (controlsReady && !updatingControls) ApplyPlaybackRate(e.NewValue); }
    private void ResetRateClicked(object sender, RoutedEventArgs e) => ApplyPlaybackRate(1);

    private void ApplyPlaybackRate(double rate)
    {
        if (!double.IsFinite(rate)) { RefreshControls(); return; }
        rate = Math.Clamp(rate, .25, 5);
        try
        {
            if (player is not null)
            {
                player.PlaybackSession.PlaybackRate = rate;
                rate = player.PlaybackSession.PlaybackRate;
            }
            preferredRate = rate;
            RateMessage.Text = "切换文件和暂停后保留此速度";
        }
        catch (Exception)
        {
            preferredRate = player?.PlaybackSession.PlaybackRate ?? 1;
            RateMessage.Text = "当前媒体不支持此速度，已保留实际播放速度。";
        }
        RefreshControls();
    }

    private void RefreshControls()
    {
        if (!controlsReady || closed) return;
        updatingControls = true;
        try
        {
            var session = player?.PlaybackSession;
            bool playing = session?.PlaybackState == MediaPlaybackState.Playing && !ended;
            PlayPauseButton.IsEnabled = player is not null;
            PlayPauseIcon.Glyph = playing ? "\uE769" : "\uE768";
            AutomationProperties.SetName(PlayPauseButton, playing ? "Pause" : "Play");
            ProgressSlider.IsEnabled = session?.CanSeek == true;
            ProgressSlider.Maximum = Math.Max(1, session?.NaturalDuration.TotalSeconds ?? 0);
            if (!scrubbing) ProgressSlider.Value = session?.Position.TotalSeconds ?? 0;
            TimeLabel.Text = $"{FormatTime(session?.Position ?? TimeSpan.Zero)} / {FormatTime(session?.NaturalDuration ?? TimeSpan.Zero)}";
            VolumeSlider.Value = (player?.Volume ?? preferredVolume) * 100;
            bool muted = player?.IsMuted ?? preferredMute;
            VolumeIcon.Glyph = muted ? "\uE74F" : "\uE767";
            AudioMuteButton.Content = muted ? "取消静音" : "静音";
            AutomationProperties.SetName(AudioMuteButton, muted ? "Unmute" : "Mute");
            RateButton.Content = $"{preferredRate:0.##}×";
            RateValue.Value = RateSlider.Value = preferredRate;
            int index = currentEntry is null ? -1 : playlist.IndexOf(currentEntry);
            PreviousButton.IsEnabled = index > 0;
            NextButton.IsEnabled = index + 1 < playlist.Count;
            RemoveButton.IsEnabled = PlaylistView.SelectedItem is not null;
            PlaylistHeading.Text = $"播放列表 · {playlist.Count}";
            TracksButton.IsEnabled = source?.PlaybackItem is not null;
        }
        finally { updatingControls = false; }
    }

    private static string FormatTime(TimeSpan time) => time.TotalHours >= 1 ? time.ToString(@"h\:mm\:ss") : time.ToString(@"mm\:ss");
    private object ElementBounds(FrameworkElement element)
    {
        var point = element.TransformToVisual(Root).TransformPoint(new Windows.Foundation.Point());
        return new { x = point.X, y = point.Y, width = element.ActualWidth, height = element.ActualHeight };
    }

    private void TracksClicked(object sender, RoutedEventArgs e)
    {
        var item = source?.PlaybackItem;
        if (item is null) return;
        var menu = new MenuFlyout();
        for (int i = 0; i < item.AudioTracks.Count; i++)
        {
            int index = i;
            var track = item.AudioTracks[i];
            var option = new ToggleMenuFlyoutItem { Text = $"音轨 {i + 1} {track.Label} {track.Language}", IsChecked = item.AudioTracks.SelectedIndex == i };
            option.Click += (_, _) => { if (source?.PlaybackItem == item) item.AudioTracks.SelectedIndex = index; };
            menu.Items.Add(option);
        }
        menu.Items.Add(new MenuFlyoutSeparator());
        var off = new MenuFlyoutItem { Text = "关闭字幕" };
        off.Click += (_, _) =>
        {
            if (source?.PlaybackItem != item) return;
            for (uint i = 0; i < item.TimedMetadataTracks.Count; i++) item.TimedMetadataTracks.SetPresentationMode(i, TimedMetadataTrackPresentationMode.Disabled);
        };
        menu.Items.Add(off);
        for (int i = 0; i < item.TimedMetadataTracks.Count; i++)
        {
            uint index = (uint)i;
            var track = item.TimedMetadataTracks[i];
            if (track.TimedMetadataKind is not (Windows.Media.Core.TimedMetadataKind.Caption or Windows.Media.Core.TimedMetadataKind.Subtitle)) continue;
            var option = new MenuFlyoutItem { Text = $"字幕 {i + 1} {track.Label} {track.Language}" };
            option.Click += (_, _) =>
            {
                if (source?.PlaybackItem != item) return;
                for (uint j = 0; j < item.TimedMetadataTracks.Count; j++) item.TimedMetadataTracks.SetPresentationMode(j, j == index ? TimedMetadataTrackPresentationMode.PlatformPresented : TimedMetadataTrackPresentationMode.Disabled);
            };
            menu.Items.Add(option);
        }
        menu.ShowAt(TracksButton);
    }
}
