// Penumbra engine: a space-rock studio effects rack in one plugin. Plain C++17, no JUCE
// dependency, unit-testable. Stereo in, stereo out, zero latency.
//
// Fixed signal chain (the order a 1970s studio would patch it):
//
//   in -> Input -> [Drive] -> [EQ] -> [Synthi] -> [Modulation] -> [Rotary] -> [Tape Delay]
//      -> [Reverb] -> [Motion] -> Output
//
// Each module has its own on/off switch; switching crossfades over ~15 ms, and a module that
// is fully off costs nothing and is cleared, so it starts clean when switched back on.
#pragma once

#include "Params.h"
#include "Drive.h"
#include "Synthi.h"
#include "Modulation.h"
#include "Rotary.h"
#include "TapeDelay.h"
#include "Reverbs.h"
#include "Motion.h"
#include <atomic>

namespace pn
{
enum Module { kDrive = 0, kEq, kSynthi, kMod, kRotary, kDelay, kReverb, kMotion, kNumModules };

struct Preset { const char* name; const char* category; Params p; };
const Preset* factoryPresets (int& count);

class Engine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setParams (const Params& p);
    const Params& getParams() const { return params; }
    void setTempo (double bpm) { delay.setTempo (bpm); }

    // Stereo, in place.
    void process (float* left, float* right, int numSamples);

    // ---- UI state (written by the audio thread) ----
    float getInputPeak() const { return inPeak.load (std::memory_order_relaxed); }
    float getOutputPeak() const { return outPeak.load (std::memory_order_relaxed); }
    float getHornAngle() const { return hornAngle.load (std::memory_order_relaxed); }
    float getDrumAngle() const { return drumAngle.load (std::memory_order_relaxed); }
    float getModLfo() const { return modLfo.load (std::memory_order_relaxed); }
    float getSynLfo() const { return synLfo.load (std::memory_order_relaxed); }
    float getPanPosition() const { return panPos.load (std::memory_order_relaxed); }
    float getDelayMs() const { return delayMs.load (std::memory_order_relaxed); }
    float getModuleActivity (int m) const { return activity[m].load (std::memory_order_relaxed); }

private:
    void updateEq();
    void resetModule (int m);

    Params params;
    double fs = 48000;

    Drive drive;
    Biquad eqHp[2], eqHp2[2], eqLow[2], eqMid[2], eqHigh[2];
    Synthi synthi;
    Modulation mod;
    Rotary rotary;
    TapeDelay delay;
    Reverb reverb;
    Motion motion;

    Smoother fade[kNumModules], sIn, sOut;
    bool cleared[kNumModules] {};
    float lastEq[5] { -99, -99, -99, -99, -99 };

    std::atomic<float> inPeak { 0 }, outPeak { 0 }, hornAngle { 0 }, drumAngle { 0 }, modLfo { 0 }, synLfo { 0 }, panPos { 0 }, delayMs { 0 };
    std::atomic<float> activity[kNumModules] {};
};
} // namespace pn
