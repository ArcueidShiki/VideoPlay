using System.Runtime.InteropServices;
using NAudio.CoreAudioApi;
using NAudio.CoreAudioApi.Interfaces;
using NAudio.Wasapi.CoreAudioApi.Interfaces;
using NAudio.Wave;

// Windows' supported process-loopback API, restricted to the test player's PID.
// Other desktop applications may keep playing audio during verification.
internal sealed class ProcessLoopbackCapture : IWaveIn
{
    [StructLayout(LayoutKind.Explicit, Size = 24)]
    private struct BlobVariant
    {
        [FieldOffset(0)] public ushort Type;
        [FieldOffset(8)] public int Size;
        [FieldOffset(16)] public nint Data;
    }
    [DllImport("Mmdevapi.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
    private static extern int ActivateAudioInterfaceAsync(string path, in Guid iid, in BlobVariant parameters,
        IActivateAudioInterfaceCompletionHandler completion, out IActivateAudioInterfaceAsyncOperation operation);

    [ComVisible(true), ClassInterface(ClassInterfaceType.None)]
    public sealed class Completion : IActivateAudioInterfaceCompletionHandler, Agile
    {
        internal readonly TaskCompletionSource<IAudioClient> Ready = new(TaskCreationOptions.RunContinuationsAsynchronously);
        public void ActivateCompleted(IActivateAudioInterfaceAsyncOperation operation)
        {
            try { operation.GetActivateResult(out var hr, out var value); Marshal.ThrowExceptionForHR(hr); Ready.TrySetResult((IAudioClient)value); }
            catch (Exception ex) { Ready.TrySetException(ex); }
        }
    }
    [ComImport, Guid("94EA2B94-E9CC-49E0-C0FF-EE64CA8F5B90"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface Agile { }

    private readonly AudioClient client;
    private readonly AutoResetEvent ready = new(false);
    private Thread worker;
    private volatile bool stop;
    public double PacketStartQpcSeconds { get; private set; }
    public WaveFormat WaveFormat { get; set; } = WaveFormat.CreateIeeeFloatWaveFormat(48000, 2);
    public event EventHandler<WaveInEventArgs> DataAvailable;
    public event EventHandler<StoppedEventArgs> RecordingStopped;
    private ProcessLoopbackCapture(IAudioClient value) => client = new AudioClient(value);

    public static async Task<ProcessLoopbackCapture> CreateAsync(uint processId)
    {
        nint data = Marshal.AllocHGlobal(12);
        var handler = new Completion();
        try
        {
            Marshal.WriteInt32(data, 0, 1); // AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK
            Marshal.WriteInt32(data, 4, (int)processId);
            Marshal.WriteInt32(data, 8, 0); // INCLUDE_TARGET_PROCESS_TREE
            var parameters = new BlobVariant { Type = 65, Size = 12, Data = data };
            var iid = typeof(IAudioClient).GUID;
            Marshal.ThrowExceptionForHR(ActivateAudioInterfaceAsync("VAD\\Process_Loopback", iid, parameters, handler, out var operation));
            var result = new ProcessLoopbackCapture(await handler.Ready.Task.WaitAsync(TimeSpan.FromSeconds(10)));
            GC.KeepAlive(operation);
            return result;
        }
        finally { Marshal.FreeHGlobal(data); GC.KeepAlive(handler); }
    }
    public void StartRecording()
    {
        client.Initialize(AudioClientShareMode.Shared, AudioClientStreamFlags.Loopback | AudioClientStreamFlags.EventCallback | AudioClientStreamFlags.AutoConvertPcm,
            0, 0, WaveFormat, Guid.Empty);
        client.SetEventHandle(ready.SafeWaitHandle.DangerousGetHandle());
        client.Start();
        worker = new Thread(() =>
        {
            Exception failure = null;
            try
            {
                var reader = client.AudioCaptureClient;
                while (!stop)
                {
                    ready.WaitOne(100);
                    while (reader.GetNextPacketSize() > 0)
                    {
                        var pointer = reader.GetBuffer(out var frames, out var flags, out _, out var qpc);
                        try
                        {
                            var bytes = new byte[frames * WaveFormat.BlockAlign];
                            if ((flags & AudioClientBufferFlags.Silent) == 0) Marshal.Copy(pointer, bytes, 0, bytes.Length);
                            PacketStartQpcSeconds = qpc / 10000000.0;
                            DataAvailable?.Invoke(this, new WaveInEventArgs(bytes, bytes.Length));
                        }
                        finally { reader.ReleaseBuffer(frames); }
                    }
                }
            }
            catch (Exception ex) { failure = ex; }
            finally { client.Stop(); RecordingStopped?.Invoke(this, new StoppedEventArgs(failure)); }
        }) { IsBackground = true };
        worker.Start();
    }
    public void StopRecording() { stop = true; ready.Set(); }
    public void Dispose() { StopRecording(); worker?.Join(2000); client.Dispose(); ready.Dispose(); }
}
