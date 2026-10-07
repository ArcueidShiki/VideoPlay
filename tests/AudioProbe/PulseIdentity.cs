// The coded fixture identifies pulse n by RGB base-4 digits and 400+100*n Hz.
// Never infer identity from the nearest time: a full-period delay must fail.
internal static class PulseIdentity
{
    static readonly Dictionary<int, (double[][] Cos, double[][] Sin)> bases = new();
    public static int? FromRgb(int red, int green, int blue)
    {
        int Digit(int value) => (int)Math.Round((value - 48) / 64.0);
        int r = Digit(red), g = Digit(green), b = Digit(blue);
        if (r is < 0 or > 3 || g is < 0 or > 3 || b is < 0 or > 3 ||
            Math.Abs(red - (48 + r * 64)) > 16 || Math.Abs(green - (48 + g * 64)) > 16 || Math.Abs(blue - (48 + b * 64)) > 16) return null;
        int id = r + 4 * g + 16 * b;
        return id is >= 1 and <= 44 ? id : null;
    }
    public static int? FromAudio(byte[] bytes, int byteCount, int stride, int sampleRate)
    {
        int frames = byteCount / stride;
        if (sampleRate != 48000 || frames < 400) return null;
        if (!bases.TryGetValue(frames, out var basis))
        {
            basis = (new double[44][], new double[44][]);
            for (int id = 1; id <= 44; id++)
            {
                basis.Cos[id - 1] = new double[frames]; basis.Sin[id - 1] = new double[frames];
                for (int i = 0; i < frames; i++)
                {
                    double phase = 2 * Math.PI * (400 + 100 * id) * i / sampleRate;
                    basis.Cos[id - 1][i] = Math.Cos(phase); basis.Sin[id - 1][i] = Math.Sin(phase);
                }
            }
            bases.Add(frames, basis);
        }
        double best = 0, second = 0; int winner = 0;
        for (int id = 1; id <= 44; id++)
        {
            double re = 0, im = 0;
            for (int i = 0; i < frames; i++)
            {
                float value = BitConverter.ToSingle(bytes, i * stride);
                re += value * basis.Cos[id - 1][i]; im += value * basis.Sin[id - 1][i];
            }
            double amplitude = 2 * Math.Sqrt(re * re + im * im) / frames;
            if (amplitude > best) { second = best; best = amplitude; winner = id; }
            else second = Math.Max(second, amplitude);
        }
        return best > 0.012 && best > second * 1.5 ? winner : null;
    }
}
