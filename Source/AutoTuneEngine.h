#pragma once
// Motor de autotune sin dependencias de JUCE (C++ puro).
//  - Deteccion de tono: YIN (sobre audio decimado, barato en CPU)
//  - Cuantizacion a nota/escala + "retune speed"
//  - Cambio de tono: dos cabezales de lectura con ventana Hann, sincronizados
//    al periodo de la voz (suena mucho mas limpio que un granular fijo)
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

class AutoTuneEngine
{
public:
    static constexpr int kNumScales = 14;
    static constexpr float kLatencyMs = 35.0f;

    static const char* scaleName (int i)
    {
        static const char* names[kNumScales] = {
            "Chromatic", "Major", "Minor", "Harmonic Minor", "Melodic Minor",
            "Pentatonic Major", "Pentatonic Minor", "Blues", "Dorian", "Phrygian",
            "Lydian", "Mixolydian", "Locrian", "Phrygian Dominant" };
        return names[std::clamp (i, 0, kNumScales - 1)];
    }

    static const char* keyName (int i)
    {
        static const char* names[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        return names[((i % 12) + 12) % 12];
    }

    void prepare (double sampleRate, int numChannels)
    {
        sr = sampleRate;
        nCh = std::clamp (numChannels, 1, 2);

        D = std::max (1, (int) std::lround (sr / 12000.0));
        dsr = sr / D;
        tauMin = std::max (2, (int) (dsr / 1000.0));
        tauMax = (int) (dsr / 70.0);
        winLen = 384;
        hopDec = 48;

        dbuf.assign (2048, 0.0f);
        tmp.assign ((size_t) (winLen + tauMax + 2), 0.0f);
        cmnd.assign ((size_t) tauMax + 2, 0.0f);

        int sz = 1; while (sz < 16384 + (int) (sr / 48000.0 * 16384)) sz <<= 1;
        bufMask = sz - 1;
        for (auto& b : buf) b.assign ((size_t) sz, 0.0f);
        maxW = (float) (sz / 2 - 16);

        latency = (int) std::lround (kLatencyMs * 0.001 * sr);
        ratioCoef  = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));   // ~2 ms
        periodCoef = 1.0f - std::exp (-1.0f / (0.060f * (float) sr));   // ~60 ms
        reset();
    }

    // Latencia fija (el audio se retrasa lo que tarda el detector, asi la correccion cae justo encima)
    int getLatencySamples() const { return latency; }
    void setLatencyMs (float ms) { latency = (int) std::lround (ms * 0.001 * sr); }

    void reset()
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        std::fill (dbuf.begin(), dbuf.end(), 0.0f);
        wi = 0; dpos = 0; decAcc = 0; decCount = 0; hopCount = 0;
        phase = 0; ratioSm = 1; ratioTarget = 1;
        periodSm = periodTarget = (float) (sr / 200.0);
        corr = 0; lastTarget = -1000; unvoicedHops = 100;
        hist = { 0, 0, 0 }; histCount = 0;
    }

    void setParams (int keyRoot, int scaleIndex, float retuneMs)
    {
        retune = retuneMs;
        static const int iv[kNumScales][12] = {
            {0,1,2,3,4,5,6,7,8,9,10,11}, {0,2,4,5,7,9,11,-1}, {0,2,3,5,7,8,10,-1},
            {0,2,3,5,7,8,11,-1}, {0,2,3,5,7,9,11,-1}, {0,2,4,7,9,-1}, {0,3,5,7,10,-1},
            {0,3,5,6,7,10,-1}, {0,2,3,5,7,9,10,-1}, {0,1,3,5,7,8,10,-1},
            {0,2,4,6,7,9,11,-1}, {0,2,4,5,7,9,10,-1}, {0,1,3,5,6,8,10,-1}, {0,1,4,5,7,8,10,-1} };
        allowed.fill (false);
        const int s = std::clamp (scaleIndex, 0, kNumScales - 1);
        for (int k = 0; k < 12 && iv[s][k] >= 0; ++k)
            allowed[(size_t) (((keyRoot + iv[s][k]) % 12 + 12) % 12)] = true;
    }

    void process (float* const* ch, int numCh, int numSamples)
    {
        const int c = std::min (numCh, nCh);
        for (int i = 0; i < numSamples; ++i)
        {
            float mono = 0;
            for (int k = 0; k < c; ++k) mono += ch[k][i];
            feedDetector (mono / (float) c);

            ratioSm  += (ratioTarget  - ratioSm)  * ratioCoef;
            periodSm += (periodTarget - periodSm) * periodCoef;
            const float W = std::clamp (4.0f * periodSm, 128.0f, maxW);

            for (int k = 0; k < c; ++k) buf[(size_t) k][(size_t) (wi & bufMask)] = ch[k][i];

            phase += (1.0f - ratioSm) / W;
            phase -= std::floor (phase);
            float p2 = phase + 0.5f; p2 -= std::floor (p2);

            const float w1 = 0.5f - 0.5f * std::cos (6.2831853f * phase);
            const float w2 = 1.0f - w1;
            const float d1 = phase * W + 2.0f + (float) latency;
            const float d2 = p2 * W + 2.0f + (float) latency;

            for (int k = 0; k < c; ++k)
                ch[k][i] = w1 * readHermite (k, d1) + w2 * readHermite (k, d2);

            ++wi;
        }
    }

