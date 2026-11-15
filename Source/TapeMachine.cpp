#include "TapeMachine.h"

namespace tapermate_dsp
{
namespace
{
constexpr float kPi = 3.14159265f, kTwoPi = 6.2831853f;

inline float hermite (float x0, float x1, float x2, float x3, float t)
{
    const float c0 = x1, c1 = 0.5f * (x2 - x0), c2 = x0 - 2.5f * x1 + 2.f * x2 - 0.5f * x3, c3 = 0.5f * (x3 - x0) + 1.5f * (x1 - x2);
    return ((c3 * t + c2) * t + c1) * t + c0;
}
inline float dbToLin (float db) { return std::pow (10.f, db / 20.f); }
}

void TapeMachine::Biquad::peak (double fs, double f, double Q, double dB)
{
    const double A = std::pow (10.0, dB / 40.0), w = 2.0 * 3.14159265358979 * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * Q);
    set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
}
void TapeMachine::Biquad::lowpass (double fs, double f, double Q)
{
    const double w = 2.0 * 3.14159265358979 * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * Q);
    set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
}
void TapeMachine::Biquad::highpass (double fs, double f, double Q)
{
    const double w = 2.0 * 3.14159265358979 * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * Q);
    set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
}

float TapeMachine::rnd() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) (int32_t) rng * (1.f / 2147483648.f); }

//==============================================================================
void TapeMachine::prepare (double sampleRate)
{
    fs = sampleRate;
    cap = (size_t) (kTapeSeconds * fs);
    for (auto& t : tape) t.assign (cap, 0.f);
    shW = (int) (0.05 * fs);
    shSize = shW + (int) (0.012 * fs) + 64;
    vibSize = (int) (0.05 * fs) + 64;
    dlySize = (int) (2.6 * fs) + 8;
    for (int c = 0; c < 2; ++c)
    {
        shBuf[c].assign ((size_t) shSize, 0.f);
        vibBuf[c].assign ((size_t) vibSize, 0.f);
        dly[c].assign ((size_t) dlySize, 0.f);
    }
    azBuf.assign (256, 0.f);
    reverb.setSampleRate (fs);
    for (auto& v : vuIn) v.store (0.f);
    for (auto& v : vuOut) v.store (0.f);
    reset();
}

void TapeMachine::reset()
{
    pos = 0.0; recLen = 0.0; motor = 0.f; liveG = 1.f; tapeG = 0.f; pw = 1.f; prevState = 0;
    for (int c = 0; c < 2; ++c)
    {
        std::fill (shBuf[c].begin(), shBuf[c].end(), 0.f);
        std::fill (vibBuf[c].begin(), vibBuf[c].end(), 0.f);
        std::fill (dly[c].begin(), dly[c].end(), 0.f);
        bump[c] = Biquad(); hf[c] = Biquad(); hpf[c] = Biquad(); lpf[c] = Biquad(); mid[c] = Biquad();
        dlLP[c] = dlHP[c] = 0.f;
    }
    shWrite = vibW = dlyW = 0; shPhase = 0.f; mixB = 0.f;
    wowPh = flPh = drift = driftT = 0.f; driftCnt = 0;
    lastSpeed = -1; lastLo = lastHi = -1.f; lastMid = 999.f;
    hissPrev = humPh = 0.f; dropEnv = dropTarget = 1.f; dropCnt = 0;
    for (auto& v : vuAcc) v = 0.f;
    reverb.reset();
}

float TapeMachine::readShift (int c, float d) const
{
    float rp = (float) shWrite - d;
    while (rp < 0.f) rp += (float) shSize;
    int i0 = juce::jmin ((int) rp, shSize - 1);
    const float f = rp - (float) i0;
    const int i1 = i0 + 1 >= shSize ? 0 : i0 + 1;
    return shBuf[c][(size_t) i0] + f * (shBuf[c][(size_t) i1] - shBuf[c][(size_t) i0]);
}

float TapeMachine::readVib (int c, float d) const
{
    float rp = (float) vibW - juce::jlimit (2.f, (float) vibSize - 4.f, d);
    while (rp < 0.f) rp += (float) vibSize;
    int i0 = juce::jmin ((int) rp, vibSize - 1);
    const float f = rp - (float) i0;
    const int i1 = i0 + 1 >= vibSize ? 0 : i0 + 1;
    return vibBuf[c][(size_t) i0] + f * (vibBuf[c][(size_t) i1] - vibBuf[c][(size_t) i0]);
}

void TapeMachine::process (float* L, float* R, int n, const Params& p, std::atomic<int>& transport)
{
    if (clearReq.exchange (false)) { recLen = 0.0; pos = 0.0; }
    if (rewindReq.exchange (false)) pos = 0.0;
    for (int s = 0; s < n; s += 256) run (L + s, R + s, juce::jmin (256, n - s), p, transport);
}

