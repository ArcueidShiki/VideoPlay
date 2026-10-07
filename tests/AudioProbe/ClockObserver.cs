using System.Diagnostics;
using System.IO.Pipes;
using System.Text.Json;

// Read-only correlation with the opt-in test clock. This never drives playback.
internal static class ClockObserver
{
    public static async Task<List<object>> Observe(string pipeName, Stopwatch clock, double seconds)
    {
        if (!pipeName.StartsWith("VideoPlay-test-", StringComparison.Ordinal))
            throw new ArgumentException("Expected the test player's private pipe name.");
        var observations = new List<object>();
        while (clock.Elapsed.TotalSeconds < seconds)
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(2));
            using var pipe = new NamedPipeClientStream(".", pipeName, PipeDirection.InOut, PipeOptions.Asynchronous);
            await pipe.ConnectAsync(timeout.Token);
            using var writer = new StreamWriter(pipe, leaveOpen: true) { AutoFlush = true };
            using var reader = new StreamReader(pipe, leaveOpen: true);
            double before = clock.Elapsed.TotalSeconds;
            await writer.WriteLineAsync("{\"action\":\"state\"}");
            using var json = JsonDocument.Parse(await reader.ReadLineAsync(timeout.Token)
                ?? throw new IOException("The player closed the observation pipe."));
            observations.Add(new { start = before, end = clock.Elapsed.TotalSeconds,
                position = json.RootElement.GetProperty("position").GetDouble(),
                rate = json.RootElement.GetProperty("rate").GetDouble(),
                state = json.RootElement.GetProperty("state").GetString() });
            await Task.Delay(25);
        }
        return observations;
    }
}
