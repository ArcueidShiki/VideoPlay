using System.Diagnostics;
using System.Runtime.InteropServices;

// Observe displayed pixels and their capture times; never read decoder internals.
internal static class VideoObserver
{
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern nint OpenDesktop(string name, uint flags, bool inherit, uint access);
    [DllImport("user32.dll")] private static extern bool SetThreadDesktop(nint desktop);
    [DllImport("user32.dll")] private static extern bool CloseDesktop(nint desktop);
    [DllImport("user32.dll")] private static extern nint GetThreadDesktop(uint thread);
    [DllImport("kernel32.dll")] private static extern uint GetCurrentThreadId();
    [StructLayout(LayoutKind.Sequential)] private struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] private static extern bool GetWindowRect(nint window, out Rect rect);
    [DllImport("user32.dll")] private static extern nint GetDC(nint window);
    [DllImport("user32.dll")] private static extern int ReleaseDC(nint window, nint dc);
    [DllImport("user32.dll")] private static extern bool PrintWindow(nint window, nint dc, uint flags);
    [DllImport("gdi32.dll")] private static extern nint CreateCompatibleDC(nint dc);
    [DllImport("gdi32.dll")] private static extern nint CreateCompatibleBitmap(nint dc, int width, int height);
    [DllImport("gdi32.dll")] private static extern nint SelectObject(nint dc, nint value);
    [DllImport("gdi32.dll")] private static extern uint GetPixel(nint dc, int x, int y);
    [DllImport("gdi32.dll")] private static extern bool DeleteObject(nint value);
    [DllImport("gdi32.dll")] private static extern bool DeleteDC(nint dc);

    public static Task<List<object>> Observe(nint window, Stopwatch clock, double seconds, string desktopName) => Task.Run(() =>
    {
        var original = GetThreadDesktop(GetCurrentThreadId());
        var desktop = OpenDesktop(desktopName, 0, false, 0x01ff);
        if (desktop == 0 || !SetThreadDesktop(desktop)) throw new InvalidOperationException("Cannot enter test desktop");
        GetWindowRect(window, out var rect);
        var screen = GetDC(0);
        var dc = CreateCompatibleDC(screen);
        var bitmap = CreateCompatibleBitmap(screen, rect.Right - rect.Left, rect.Bottom - rect.Top);
        var old = SelectObject(dc, bitmap);
        ReleaseDC(0, screen);
        var observations = new List<object>();
        try
        {
            while (clock.Elapsed.TotalSeconds < seconds)
            {
                double before = clock.Elapsed.TotalSeconds;
                if (!PrintWindow(window, dc, 2)) throw new InvalidOperationException("PrintWindow failed");
                uint pixel = GetPixel(dc, (rect.Right - rect.Left) / 2, (rect.Bottom - rect.Top) / 2);
                observations.Add(new { start = before, end = clock.Elapsed.TotalSeconds, bright = (pixel & 255) > 180 && ((pixel >> 8) & 255) > 180 && ((pixel >> 16) & 255) > 180 });
                Thread.Sleep(15);
            }
        }
        finally { SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc); SetThreadDesktop(original); CloseDesktop(desktop); }
        return observations;
    });
}
