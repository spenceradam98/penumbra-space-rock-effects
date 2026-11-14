#include "Engine.h"

namespace pn
{
void Engine::prepare (double sampleRate, int)
{
    fs = sampleRate > 0 ? sampleRate : 48000.0;
    drive.prepare (fs);
    synthi.prepare (fs);
    mod.prepare (fs);
    rotary.prepare (fs);
    delay.prepare (fs);
    reverb.prepare (fs);
    motion.prepare (fs);
    for (auto& f : fade) f.set (fs, 15.0);
    sIn.set (fs, 20.0); sOut.set (fs, 20.0);
    for (auto& e : lastEq) e = -99;
    setParams (params);
    reset();
}

void Engine::reset()
{
    drive.reset(); synthi.reset(); mod.reset(); rotary.reset(); delay.reset(); reverb.reset(); motion.reset();
    for (int c = 0; c < 2; ++c) { eqHp[c].reset(); eqHp2[c].reset(); eqLow[c].reset(); eqMid[c].reset(); eqHigh[c].reset(); }
    const bool on[kNumModules] = { params.drvOn, params.eqOn, params.synOn, params.modOn, params.rotOn, params.dlyOn, params.revOn, params.motOn };
    for (int m = 0; m < kNumModules; ++m) { fade[m].reset (on[m] ? 1.0 : 0.0); cleared[m] = ! on[m]; }
    sIn.reset (dbToGain (params.inputDb)); sOut.reset (dbToGain (params.outputDb));
}

void Engine::resetModule (int m)
{
    switch (m)
    {
        case kDrive: drive.reset(); break;
        case kEq: for (int c = 0; c < 2; ++c) { eqHp[c].reset(); eqHp2[c].reset(); eqLow[c].reset(); eqMid[c].reset(); eqHigh[c].reset(); } break;
        case kSynthi: synthi.reset(); break;
        case kMod: mod.reset(); break;
        case kRotary: rotary.reset(); break;
        case kDelay: delay.reset(); break;
        case kReverb: reverb.reset(); break;
        case kMotion: motion.reset(); break;
        default: break;
    }
}

void Engine::updateEq()
{
    const float v[5] = { params.eqLow, params.eqMid, params.eqMidFreq, params.eqHigh, params.eqLowCut };
    bool changed = false;
    for (int i = 0; i < 5; ++i) if (v[i] != lastEq[i]) { changed = true; lastEq[i] = v[i]; }
    if (! changed) return;
    for (int c = 0; c < 2; ++c)
    {
        eqHp[c].highpass (fs, params.eqLowCut, 0.5412);   // 4th-order Butterworth low cut
        eqHp2[c].highpass (fs, params.eqLowCut, 1.3066);
        eqLow[c].lowShelf (fs, 100.0, params.eqLow, 0.7);
        eqMid[c].peak (fs, params.eqMidFreq, 0.9, params.eqMid);
        eqHigh[c].highShelf (fs, 4500.0, params.eqHigh, 0.7);
    }
}

void Engine::setParams (const Params& p)
{
    params = p;
    drive.setParams (p.drvType, p.drvDrive, p.drvTone, p.drvLevel);
    updateEq();
    synthi.setParams (p.synRingFreq, p.synRingMix, p.synCutoff, p.synRes, p.synLfoRate, p.synLfoDepth, p.synEnv);
    mod.setParams (p.modType, p.modRate, p.modDepth, p.modFeedback, p.modMix);
    rotary.setParams (p.rotFast, p.rotDrive, p.rotMix, p.rotSpread);
    delay.setSync (p.dlySync, p.dlyDiv);
    delay.setParams (p.dlyMode, p.dlyHeads, p.dlyTimeMs, p.dlyFeedback, p.dlyMix, p.dlyWow, p.dlyTone, p.dlyGlide, p.dlyFreeze);
    reverb.setParams (p.revType, p.revDecay, p.revPreDelay, p.revDamp, p.revMix);
    motion.setParams (p.motMode, p.motRate, p.motDepth, p.motShape);
}

void Engine::process (float* left, float* right, int n)
{
    const bool on[kNumModules] = { params.drvOn, params.eqOn, params.synOn, params.modOn, params.rotOn, params.dlyOn, params.revOn, params.motOn };
    for (int m = 0; m < kNumModules; ++m)
        if (on[m] && cleared[m]) { resetModule (m); cleared[m] = false; }

    const double inG = dbToGain (params.inputDb), outG = dbToGain (params.outputDb);
    float pkIn = 0, pkOut = 0;
    bool bad = false;

    for (int i = 0; i < n; ++i)
    {
        const double gi = sIn.next (inG);
        double l = left[i] * gi, r = right[i] * gi;
        pkIn = std::max (pkIn, (float) std::max (std::abs (l), std::abs (r)));

        for (int m = 0; m < kNumModules; ++m)
        {
            const double f = fade[m].next (on[m] ? 1.0 : 0.0);
            if (! on[m] && f < 1e-4) continue;
            double pl = l, pr = r;
            switch (m)
            {
                case kDrive: pl = drive.process (l, 0); pr = drive.process (r, 1); break;
                case kEq:
                    for (int c = 0; c < 2; ++c)
                    {
                        double& v = c == 0 ? pl : pr;
                        v = eqHigh[c].process (eqMid[c].process (eqLow[c].process (eqHp2[c].process (eqHp[c].process (v)))));
                    }
                    break;
                case kSynthi: synthi.process (pl, pr); break;
                case kMod: mod.process (pl, pr); break;
                case kRotary: rotary.process (pl, pr); break;
                case kDelay: delay.process (pl, pr); break;
                case kReverb: reverb.process (pl, pr); break;
                case kMotion: motion.process (pl, pr); break;
                default: break;
            }
            l += f * (pl - l);
            r += f * (pr - r);
        }

        const double go = sOut.next (outG);
        l *= go; r *= go;
        if (! std::isfinite (l) || ! std::isfinite (r)) { bad = true; l = r = 0; }
        // safety: never hand the host anything wild (a runaway loop is held by the tape anyway)
        l = clampd (l, -4.0, 4.0); r = clampd (r, -4.0, 4.0);
        left[i] = (float) l; right[i] = (float) r;
        pkOut = std::max (pkOut, (float) std::max (std::abs (l), std::abs (r)));
    }

    if (bad) reset();
    for (int m = 0; m < kNumModules; ++m)
    {
        if (! on[m] && fade[m].y < 1e-4 && ! cleared[m]) cleared[m] = true;   // reset lazily on re-enable
        activity[m].store ((float) fade[m].y, std::memory_order_relaxed);
    }

    inPeak.store (std::max (pkIn, inPeak.load (std::memory_order_relaxed) * 0.9f), std::memory_order_relaxed);
    outPeak.store (std::max (pkOut, outPeak.load (std::memory_order_relaxed) * 0.9f), std::memory_order_relaxed);
    hornAngle.store ((float) rotary.getHornAngle(), std::memory_order_relaxed);
    drumAngle.store ((float) rotary.getDrumAngle(), std::memory_order_relaxed);
    modLfo.store ((float) mod.getLfo(), std::memory_order_relaxed);
    synLfo.store ((float) synthi.getLfo(), std::memory_order_relaxed);
    panPos.store ((float) motion.getPosition(), std::memory_order_relaxed);
    delayMs.store ((float) delay.currentDelayMs(), std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------- parameter table
namespace
{
#define PN_F(ID, NAME, GROUP, FIELD, LO, HI, CENTRE, UNIT) \
    { ID, NAME, GROUP, Kind::Float, LO, HI, CENTRE, UNIT, nullptr, [] (Params& p, float v) { p.FIELD = v; }, [] (const Params& p) { return (float) p.FIELD; } }
#define PN_C(ID, NAME, GROUP, FIELD, CHOICES, N) \
    { ID, NAME, GROUP, Kind::Choice, 0, (float) (N - 1), 0, "", CHOICES, [] (Params& p, float v) { p.FIELD = (int) std::lround (v); }, [] (const Params& p) { return (float) p.FIELD; } }
#define PN_T(ID, NAME, GROUP, FIELD) \
    { ID, NAME, GROUP, Kind::Toggle, 0, 1, 0, "", nullptr, [] (Params& p, float v) { p.FIELD = v > 0.5f; }, [] (const Params& p) { return p.FIELD ? 1.0f : 0.0f; } }

const ParamDesc kTable[] = {
    PN_F ("in_gain", "Input", "Global", inputDb, -24, 24, 0, "dB"),
    PN_F ("out_gain", "Output", "Global", outputDb, -24, 24, 0, "dB"),

    PN_T ("drv_on", "Drive On", "Drive", drvOn),
    PN_C ("drv_type", "Drive Type", "Drive", drvType, "Silicon Fuzz|Germanium Fuzz|Overdrive|Valve Amp", 4),
    PN_F ("drv_drive", "Drive", "Drive", drvDrive, 0, 1, 0, "%"),
    PN_F ("drv_tone", "Drive Tone", "Drive", drvTone, 0, 1, 0, "%"),
    PN_F ("drv_level", "Drive Level", "Drive", drvLevel, -24, 12, 0, "dB"),

    PN_T ("eq_on", "EQ On", "EQ", eqOn),
    PN_F ("eq_low", "EQ Bass", "EQ", eqLow, -15, 15, 0, "dB"),
    PN_F ("eq_mid", "EQ Mid", "EQ", eqMid, -15, 15, 0, "dB"),
    PN_F ("eq_midf", "EQ Mid Freq", "EQ", eqMidFreq, 200, 5000, 1000, "Hz"),
    PN_F ("eq_high", "EQ Treble", "EQ", eqHigh, -15, 15, 0, "dB"),
    PN_F ("eq_lowcut", "EQ Low Cut", "EQ", eqLowCut, 20, 400, 80, "Hz"),

    PN_T ("syn_on", "Synthi On", "Synthi", synOn),
    PN_F ("syn_ring", "Ring Freq", "Synthi", synRingFreq, 20, 2000, 200, "Hz"),
    PN_F ("syn_ringmix", "Ring Mix", "Synthi", synRingMix, 0, 1, 0, "%"),
    PN_F ("syn_cut", "Filter Cutoff", "Synthi", synCutoff, 40, 18000, 1000, "Hz"),
    PN_F ("syn_res", "Filter Resonance", "Synthi", synRes, 0, 1, 0, "%"),
    PN_F ("syn_lfo", "Synthi LFO Rate", "Synthi", synLfoRate, 0.02f, 20, 1, "Hz"),
    PN_F ("syn_lfodepth", "Synthi LFO Depth", "Synthi", synLfoDepth, 0, 1, 0, "%"),
    PN_F ("syn_env", "Envelope Amount", "Synthi", synEnv, -1, 1, 0, "%"),

    PN_T ("mod_on", "Mod On", "Modulation", modOn),
    PN_C ("mod_type", "Mod Type", "Modulation", modType, "Phaser 4|Phaser 8|Flanger|ADT Doubler|Chorus", 5),
    PN_F ("mod_rate", "Mod Rate", "Modulation", modRate, 0.02f, 10, 0.8f, "Hz"),
    PN_F ("mod_depth", "Mod Depth", "Modulation", modDepth, 0, 1, 0, "%"),
    PN_F ("mod_fb", "Mod Feedback", "Modulation", modFeedback, 0, 1, 0, "%"),
    PN_F ("mod_mix", "Mod Mix", "Modulation", modMix, 0, 1, 0, "%"),

    PN_T ("rot_on", "Rotary On", "Rotary", rotOn),
    PN_T ("rot_fast", "Rotary Fast", "Rotary", rotFast),
    PN_F ("rot_drive", "Rotary Drive", "Rotary", rotDrive, 0, 1, 0, "%"),
    PN_F ("rot_mix", "Rotary Mix", "Rotary", rotMix, 0, 1, 0, "%"),
    PN_F ("rot_spread", "Rotary Spread", "Rotary", rotSpread, 0, 1, 0, "%"),

    PN_T ("dly_on", "Delay On", "Tape Delay", dlyOn),
    PN_C ("dly_mode", "Delay Mode", "Tape Delay", dlyMode, "Tape|Ping-Pong|Reverse", 3),
    PN_C ("dly_heads", "Delay Heads", "Tape Delay", dlyHeads, "Single|Dual|Triple", 3),
    PN_F ("dly_time", "Delay Time", "Tape Delay", dlyTimeMs, 20, 1900, 400, "ms"),
    PN_T ("dly_sync", "Delay Sync", "Tape Delay", dlySync),
    PN_C ("dly_div", "Delay Division", "Tape Delay", dlyDiv, "1/16|1/8T|1/8|1/8D|1/4T|1/4|1/4D|1/2|1 bar", 9),
    PN_F ("dly_fb", "Delay Feedback", "Tape Delay", dlyFeedback, 0, 1, 0, "%"),
    PN_F ("dly_mix", "Delay Mix", "Tape Delay", dlyMix, 0, 1, 0, "%"),
    PN_F ("dly_wow", "Wow & Flutter", "Tape Delay", dlyWow, 0, 1, 0, "%"),
    PN_F ("dly_tone", "Delay Tone", "Tape Delay", dlyTone, 0, 1, 0, "%"),
    PN_F ("dly_glide", "Tape Glide", "Tape Delay", dlyGlide, 0, 1, 0, "%"),
    PN_T ("dly_freeze", "Tape Freeze", "Tape Delay", dlyFreeze),

    PN_T ("rev_on", "Reverb On", "Reverb", revOn),
    PN_C ("rev_type", "Reverb Type", "Reverb", revType, "Plate|Spring|Hall", 3),
    PN_F ("rev_decay", "Reverb Decay", "Reverb", revDecay, 0.3f, 12, 2.5f, "s"),
    PN_F ("rev_pre", "Pre-Delay", "Reverb", revPreDelay, 0, 250, 40, "ms"),
    PN_F ("rev_damp", "Reverb Damping", "Reverb", revDamp, 0, 1, 0, "%"),
    PN_F ("rev_mix", "Reverb Mix", "Reverb", revMix, 0, 1, 0, "%"),

    PN_T ("mot_on", "Motion On", "Motion", motOn),
    PN_C ("mot_mode", "Motion Mode", "Motion", motMode, "Auto-Pan|Tremolo|Swirl", 3),
    PN_F ("mot_rate", "Motion Rate", "Motion", motRate, 0.05f, 12, 1, "Hz"),
    PN_F ("mot_depth", "Motion Depth", "Motion", motDepth, 0, 1, 0, "%"),
    PN_F ("mot_shape", "Motion Shape", "Motion", motShape, 0, 1, 0, "%"),
};
#undef PN_F
#undef PN_C
#undef PN_T
} // namespace

const ParamDesc* paramTable (int& count)
{
    count = (int) (sizeof (kTable) / sizeof (kTable[0]));
    return kTable;
}
} // namespace pn
