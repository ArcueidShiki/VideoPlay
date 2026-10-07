using System.Diagnostics;
using System.Runtime.InteropServices;
using Windows.Graphics.Capture;
using Windows.Graphics.DirectX;
using Windows.Graphics.DirectX.Direct3D11;
using Windows.Graphics.Imaging;
using Windows.Storage.Streams;

// Windows Graphics Capture is restricted to the explicit test-player HWND/PID.
// Keep the system capture border. Never request borderless capture permission,
// create a monitor capture item, or fall back to desktop/microphone recording.
internal static class WindowCaptureObserver
{
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(nint window, out uint pid);
    [DllImport("d3d11.dll")] static extern int D3D11CreateDevice(nint adapter, int type, nint software, uint flags,
        nint levels, uint levelCount, uint sdk, out nint device, out int level, out nint context);
    [DllImport("d3d11.dll")] static extern int CreateDirect3D11DeviceFromDXGIDevice(nint dxgi, out nint device);
    [DllImport("combase.dll", CharSet = CharSet.Unicode)] static extern int WindowsCreateString(string value, uint length, out nint text);
    [DllImport("combase.dll")] static extern int WindowsDeleteString(nint text);
    [DllImport("combase.dll")] static extern int RoGetActivationFactory(nint name, in Guid iid, out nint factory);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int CreateForWindow(nint factory, nint window, in Guid iid, out nint item);

    public static Task<List<object>> Observe(nint window, uint targetPid, Stopwatch clock, double seconds) => Task.Run(async () =>
    {
        GetWindowThreadProcessId(window, out uint actualPid);
        if (targetPid == 0 || actualPid != targetPid) throw new ArgumentException("Capture HWND must belong to the test player's PID.");
        if (!GraphicsCaptureSession.IsSupported()) throw new NotSupportedException("Windows Graphics Capture unavailable.");
        double origin = Stopwatch.GetTimestamp() / (double)Stopwatch.Frequency - clock.Elapsed.TotalSeconds;
        var iidInterop = new Guid("3628e81b-3cac-4c60-b7f4-23ce0e0c3356");
        var iidItem = new Guid("79c3f95b-31f7-4ec2-a464-632ef5d30760");
        var iidDxgi = new Guid("54ec77fa-1377-44e6-8c32-88fd5f44c84c");
        nint name = 0, factory = 0, itemAbi = 0, d3d = 0, context = 0, dxgi = 0, deviceAbi = 0;
        IDirect3DDevice device = null;
        try
        {
            const string className = "Windows.Graphics.Capture.GraphicsCaptureItem";
            Marshal.ThrowExceptionForHR(WindowsCreateString(className, (uint)className.Length, out name));
            Marshal.ThrowExceptionForHR(RoGetActivationFactory(name, iidInterop, out factory));
            var create = Marshal.GetDelegateForFunctionPointer<CreateForWindow>(Marshal.ReadIntPtr(Marshal.ReadIntPtr(factory), 3 * nint.Size));
            Marshal.ThrowExceptionForHR(create(factory, window, iidItem, out itemAbi));
            var item = WinRT.MarshalInterface<GraphicsCaptureItem>.FromAbi(itemAbi);
            Marshal.ThrowExceptionForHR(D3D11CreateDevice(0, 1, 0, 0x20, 0, 0, 7, out d3d, out _, out context));
            Marshal.ThrowExceptionForHR(Marshal.QueryInterface(d3d, iidDxgi, out dxgi));
            Marshal.ThrowExceptionForHR(CreateDirect3D11DeviceFromDXGIDevice(dxgi, out deviceAbi));
            device = WinRT.MarshalInterface<IDirect3DDevice>.FromAbi(deviceAbi);
            using var pool = Direct3D11CaptureFramePool.CreateFreeThreaded(device, DirectXPixelFormat.B8G8R8A8UIntNormalized, 2, item.Size);
            using var session = pool.CreateCaptureSession(item);
            session.IsCursorCaptureEnabled = false;
            session.StartCapture();
            var observations = new List<object>();
            while (clock.Elapsed.TotalSeconds < seconds)
            {
                using var frame = pool.TryGetNextFrame();
                if (frame is null) { await Task.Delay(2); continue; }
                double captured = frame.SystemRelativeTime.TotalSeconds - origin;
                double received = clock.Elapsed.TotalSeconds;
                using var bitmap = await SoftwareBitmap.CreateCopyFromSurfaceAsync(frame.Surface);
                var bytes = new byte[bitmap.PixelWidth * bitmap.PixelHeight * 4];
                var buffer = new Windows.Storage.Streams.Buffer((uint)bytes.Length);
                bitmap.CopyToBuffer(buffer);
                using var reader = DataReader.FromBuffer(buffer);
                reader.ReadBytes(bytes);
                int offset = (frame.ContentSize.Height / 2 * bitmap.PixelWidth + frame.ContentSize.Width / 2) * 4;
                observations.Add(new { start = captured, end = captured, received,
                    bright = bytes[offset] > 180 && bytes[offset + 1] > 180 && bytes[offset + 2] > 180,
                    pulseId = PulseIdentity.FromRgb(bytes[offset + 2], bytes[offset + 1], bytes[offset]) });
            }
            if (observations.Count == 0) throw new InvalidOperationException("No window frames. Do not accept a permission prompt automatically.");
            return observations;
        }
        finally
        {
            device?.Dispose();
            foreach (var value in new[] { deviceAbi, dxgi, context, d3d, itemAbi, factory }) if (value != 0) Marshal.Release(value);
            if (name != 0) WindowsDeleteString(name);
        }
    });
}