//==============================================================================
void TapeMachine::run (float* L, float* R, int n, const Params& P, std::atomic<int>& tr)
{
    const float sr = (float) fs;
    int state = tr.load();
    if (state != prevState) { if (state == Rec) motor = 1.f; prevState = state; }

    // --- filtros que dependen de la velocidad de cinta y de los extras ---
    if (P.speed != lastSpeed)
    {
        const double bumpHz = P.speed ? 85.0 : 45.0, hfHz = P.speed ? 18000.0 : 10000.0;      // "head bump" y perdida de agudos
        for (int c = 0; c < 2; ++c) { bump[c].peak (fs, bumpHz, 0.9, 1.8); hf[c].lowpass (fs, juce::jmin (hfHz, 0.45 * fs), 0.707); }
        lastSpeed = P.speed;
    }
    if (P.locut != lastLo)  { for (auto& f : hpf) f.highpass (fs, juce::jlimit (10.0, 500.0, (double) P.locut), 0.707); lastLo = P.locut; }
    if (P.hicut != lastHi)  { for (auto& f : lpf) f.lowpass (fs, juce::jlimit (1000.0, 0.45 * fs, (double) P.hicut), 0.707); lastHi = P.hicut; }
    if (P.midDb != lastMid) { for (auto& f : mid) f.peak (fs, 1000.0, 0.8, P.midDb); lastMid = P.midDb; }
    const bool useHp = P.locut > 12.f, useLp = P.hicut < 19500.f, useMid = std::abs (P.midDb) > 0.05f;

    const float inG = dbToLin (P.inDb), outG = dbToLin (P.outDb);
    const float ratioBase = (1.f + P.vari) * std::pow (2.f, (P.pitchSemis + P.fineCents * 0.01f) / 12.f);
    const float speedMul = P.speed == 0 ? 1.8f : 1.f;
    const float lf = P.lfPitch;
    const float wowDev = lf * 0.012f, flDev = (0.0005f + 0.0016f * lf) * speedMul;
    const float wowInc = kTwoPi * 0.55f / sr, flInc = kTwoPi * 7.2f / sr;
    const float dA1 = wowDev / (kTwoPi * 0.55f) * sr, dA2 = flDev / (kTwoPi * 7.2f) * sr, dA3 = 0.0006f * lf * sr;
    const float vibD0 = 0.006f * sr;
    const float hissLvl = std::pow (P.hiss, 1.5f) * 0.04f * (P.speed == 0 ? 1.6f : 1.f);
    const float noiseLvl = P.noise, humAmp = noiseLvl * 0.004f, crackleP = noiseLvl * 2.0e-5f, dropP = noiseLvl * 4.0e-5f;
    const float satK = 1.2f, satB = 0.08f, tb = std::tanh (satB), satNorm = 1.f / (satK * (1.f - tb * tb));
    const float delTarget = juce::jlimit (20.f, 1200.f, P.delayMs) * 0.001f * sr;
    const float fbGain = juce::jlimit (0.f, 0.95f, P.dlyFb);
    const float aDlyLP = 1.f - std::exp (-kTwoPi * (P.speed ? 10000.f : 6000.f) / sr), aDlyHP = 1.f - std::exp (-kTwoPi * 60.f / sr);
    const float coefMix = 1.f - std::exp (-1.f / (0.03f * sr)), coefLive = 1.f - std::exp (-1.f / (0.2f * sr)), coefPw = 1.f - std::exp (-1.f / (0.02f * sr));
    const float coefUp = 1.f - std::exp (-1.f / (0.12f * sr)), coefDn = 1.f - std::exp (-1.f / (0.22f * sr));   // frenado = efecto "tape stop"

    float dryL[256], dryR[256];
    for (int i = 0; i < n; ++i) { dryL[i] = L[i]; dryR[i] = R[i]; }

    auto targetOf = [] (int s) { return s == Play || s == Rec ? 1.f : (s == Rev ? -1.f : (s == FFwd ? 6.f : (s == Rwd ? -6.f : 0.f))); };
    auto isPlayMode = [] (int s) { return s == Play || s == Rev || s == FFwd || s == Rwd; };
    float motorTarget = targetOf (state);
    auto stopNow = [&] { tr.store (Stop); state = Stop; prevState = Stop; motorTarget = 0.f; };

    double inSq[2] { 0.0, 0.0 };

    for (int i = 0; i < n; ++i)
    {
        // ---- movimiento irregular de la cinta (LF PITCH): wow, flutter, deriva ----
        wowPh += wowInc; if (wowPh > kPi) wowPh -= kTwoPi;
        flPh += flInc;   if (flPh > kPi) flPh -= kTwoPi;
        if (--driftCnt <= 0) { driftT = rnd(); driftCnt = (int) (0.5f * sr); }
        drift += (driftT - drift) * 0.0003f;
        const float sW = std::sin (wowPh), sF = std::sin (flPh);
        const float dev = wowDev * sW + flDev * sF + 0.005f * lf * drift;

        // ---- cabezal de grabacion: INPUT -> saturacion (SAT MIX) -> respuesta segun velocidad ----
        float rec[2], x;
        for (int c = 0; c < 2; ++c)
        {
            x = (c == 0 ? dryL[i] : dryR[i]) * inG;
            inSq[c] += (double) x * x;
            const float sat = (std::tanh (satK * x + satB) - tb) * satNorm;
            float r = x + (sat - x) * P.satMix;
            r = hf[c].run (bump[c].run (r));
            rec[c] = r;
        }

        // ---- grabar (REC) ----
        if (state == Rec)
        {
            const size_t ip = (size_t) pos;
            if (ip < cap)
            {
                tape[0][ip] = rec[0]; tape[1][ip] = rec[1];
                if (pos + 1.0 > recLen) recLen = pos + 1.0;
            }
            pos += 1.0;
            if (pos >= (double) cap - 1.0) { pos = (double) cap - 1.0; stopNow(); }
        }

        // ---- motor ----
        if (state == Rec) motor = 1.f;
        else motor += (motorTarget - motor) * (std::abs (motorTarget) < std::abs (motor) ? coefDn : coefUp);

        const float tapeT = isPlayMode (state) ? 1.f : (state == Stop ? juce::jmin (1.f, std::abs (motor) * 2.f) : 0.f);
        tapeG += (tapeT - tapeG) * coefLive;
        liveG += (((state == Stop || state == Rec) ? 1.f : 0.f) - liveG) * coefLive;

        // ---- fuente 1: entrada en vivo (cambio de velocidad + vibrato de wow/flutter) ----
        float live[2] { 0.f, 0.f };
        if (liveG > 0.0005f)
        {
            mixB += ((std::abs (ratioBase - 1.f) > 0.0015f ? 1.f : 0.f) - mixB) * coefMix;
            shPhase += (1.f - ratioBase) / (float) shW;
            shPhase -= std::floor (shPhase);
            float p2 = shPhase + 0.5f; p2 -= std::floor (p2);
            const float d1 = shPhase * (float) shW + 2.f, d2 = p2 * (float) shW + 2.f;
            const float s1 = std::sin (kPi * shPhase), s2 = std::sin (kPi * p2), g1 = s1 * s1, g2 = s2 * s2;
            for (int c = 0; c < 2; ++c)
            {
                shBuf[c][(size_t) shWrite] = rec[c];
                const float a = readShift (c, 0.005f * sr);
                float y = a;
                if (mixB > 0.0005f) y = a + mixB * ((g1 * readShift (c, d1) + g2 * readShift (c, d2)) - a);
                vibBuf[c][(size_t) vibW] = y;
                live[c] = readVib (c, vibD0 + dA1 * sW + dA2 * sF + dA3 * drift);
            }
            if (++shWrite >= shSize) shWrite = 0;
            if (++vibW >= vibSize) vibW = 0;
        }

        // ---- fuente 2: la cinta grabada (PLAY / REV / F.FWD / RWD) ----
        float tp[2] { 0.f, 0.f };
        if (tapeG > 0.0005f && recLen > 4.0)
        {
            const int last = (int) recLen - 1, ip = (int) pos;
            const float t = (float) (pos - (double) ip);
            for (int c = 0; c < 2; ++c)
            {
                auto at = [&] (int k) { return tape[c][(size_t) juce::jlimit (0, last, k)]; };
                tp[c] = hermite (at (ip - 1), at (ip), at (ip + 1), at (ip + 2), t);
                if (std::abs (motorTarget) > 1.5f) tp[c] *= 0.6f;
            }
        }
        if (state != Rec)
        {
            const float rate = (state == FFwd || state == Rwd) ? 1.f : ratioBase * (1.f + dev);     // VARI / PITCH / wow actuan sobre la cinta
            pos += (double) motor * (double) rate;
            if (pos > recLen)
            {
                if (P.loop && (state == Play || state == FFwd)) pos = 0.0;
                else { pos = recLen; if (state != Stop) stopNow(); motor = 0.f; }
            }
            else if (pos < 0.0)
            {
                if (P.loop && (state == Rev || state == Rwd)) pos = recLen;
                else { pos = 0.0; if (state != Stop) stopNow(); motor = 0.f; }
            }
        }

        float sig[2] { live[0] * liveG + tp[0] * tapeG, live[1] * liveG + tp[1] * tapeG };

        // ---- TILT: desalineacion del cabezal (azimut): el canal derecho llega un poco tarde y mas apagado ----
        if (P.tilt)
        {
            azBuf[(size_t) azW] = sig[1];
            const float d = azBuf[(size_t) ((azW - 8 + 256) & 255)];
            azLP += 0.45f * (d - azLP);
            sig[1] = azLP;
            azW = (azW + 1) & 255;
        }

        // ---- HISS y NOISE (zumbido, chasquidos, caidas de cinta) ----
        const float u = rnd() * 0.5f + 0.5f;
        if (dropCnt > 0) { if (--dropCnt == 0) dropTarget = 1.f; }
        else if (u < dropP) { dropTarget = 0.3f + 0.4f * (rnd() * 0.5f + 0.5f); dropCnt = (int) (sr * (0.01f + 0.05f * (rnd() * 0.5f + 0.5f))); }
        dropEnv += (dropTarget - dropEnv) * 0.003f;
        humPh += kTwoPi * 60.f / sr; if (humPh > kPi) humPh -= kTwoPi;
        const float hum = (std::sin (humPh) * 0.6f + std::sin (2.f * humPh) * 0.3f) * humAmp;
        for (int c = 0; c < 2; ++c)
        {
            const float nz = rnd();
            float add = (c == 0 ? (nz - hissPrev) * 0.5f : nz * 0.35f) * hissLvl + hum;
            if (c == 0) hissPrev = nz;
            if (noiseLvl > 0.001f && (rnd() * 0.5f + 0.5f) < crackleP) add += rnd() * 0.12f * noiseLvl;
            sig[c] = sig[c] * dropEnv + add;
        }

        // ---- DELAY: eco de cinta (FB realimenta saturado y oscuro; MIX lo mezcla) ----
        dlySm += (delTarget - dlySm) * 0.0006f;
        float rd = (float) dlyW - (dlySm + dA1 * sW * 0.1f + dA2 * sF * 0.2f);
        while (rd < 0.f) rd += (float) dlySize;
        const int d0 = juce::jmin ((int) rd, dlySize - 1);
        const float df = rd - (float) d0;
        const int d1i = d0 + 1 >= dlySize ? 0 : d0 + 1;
        float outv[2];
        for (int c = 0; c < 2; ++c)
        {
            const float wet = dly[c][(size_t) d0] + df * (dly[c][(size_t) d1i] - dly[c][(size_t) d0]);
            const float fbIn = std::tanh (wet * fbGain * 1.3f) / 1.3f;
            dlLP[c] += aDlyLP * (fbIn - dlLP[c]);
            dlHP[c] += aDlyHP * (dlLP[c] - dlHP[c]);
            dly[c][(size_t) dlyW] = sig[c] + (dlLP[c] - dlHP[c]);
            outv[c] = (sig[c] + wet * P.dlyMix) * outG;
            if (useMid) outv[c] = mid[c].run (outv[c]);
            if (useHp)  outv[c] = hpf[c].run (outv[c]);
            if (useLp)  outv[c] = lpf[c].run (outv[c]);
        }
        if (++dlyW >= dlySize) dlyW = 0;
        L[i] = outv[0]; R[i] = outv[1];
    }

    // ---- reverb (extra de la v1.0) ----
    if (P.reverb > 0.001f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.7f; rp.damping = 0.5f; rp.width = 1.f; rp.freezeMode = 0.f;
        rp.wetLevel = P.reverb * 0.4f; rp.dryLevel = 1.f;
        reverb.setParameters (rp);
        reverb.processStereo (L, R, n);
    }

    // ---- POWER (derivacion limpia) y medidores VU ----
    double outSq[2] { 0.0, 0.0 };
    for (int i = 0; i < n; ++i)
    {
        pw += ((P.power ? 1.f : 0.f) - pw) * coefPw;
        L[i] = dryL[i] * (1.f - pw) + L[i] * pw;
        R[i] = dryR[i] * (1.f - pw) + R[i] * pw;
        outSq[0] += (double) L[i] * L[i]; outSq[1] += (double) R[i] * R[i];
    }
    const float k = 1.f - std::exp (-(float) n / (0.3f * sr));          // balistica VU ~300 ms
    for (int c = 0; c < 2; ++c)
    {
        vuAcc[c]     += ((float) std::sqrt (inSq[c] / n)  - vuAcc[c]) * k;
        vuAcc[2 + c] += ((float) std::sqrt (outSq[c] / n) - vuAcc[2 + c]) * k;
        vuIn[c].store (vuAcc[c]); vuOut[c].store (vuAcc[2 + c]);
    }
    motorOut.store (motor);
    posSec.store ((float) (pos / fs));
    recSec.store ((float) (recLen / fs));
}
} // namespace tapermate_dsp