private:
    float readHermite (int c, float d) const
    {
        const int di = (int) d;
        const float t = d - (float) di;
        const auto& b = buf[(size_t) c];
        const float xm1 = b[(size_t) ((wi - di + 1) & bufMask)];
        const float y0  = b[(size_t) ((wi - di)     & bufMask)];
        const float y1  = b[(size_t) ((wi - di - 1) & bufMask)];
        const float y2  = b[(size_t) ((wi - di - 2) & bufMask)];
        const float c1 = 0.5f * (y1 - xm1);
        const float c2 = xm1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - xm1) + 1.5f * (y0 - y1);
        return ((c3 * t + c2) * t + c1) * t + y0;
    }

    void feedDetector (float x)
    {
        decAcc += x;
        if (++decCount < D) return;
        dbuf[(size_t) (dpos & 2047)] = decAcc / (float) D;
        ++dpos; decAcc = 0; decCount = 0;
        if (++hopCount >= hopDec) { hopCount = 0; analyse(); }
    }

    void analyse()
    {
        const int total = winLen + tauMax;
        const int start = dpos - total;
        for (int j = 0; j < total; ++j) tmp[(size_t) j] = dbuf[(size_t) ((start + j) & 2047)];

        float energy = 0;
        for (int j = total - winLen; j < total; ++j) energy += tmp[(size_t) j] * tmp[(size_t) j];
        const bool loud = std::sqrt (energy / (float) winLen) > 0.004f;

        float tauEst = 0;
        if (loud)
        {
            float running = 0;
            cmnd[0] = 1.0f;
            for (int tau = 1; tau <= tauMax; ++tau)
            {
                float s = 0;
                for (int j = 0; j < winLen; ++j)
                {
                    const float df = tmp[(size_t) j] - tmp[(size_t) (j + tau)];
                    s += df * df;
                }
                running += s;
                cmnd[(size_t) tau] = running > 1e-9f ? s * (float) tau / running : 1.0f;
            }
            for (int tau = tauMin; tau < tauMax; ++tau)
            {
                if (cmnd[(size_t) tau] < 0.15f)
                {
                    while (tau + 1 < tauMax && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                    const float s0 = cmnd[(size_t) tau - 1], s1 = cmnd[(size_t) tau], s2 = cmnd[(size_t) tau + 1];
                    const float den = s0 - 2 * s1 + s2;
                    tauEst = (float) tau + (std::abs (den) > 1e-9f ? 0.5f * (s0 - s2) / den : 0.0f);
                    break;
                }
            }
        }

        const float hopSec = (float) (hopDec * D / sr);

        if (tauEst > 0)
        {
            unvoicedHops = 0;
            const float f0 = (float) (dsr / tauEst);
            periodTarget = tauEst * (float) D;

            float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
            // mediana de 3 para eliminar errores sueltos
            if (histCount == 0) hist = { midi, midi, midi };
            hist = { hist[1], hist[2], midi }; histCount++;
            auto h = hist; std::sort (h.begin(), h.end());
            midi = h[1];

            const float target = pickNote (midi);
            lastTarget = target;
            const float c = target - midi;

            if (retune < 1.0f) corr = c;                                   // modo robot: instantaneo
            else               corr += (c - corr) * (1.0f - std::exp (-hopSec * 1000.0f / retune));
            ratioTarget = std::exp2 (corr / 12.0f);
        }
        else
        {
            ++unvoicedHops; histCount = 0;
            if (unvoicedHops > 3)   // sin voz: volver suavemente a sin correccion
            {
                corr *= std::exp (-hopSec / 0.02f);
                ratioTarget = std::exp2 (corr / 12.0f);
                lastTarget = -1000;
            }
        }
    }

    float pickNote (float m) const
    {
        const int base = (int) std::floor (m);
        float best = (float) base; float bestDist = 1e9f; bool found = false;
        for (int n = base - 6; n <= base + 7; ++n)
        {
            if (! allowed[(size_t) (((n % 12) + 12) % 12)]) continue;
            const float dist = std::abs ((float) n - m);
            if (dist < bestDist) { bestDist = dist; best = (float) n; found = true; }
        }
        if (! found) return m;
        // histeresis: no saltar de nota por ruido en el borde
        const int ln = (int) std::lround (lastTarget);
        if (lastTarget > -999 && allowed[(size_t) (((ln % 12) + 12) % 12)]
            && std::abs (m - lastTarget) <= bestDist + 0.2f)
            return lastTarget;
        return best;
    }

    double sr = 44100, dsr = 11025;
    int latency = 1544;
    int nCh = 2, D = 4, tauMin = 11, tauMax = 157, winLen = 384, hopDec = 48;
    std::vector<float> dbuf, tmp, cmnd;
    std::array<std::vector<float>, 2> buf;
    int bufMask = 0, wi = 0, dpos = 0, decCount = 0, hopCount = 0, unvoicedHops = 100, histCount = 0;
    float decAcc = 0, phase = 0, ratioSm = 1, ratioTarget = 1, periodSm = 220, periodTarget = 220;
    float ratioCoef = 0.01f, periodCoef = 0.0004f, maxW = 8000, corr = 0, lastTarget = -1000, retune = 0;
    std::array<float, 3> hist { 0, 0, 0 };
    std::array<bool, 12> allowed {};
};
