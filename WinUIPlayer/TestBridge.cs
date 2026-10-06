using System.IO;
using System.IO.Pipes;
using System.Text.Json;

namespace VideoPlay.WinUI;

// Explicit opt-in local harness. No server is created during normal app startup.
internal static class TestBridge
{
    internal static async Task RunAsync(MainWindow window, string name, CancellationToken cancellation)
    {
        if (!name.StartsWith("VideoPlay-test-", StringComparison.Ordinal) || name.Length > 100) return;
        try
        {
            while (!cancellation.IsCancellationRequested)
            {
                using var pipe = new NamedPipeServerStream(name, PipeDirection.InOut, 1, PipeTransmissionMode.Byte,
                    PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
                await pipe.WaitForConnectionAsync(cancellation).ConfigureAwait(false);
                using var reader = new StreamReader(pipe, leaveOpen: true);
                using var writer = new StreamWriter(pipe, leaveOpen: true) { AutoFlush = true };
                string? line = await reader.ReadLineAsync(cancellation).ConfigureAwait(false);
                if (line is null) continue;
                var result = new TaskCompletionSource<object>(TaskCreationOptions.RunContinuationsAsynchronously);
                using var json = JsonDocument.Parse(line);
                window.DispatcherQueue.TryEnqueue(async () =>
                {
                    try { result.TrySetResult(await window.TestCommandAsync(json.RootElement)); }
                    catch (Exception ex) { result.TrySetResult(new { failure = ex.ToString() }); }
                });
                await writer.WriteLineAsync(JsonSerializer.Serialize(await result.Task.ConfigureAwait(false))).ConfigureAwait(false);
            }
        }
        catch (OperationCanceledException) { }
        catch (IOException) { }
    }
}
