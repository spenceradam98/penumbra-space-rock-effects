// Factory presets: a clean init patch, then guitar and synth programs in the spirit of
// early-70s space-rock studio production (descriptive names, not song titles).
#include "Engine.h"
#include <string>
#include <vector>

namespace pn
{
namespace
{
using Fn = void (*) (Params&);

struct Def { const char* name; const char* category; Fn fn; };

// small helpers to keep the table readable
void drive (Params& p, int type, float amt, float tone, float level = 0) { p.drvOn = true; p.drvType = type; p.drvDrive = amt; p.drvTone = tone; p.drvLevel = level; }
void eq (Params& p, float low, float mid, float midF, float high, float lowCut = 20) { p.eqOn = true; p.eqLow = low; p.eqMid = mid; p.eqMidFreq = midF; p.eqHigh = high; p.eqLowCut = lowCut; }
void mod (Params& p, int type, float rate, float depth, float fb, float mix) { p.modOn = true; p.modType = type; p.modRate = rate; p.modDepth = depth; p.modFeedback = fb; p.modMix = mix; }
void tape (Params& p, int mode, int heads, float ms, float fb, float mix, float wow = 0.25f, float tone = 0.55f, float glide = 0.25f)
{
    p.dlyOn = true; p.dlyMode = mode; p.dlyHeads = heads; p.dlyTimeMs = ms; p.dlyFeedback = fb; p.dlyMix = mix; p.dlyWow = wow; p.dlyTone = tone; p.dlyGlide = glide;
}
void synced (Params& p, int div) { p.dlySync = true; p.dlyDiv = div; }
void verb (Params& p, int type, float decay, float mix, float pre = 20, float damp = 0.4f) { p.revOn = true; p.revType = type; p.revDecay = decay; p.revMix = mix; p.revPreDelay = pre; p.revDamp = damp; }
void motion (Params& p, int mode, float rate, float depth, float shape = 0) { p.motOn = true; p.motMode = mode; p.motRate = rate; p.motDepth = depth; p.motShape = shape; }
void synthi (Params& p, float cut, float res, float lfo, float depth, float env = 0, float ring = 220, float ringMix = 0)
{
    p.synOn = true; p.synCutoff = cut; p.synRes = res; p.synLfoRate = lfo; p.synLfoDepth = depth; p.synEnv = env; p.synRingFreq = ring; p.synRingMix = ringMix;
}
void rotary (Params& p, bool fast, float drv, float mix = 1, float spread = 0.8f) { p.rotOn = true; p.rotFast = fast; p.rotDrive = drv; p.rotMix = mix; p.rotSpread = spread; }

enum { Plate = 0, Spring = 1, Hall = 2 };
enum { Si = 0, Ge = 1, OD = 2, Valve = 3 };
enum { Ph4 = 0, Ph8 = 1, Flanger = 2, ADT = 3, Chorus = 4 };
enum { Tape = 0, PingPong = 1, Reverse = 2 };
enum { Pan = 0, Trem = 1, Swirl = 2 };

const Def kDefs[] = {
    { "Init", "Basic", [] (Params&) {} },

    // ---------------- guitar ----------------
    { "Lunar Solo Lead", "Guitar", [] (Params& p) { drive (p, Si, 0.55f, 0.55f, -2); eq (p, 0, 3, 900, 0, 70); tape (p, Tape, 0, 480, 0.38f, 0.3f, 0.2f, 0.5f); verb (p, Plate, 3.2f, 0.22f, 30); } },
    { "Breathing Clean Arpeggio", "Guitar", [] (Params& p) { eq (p, 0, 0, 1000, 2, 90); mod (p, Chorus, 0.6f, 0.4f, 0, 0.35f); tape (p, Tape, 0, 450, 0.3f, 0.28f, 0.15f, 0.6f); synced (p, 6); verb (p, Plate, 2.8f, 0.25f, 25); } },
    { "Rotating Speaker Solo", "Guitar", [] (Params& p) { drive (p, Valve, 0.5f, 0.6f); rotary (p, false, 0.3f); verb (p, Plate, 2.0f, 0.18f); } },
    { "Doubled Rhythm", "Guitar", [] (Params& p) { eq (p, 0, 0, 1000, 1, 80); mod (p, ADT, 0.5f, 0.5f, 0, 0.5f); verb (p, Plate, 1.5f, 0.12f); } },
    { "Slide Across the Sky", "Guitar", [] (Params& p) { drive (p, Ge, 0.35f, 0.5f); tape (p, Tape, 0, 380, 0.45f, 0.3f, 0.35f, 0.45f); verb (p, Hall, 5.0f, 0.35f, 40); } },
    { "Clockwork Triple Echo", "Guitar", [] (Params& p) { tape (p, Tape, 2, 600, 0.4f, 0.4f, 0.2f, 0.55f); verb (p, Plate, 2.0f, 0.15f); } },
    { "Phased Twelve-String", "Guitar", [] (Params& p) { mod (p, Ph4, 0.35f, 0.7f, 0.45f, 0.7f); verb (p, Plate, 2.2f, 0.2f); } },
    { "Wall of Fuzz", "Guitar", [] (Params& p) { drive (p, Si, 0.85f, 0.45f, -6); eq (p, 3, -4, 700, 0, 60); tape (p, Tape, 0, 550, 0.3f, 0.2f); verb (p, Hall, 3.5f, 0.25f, 30); } },
    { "Spring Twang Blues", "Guitar", [] (Params& p) { drive (p, Valve, 0.35f, 0.6f); verb (p, Spring, 2.5f, 0.35f, 0); motion (p, Trem, 5.0f, 0.4f); } },
    { "Flanged Jangle", "Guitar", [] (Params& p) { mod (p, Flanger, 0.15f, 0.8f, 0.55f, 0.7f); verb (p, Plate, 1.8f, 0.18f); } },
    { "Backwards Tape Solo", "Guitar", [] (Params& p) { drive (p, OD, 0.5f, 0.55f); tape (p, Reverse, 0, 700, 0.35f, 0.45f, 0.15f, 0.6f); verb (p, Plate, 3.0f, 0.25f); } },
    { "Odd-Meter Bass Throb", "Guitar", [] (Params& p) { eq (p, 3, 0, 800, -2, 30); mod (p, Chorus, 0.4f, 0.3f, 0, 0.25f); motion (p, Pan, 0.5f, 0.6f); } },

    // ---------------- synth ----------------
    { "Sequencer Swirl Run", "Synth", [] (Params& p) { synthi (p, 1800, 0.55f, 0.25f, 0.5f, 0.3f); tape (p, PingPong, 0, 300, 0.45f, 0.3f, 0.1f); synced (p, 2); motion (p, Swirl, 0.3f, 0.9f); verb (p, Hall, 3.0f, 0.2f); } },
    { "Ring Mod Clatter", "Synth", [] (Params& p) { synthi (p, 9000, 0.2f, 0.3f, 0.2f, 0, 440, 0.7f); tape (p, Tape, 0, 250, 0.35f, 0.25f); verb (p, Plate, 1.5f, 0.2f); } },
    { "Tonewheel on Rotor", "Synth", [] (Params& p) { drive (p, Valve, 0.3f, 0.6f, 2); rotary (p, false, 0.4f); verb (p, Hall, 2.2f, 0.18f); } },
    { "Choir Pad Plate", "Synth", [] (Params& p) { eq (p, 0, 0, 1000, 2, 120); mod (p, Chorus, 0.25f, 0.5f, 0, 0.4f); verb (p, Plate, 7.0f, 0.45f, 60, 0.3f); } },
    { "Filter Sweep Drone", "Synth", [] (Params& p) { synthi (p, 300, 0.75f, 0.05f, 0.8f); verb (p, Hall, 6.0f, 0.35f, 50); } },
    { "Reverse Tape Swell", "Synth", [] (Params& p) { tape (p, Reverse, 0, 900, 0.5f, 0.55f, 0.1f, 0.6f); verb (p, Hall, 5.0f, 0.4f); } },
    { "Tape Loop Bed", "Synth", [] (Params& p) { tape (p, Tape, 0, 1600, 0.9f, 0.5f, 0.3f, 0.5f, 0.6f); verb (p, Hall, 8.0f, 0.4f, 40, 0.5f); } },
    { "Heartbeat Pulse", "Synth", [] (Params& p) { synthi (p, 500, 0.3f, 0.2f, 0.0f, 0.5f); motion (p, Trem, 1.1f, 0.8f, 0.6f); verb (p, Plate, 2.0f, 0.2f); } },
    { "Deep Space Hall", "Synth", [] (Params& p) { mod (p, Chorus, 0.1f, 0.3f, 0, 0.3f); tape (p, PingPong, 0, 700, 0.5f, 0.25f, 0.2f, 0.45f); verb (p, Hall, 12.0f, 0.6f, 120, 0.5f); } },
    { "Varispeed Warp Lead", "Synth", [] (Params& p) { mod (p, ADT, 0.6f, 0.6f, 0, 0.4f); tape (p, Tape, 0, 350, 0.5f, 0.35f, 0.6f, 0.5f, 0.9f); verb (p, Plate, 2.5f, 0.2f); } },
    { "Hi-Hat Pan Run", "Synth", [] (Params& p) { eq (p, 0, 0, 1000, 4, 300); tape (p, PingPong, 0, 125, 0.3f, 0.2f); synced (p, 0); motion (p, Pan, 6.0f, 1.0f, 0.3f); } },
    { "Phaser Wash Pad", "Synth", [] (Params& p) { mod (p, Ph8, 0.08f, 0.9f, 0.6f, 0.8f); verb (p, Plate, 5.0f, 0.4f, 40); } },
};

// Output trims that level-match the presets (about +1 dB over the dry signal while playing),
// measured on the demo phrases rendered by tools/render_demo.cpp.
struct Trim { const char* name; float db; };
const Trim kTrims[] = {
    { "Lunar Solo Lead", -5.5f },
    { "Breathing Clean Arpeggio", -2.5f },
    { "Rotating Speaker Solo", -6.0f },
    { "Doubled Rhythm", -2.0f },
    { "Slide Across the Sky", -8.0f },
    { "Clockwork Triple Echo", -2.5f },
    { "Phased Twelve-String", 1.0f },
    { "Wall of Fuzz", 1.5f },
    { "Spring Twang Blues", -5.5f },
    { "Flanged Jangle", 2.0f },
    { "Backwards Tape Solo", -8.5f },
    { "Odd-Meter Bass Throb", 0.0f },
    { "Sequencer Swirl Run", 6.0f },
    { "Ring Mod Clatter", 4.5f },
    { "Tonewheel on Rotor", -6.0f },
    { "Choir Pad Plate", -6.0f },
    { "Filter Sweep Drone", 2.5f },
    { "Reverse Tape Swell", -2.0f },
    { "Tape Loop Bed", -3.5f },
    { "Heartbeat Pulse", 6.0f },
    { "Deep Space Hall", -2.0f },
    { "Varispeed Warp Lead", -4.0f },
    { "Hi-Hat Pan Run", 4.0f },
    { "Phaser Wash Pad", -3.5f },
};

std::vector<Preset> build()
{
    std::vector<Preset> v;
    for (const auto& d : kDefs)
    {
        Params p;
        d.fn (p);
        for (const auto& t : kTrims)
            if (std::string (t.name) == d.name) p.outputDb += t.db;
        v.push_back ({ d.name, d.category, p });
    }
    return v;
}
} // namespace

const Preset* factoryPresets (int& count)
{
    static const std::vector<Preset> presets = build();
    count = (int) presets.size();
    return presets.data();
}
} // namespace pn
