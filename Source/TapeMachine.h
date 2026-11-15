#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

namespace tapermate_dsp
{
enum Transport { Stop = 0, Play, Rec, Rev, FFwd, Rwd };

struct Params
{
    bool power = true, loop = false, tilt = false;
    int speed = 1;                                  // 0 = 7.5 ips, 1 = 15 ips
    float vari = 0.f;                               // vari-speed: -0.5 .. +0.5
    float pitchSemis = 0.f, fineCents = 0.f;        // PITCH y L.PITCH
    float lfPitch = 0.f, hiss = 0.f, noise = 0.f, satMix = 1.f;      // 0..1
    float delayMs = 300.f, dlyFb = 0.35f, dlyMix = 0.f;
    float inDb = 0.f, outDb = 0.f;
    float reverb = 0.f, locut = 10.f, midDb = 0.f, hicut = 20000.f;  // extras de la v1.0
};

// Maquina de cinta estereo: grabadora (60 s) + procesado en vivo. Sin asignaciones de memoria en process().
class TapeMachine
{
public:
    static constexpr double kTapeSeconds = 60.0;

    void prepare (double sampleRate);
    void reset();
    void clearTape() { clearReq.store (true); }
    void rewind() { rewindReq.store (true); }
    // transport lo modifica la interfaz (botones) y tambien el motor (fin de cinta -> STOP)
    void process (float* L, float* R, int n, const Params& p, std::atomic<int>& transport);

    // datos para la interfaz
    std::atomic<float> vuIn[2] { { 0.f }, { 0.f } }, vuOut[2] { { 0.f }, { 0.f } }, motorOut { 0.f }, posSec { 0.f }, recSec { 0.f };

private:
    struct Biquad
    {
        float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f, z1 = 0.f, z2 = 0.f;
        float run (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void set (double B0, double B1, double B2, double A0, double A1, double A2)
        { b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0); }
        void peak (double fs, double f, double Q, double dB);
        void lowpass (double fs, double f, double Q);
        void highpass (double fs, double f, double Q);
    };

    void run (float* L, float* R, int n, const Params& p, std::atomic<int>& transport);
    float readShift (int c, float delay) const;
    float readVib (int c, float delay) const;
    float rnd();

    double fs = 44100.0;
    std::atomic<bool> clearReq { false }, rewindReq { false };

    // cinta
    std::vector<float> tape[2];
    size_t cap = 0;
    double pos = 0.0, recLen = 0.0;
    float motor = 0.f, liveG = 1.f, tapeG = 0.f, pw = 1.f;
    int prevState = 0;

    // camino en vivo: cambio de velocidad (pitch) + vibrato (wow / flutter)
    std::vector<float> shBuf[2], vibBuf[2];
    int shSize = 0, shW = 0, shWrite = 0, vibSize = 0, vibW = 0;
    float shPhase = 0.f, mixB = 0.f;
    float wowPh = 0.f, flPh = 0.f, drift = 0.f, driftT = 0.f;
    int driftCnt = 0;

    // filtros
    Biquad bump[2], hf[2], hpf[2], lpf[2], mid[2];
    int lastSpeed = -1;
    float lastLo = -1.f, lastHi = -1.f, lastMid = 999.f;
    std::vector<float> azBuf; int azW = 0; float azLP = 0.f;

    // eco de cinta
    std::vector<float> dly[2];
    int dlySize = 0, dlyW = 0;
    float dlySm = 0.f, dlLP[2] {}, dlHP[2] {};

    // ruido / imperfecciones
    uint32_t rng = 2463534242u;
    float hissPrev = 0.f, humPh = 0.f, dropEnv = 1.f, dropTarget = 1.f;
    int dropCnt = 0;
    float vuAcc[4] {};

    juce::Reverb reverb;
};
}
