using NAudio.Wave;
using System.Text.Json;

try { await RunAsync(args); } catch (Exception ex) { Console.Error.WriteLine(ex); Environment.ExitCode = 1; }

static async Task RunAsync(string[] args)
{
// Capture only the explicitly supplied test player PID and its children.
// Never fall back to the desktop mix or a microphone.
var processArg = args.FirstOrDefault(a => a.StartsWith("--pid="));
var clockPipe = args.FirstOrDefault(a => a.StartsWith("--pipe="))?[7..];
bool compareWindowCapture = args.Contains("--compare-window-capture");
if (processArg is null || !uint.TryParse(processArg[6..], out uint target) || target == 0)
    throw new ArgumentException("Supply --pid=<test-player-process-id>; desktop-wide capture is disabled.");
var sessionDiagnostics = new List<object>();
{
    using var devices = new NAudio.CoreAudioApi.MMDeviceEnumerator();
    foreach (var device in devices.EnumerateAudioEndPoints(NAudio.CoreAudioApi.DataFlow.Render, NAudio.CoreAudioApi.DeviceState.Active))
    {
        using (device)
        {
            var sessions = device.AudioSessionManager.Sessions;
            for (int i = 0; i < sessions.Count; i++)
            {
                var session = sessions[i];
                if (session.GetProcessID == target)
                    sessionDiagnostics.Add(new { state = session.State.ToString(), volume = session.SimpleAudioVolume.Volume,
                        muted = session.SimpleAudioVolume.Mute, peak = session.AudioMeterInformation.MasterPeakValue });
            }
        }
    }
}
args = args.Where(a => !a.StartsWith("--pid=") && !a.StartsWith("--pipe=") && a != "--compare-window-capture").ToArray();
if (args.Length != 2 && args.Length != 4) throw new ArgumentException("Expected output, seconds, and optionally the test window HWND and desktop name.");
if (compareWindowCapture && args.Length != 4) throw new ArgumentException("Window capture requires the explicit test HWND and desktop.");
if (args.Length == 4) VideoObserver.RequireOwner((nint)long.Parse(args[2]), target, args[3]);
using var capture = await ProcessLoopbackCapture.CreateAsync(target);
using var writer = new WaveFileWriter(args[0], capture.WaveFormat);
var data = new List<float>();
var clock = System.Diagnostics.Stopwatch.StartNew();
double origin = System.Diagnostics.Stopwatch.GetTimestamp() / (double)System.Diagnostics.Stopwatch.Frequency - clock.Elapsed.TotalSeconds;
var audioObservations = new List<object>();
capture.DataAvailable += (_, e) =>
{
    writer.Write(e.Buffer, 0, e.BytesRecorded);
    if (capture.WaveFormat.BitsPerSample != 32) throw new InvalidOperationException("Expected float audio mix");
    double power = 0, toneReal = 0, toneImaginary = 0;
    for (int p = 0; p + 4 <= e.BytesRecorded; p += capture.WaveFormat.BlockAlign)
    {
        float value = BitConverter.ToSingle(e.Buffer, p);
        data.Add(value);
        power += value * value;
        double phase = 2 * Math.PI * 523.25 * (p / capture.WaveFormat.BlockAlign) / capture.WaveFormat.SampleRate;
        toneReal += value * Math.Cos(phase);
        toneImaginary += value * Math.Sin(phase);
    }
    double frames = e.BytesRecorded / capture.WaveFormat.BlockAlign;
    double end = capture.PacketStartQpcSeconds - origin + frames / capture.WaveFormat.SampleRate;
    audioObservations.Add(new { start = end - frames / capture.WaveFormat.SampleRate, end, rms = Math.Sqrt(power / Math.Max(1, frames)),
        tone = 2 * Math.Sqrt(toneReal * toneReal + toneImaginary * toneImaginary) / Math.Max(1, frames),
        pulseId = PulseIdentity.FromAudio(e.Buffer, e.BytesRecorded, capture.WaveFormat.BlockAlign, capture.WaveFormat.SampleRate) });
};
var stopped = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
capture.RecordingStopped += (_, e) => { if (e.Exception is not null) stopped.TrySetException(e.Exception); else stopped.TrySetResult(); };
capture.StartRecording();
double seconds = double.Parse(args[1], System.Globalization.CultureInfo.InvariantCulture);
var videoTask = args.Length > 3 ? VideoObserver.Observe((nint)long.Parse(args[2]), clock, seconds, args[3]) : Task.FromResult(new List<object>());
var clockTask = clockPipe is null ? Task.FromResult(new List<object>()) : ClockObserver.Observe(clockPipe, clock, seconds);
var windowTask = compareWindowCapture ? WindowCaptureObserver.Observe((nint)long.Parse(args[2]), target, clock, seconds) : Task.FromResult(new List<object>());
await Task.Delay(TimeSpan.FromSeconds(seconds));
capture.StopRecording();
await stopped.Task;
List<object> videoObservations;
try { videoObservations = await videoTask; }
catch (Exception ex) { Console.Error.WriteLine(ex.Message); Environment.ExitCode = 1; return; }
var clockObservations = await clockTask;
var windowObservations = await windowTask;
if (args.Length > 2) File.WriteAllText(Path.ChangeExtension(args[0], ".sync.json"), JsonSerializer.Serialize(new { audioObservations, videoObservations, clockObservations, windowObservations }));
double power = data.Count == 0 ? 0 : data.Sum(x => (double)x*x)/data.Count;
double re = 0, im = 0;
for (int i = 0; i < data.Count; i++)
{
    double phase = 2*Math.PI*523.25*i/capture.WaveFormat.SampleRate;
    re += data[i]*Math.Cos(phase); im += data[i]*Math.Sin(phase);
}
double tone = data.Count == 0 ? 0 : 2*Math.Sqrt(re*re+im*im)/data.Count;
Console.WriteLine(JsonSerializer.Serialize(new { samples=data.Count, sampleRate=capture.WaveFormat.SampleRate,
    channels=capture.WaveFormat.Channels, rms=Math.Sqrt(power), tone523Amplitude=tone, sessionDiagnostics, path=args[0] }));

}
