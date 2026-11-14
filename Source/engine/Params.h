// Penumbra parameters: one plain struct for the engine, plus a descriptor table that the
// JUCE wrapper turns into host parameters (so ids, ranges and defaults live in one place).
#pragma once

#include <cstddef>

namespace pn
{
struct Params
{
    // ---- global ----
    float inputDb = 0.0f;        // -24..+24
    float outputDb = 0.0f;       // -24..+24

    // ---- 1. Drive (fuzz / overdrive) ----
    bool  drvOn = false;
    int   drvType = 0;           // 0 Silicon Fuzz, 1 Germanium Fuzz, 2 Overdrive, 3 Valve Amp
    float drvDrive = 0.5f;       // 0..1
    float drvTone = 0.6f;        // 0..1 dark..bright
    float drvLevel = 0.0f;       // dB -24..+12

    // ---- 2. EQ / tone shaping ----
    bool  eqOn = false;
    float eqLow = 0.0f;          // dB, shelf at 100 Hz
    float eqMid = 0.0f;          // dB, bell
    float eqMidFreq = 800.0f;    // Hz 200..5000
    float eqHigh = 0.0f;         // dB, shelf at 4.5 kHz
    float eqLowCut = 20.0f;      // Hz 20..400 high-pass

    // ---- 3. Synthi (VCS3-style ring modulator + ladder filter + LFO + envelope) ----
    bool  synOn = false;
    float synRingFreq = 220.0f;  // Hz 20..2000
    float synRingMix = 0.0f;     // 0..1
    float synCutoff = 4000.0f;   // Hz 40..18000
    float synRes = 0.2f;         // 0..1 (self-oscillates near 1)
    float synLfoRate = 0.5f;     // Hz 0.02..20
    float synLfoDepth = 0.3f;    // 0..1 (0..+-3 octaves of cutoff, ring freq +-1/2 octave)
    float synEnv = 0.0f;         // -1..+1 envelope follower to cutoff

    // ---- 4. Modulation ----
    bool  modOn = false;
    int   modType = 0;           // 0 Phaser 4, 1 Phaser 8, 2 Flanger, 3 ADT Doubler, 4 Chorus
    float modRate = 0.4f;        // Hz 0.02..10
    float modDepth = 0.6f;       // 0..1
    float modFeedback = 0.3f;    // 0..1
    float modMix = 0.5f;         // 0..1

    // ---- 5. Rotary speaker ----
    bool  rotOn = false;
    bool  rotFast = false;       // chorale (slow) / tremolo (fast)
    float rotDrive = 0.2f;       // 0..1 preamp drive
    float rotMix = 1.0f;         // 0..1
    float rotSpread = 0.8f;      // 0..1 mic spread (stereo width)

    // ---- 6. Tape delay ----
    bool  dlyOn = false;
    int   dlyMode = 0;           // 0 Tape, 1 Ping-Pong, 2 Reverse
    int   dlyHeads = 0;          // 0 Single, 1 Dual (3/4 + 1), 2 Triple (1/3, 2/3, 1)
    float dlyTimeMs = 400.0f;    // 20..1900
    bool  dlySync = false;
    int   dlyDiv = 5;            // see kDelayDivisions
    float dlyFeedback = 0.35f;   // 0..1 (self-oscillates just below 1, held by tape saturation)
    float dlyMix = 0.35f;        // 0..1
    float dlyWow = 0.25f;        // 0..1
    float dlyTone = 0.6f;        // 0..1 dark..bright repeats
    float dlyGlide = 0.25f;      // 0..1 how slowly the tape speed follows the Time knob (pitch bends)
    bool  dlyFreeze = false;     // loop what is on the tape, stop recording

    // ---- 7. Reverb ----
    bool  revOn = false;
    int   revType = 0;           // 0 Plate, 1 Spring, 2 Hall
    float revDecay = 2.5f;       // s 0.3..12
    float revPreDelay = 20.0f;   // ms 0..250
    float revDamp = 0.4f;        // 0..1
    float revMix = 0.25f;        // 0..1

    // ---- 8. Motion (auto-pan / tremolo / swirl) ----
    bool  motOn = false;
    int   motMode = 0;           // 0 Auto-Pan, 1 Tremolo, 2 Swirl
    float motRate = 1.0f;        // Hz 0.05..12
    float motDepth = 0.7f;       // 0..1
    float motShape = 0.0f;       // 0 sine .. 1 square
};

constexpr int kNumDelayDivisions = 9;
constexpr const char* kDelayDivisionNames[kNumDelayDivisions] = { "1/16", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1 bar" };
constexpr double kDelayDivisionBeats[kNumDelayDivisions] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0 };

enum class Kind { Float, Choice, Toggle };

struct ParamDesc
{
    const char* id;
    const char* name;
    const char* group;
    Kind kind;
    float lo, hi;
    float centre;              // skew centre for Float (value shown at mid travel), 0 = linear
    const char* unit;          // display unit for Float
    const char* choices;       // '|' separated, for Choice
    void (*set) (Params&, float);
    float (*get) (const Params&);
};

const ParamDesc* paramTable (int& count);
} // namespace pn
