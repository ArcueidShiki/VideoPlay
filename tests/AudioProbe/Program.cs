using NAudio.Wave;
using System.Text.Json;

// Capture the real Windows output mix, then report the known fixture's 523.25 Hz tone.
// Compare playback against the player's mute and pause controls to reject unrelated audio.
using var capture = new WasapiLoopbackCapture();
using var writer = new WaveFileWriter(args[0], capture.WaveFormat);
var data = new List<float>();
capture.DataAvailable += (_, e) =>
{
    writer.Write(e.Buffer, 0, e.BytesRecorded);
    if (capture.WaveFormat.BitsPerSample != 32) throw new InvalidOperationException("Expected float audio mix");
    for (int p = 0; p + 4 <= e.BytesRecorded; p += capture.WaveFormat.BlockAlign)
        data.Add(BitConverter.ToSingle(e.Buffer, p));
};
var stopped = new TaskCompletionSource();
capture.RecordingStopped += (_, _) => stopped.TrySetResult();
capture.StartRecording();
await Task.Delay(TimeSpan.FromSeconds(double.Parse(args[1], System.Globalization.CultureInfo.InvariantCulture)));
capture.StopRecording();
await stopped.Task;
double power = data.Count == 0 ? 0 : data.Sum(x => (double)x*x)/data.Count;
double re = 0, im = 0;
for (int i = 0; i < data.Count; i++)
{
    double phase = 2*Math.PI*523.25*i/capture.WaveFormat.SampleRate;
    re += data[i]*Math.Cos(phase); im += data[i]*Math.Sin(phase);
}
double tone = data.Count == 0 ? 0 : 2*Math.Sqrt(re*re+im*im)/data.Count;
Console.WriteLine(JsonSerializer.Serialize(new { samples=data.Count, sampleRate=capture.WaveFormat.SampleRate,
    channels=capture.WaveFormat.Channels, rms=Math.Sqrt(power), tone523Amplitude=tone, path=args[0] }));
