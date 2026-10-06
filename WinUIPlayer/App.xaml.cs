using Microsoft.UI.Xaml;

namespace VideoPlay.WinUI;

public partial class App : Application
{
    private MainWindow? window;
    public App()
    {
        UnhandledException += (_, e) => WriteStartupError(e.Exception);
        InitializeComponent();
    }

    private static void WriteStartupError(Exception error)
    {
        try
        {
            string detail = error.ToString();
            foreach (System.Collections.DictionaryEntry item in error.Data) detail += $"\n{item.Key}: {item.Value}";
            System.IO.File.WriteAllText(System.IO.Path.Combine(System.IO.Path.GetTempPath(), "VideoPlay-startup.log"), detail);
        }
        catch { }
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        try
        {
            window = new MainWindow();
            window.Activate();
        }
        catch (Exception error) { WriteStartupError(error); throw; }
    }
}
