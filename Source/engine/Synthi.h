// "Synthi" stage: the kind of patch a 1970s VCS3-style suitcase synth was used for as an
// effects processor. The input goes through a ring modulator (against an internal sine
// oscillator), then a resonant four-pole ladder low-pass. A slow LFO sweeps the cutoff (and a
// little of the ring frequency), and an envelope follower can open or close the filter with
// the playing dynamics. Turned up, the resonance self-oscillates.
#pragma once

#include "Blocks.h"

namespace pn
{
class Synthi
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        sRing.set (fs, 30.0); sCut.set (fs / 8.0, 25.0); sRingMix.set (fs, 20.0);   // sCut advances once per 8 samples
        envAtt = std::exp (-1.0 / (0.004 * fs));
        envRel = std::exp (-1.0 / (0.150 * fs));
        reset();
    }
    void reset()
    {
        for (auto& l : ladder) l.reset();
        oscPhase = 0; env = 0; lfo.phase = 0; counter = 0;
        sRing.reset (ringHz); sCut.reset (cutHz); sRingMix.reset (ringMix);
    }

    void setParams (double ringFreq, double ringMixIn, double cutoff, double resIn, double lfoRate, double lfoDepthIn, double envAmtIn)
    {
        ringHz = ringFreq; ringMix = ringMixIn; cutHz = cutoff; res = resIn;
        lfo.setRate (fs, lfoRate); lfoDepth = lfoDepthIn; envAmt = envAmtIn;
    }

    // Processes one stereo frame in place.
    inline void process (double& l, double& r)
    {
        const double ph = lfo.tick();
        const double lv = Lfo::sine (ph);
        lastLfo = lv;

        // envelope follower on the (pre-mod) input
        const double a = 0.5 * (std::abs (l) + std::abs (r));
        env = a > env ? a + envAtt * (env - a) : a + envRel * (env - a);

        // ring modulator
        const double rf = sRing.next (ringHz) * std::pow (2.0, 0.5 * lfoDepth * lv);
        oscPhase += rf / fs; if (oscPhase >= 1.0) oscPhase -= std::floor (oscPhase);
        const double osc = std::sin (kTwoPi * oscPhase);
        const double rm = sRingMix.next (ringMix);
        l = l * (1.0 - rm) + l * osc * rm * 1.4;
        r = r * (1.0 - rm) + r * osc * rm * 1.4;

        // filter, coefficients refreshed every 8 samples
        if ((counter++ & 7) == 0)
        {
            const double envOct = envAmt * std::min (1.0, env * 4.0) * 4.0;
            const double fc = sCut.next (cutHz) * std::pow (2.0, 3.0 * lfoDepth * lv + envOct);
            ladder[0].set (fs, fc, res); ladder[1].set (fs, fc, res);
            lastCut = fc;
        }
        l = ladder[0].process (l);
        r = ladder[1].process (r);
    }

    double getLfo() const { return lastLfo; }
    double getCutoff() const { return lastCut; }

private:
    double fs = 48000, ringHz = 220, ringMix = 0, cutHz = 4000, res = 0.2, lfoDepth = 0.3, envAmt = 0;
    double oscPhase = 0, env = 0, envAtt = 0, envRel = 0, lastLfo = 0, lastCut = 4000;
    unsigned counter = 0;
    Lfo lfo;
    Smoother sRing, sCut, sRingMix;
    Ladder ladder[2];
};
} // namespace pn
