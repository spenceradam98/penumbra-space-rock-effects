// Modulation stage: phasers, flanger, ADT doubler and chorus.
//   Phaser 4 / Phaser 8 - chains of first-order allpasses swept by an LFO, with feedback; the
//                         right channel's LFO runs 90 degrees ahead for stereo movement.
//   Flanger             - short modulated delay (0.3 to 5 ms) with feedback, mixed against the dry
//                         signal (the two-tape-machine trick).
//   ADT Doubler         - artificial double tracking: a second copy 18 to 32 ms late whose delay
//                         wanders randomly like a second tape machine's speed, different per side.
//   Chorus              - three voices on a 12 ms delay with phase-spread sine modulation.
#pragma once

#include "Blocks.h"

namespace pn
{
class Modulation
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (int c = 0; c < 2; ++c)
        {
            ch[c].delay.allocate ((int) (0.06 * fs) + 8);
            ch[c].drift.set (fs, 1.0, 0x51u + 1000u * (uint32_t) c);
            ch[c].fbLp.set (fs, 7000.0);
        }
        sDepth.set (fs, 30.0); sFb.set (fs, 30.0); sMix.set (fs, 30.0);
        reset();
    }
    void reset()
    {
        for (auto& c : ch) { c.delay.clear(); for (auto& a : c.ap) a.z = 0; c.fb = 0; c.fbLp.reset(); }
        lfo.phase = 0; counter = 0;
        sDepth.reset (depth); sFb.reset (fb); sMix.reset (mix);
    }

    void setParams (int typeIn, double rate, double depthIn, double fbIn, double mixIn)
    {
        if (typeIn != type) { type = typeIn; for (auto& c : ch) { c.fb = 0; for (auto& a : c.ap) a.z = 0; } }
        lfo.setRate (fs, rate);
        rateHz = rate; depth = depthIn; fb = fbIn; mix = mixIn;
        for (int c = 0; c < 2; ++c) ch[c].driftRate = rate;
    }

    inline void process (double& l, double& r)
    {
        const double ph = lfo.tick();
        lastLfo = Lfo::sine (ph);
        const double d = sDepth.next (depth), f = sFb.next (fb), m = sMix.next (mix);
        const bool refresh = (counter++ & 3) == 0;
        double* io[2] = { &l, &r };
        for (int c = 0; c < 2; ++c)
        {
            auto& s = ch[c];
            const double x = *io[c];
            const double p = ph + (c == 1 ? 0.25 : 0.0);
            double y = x;
            switch (type)
            {
                case 0: case 1:
                {
                    const int stages = type == 0 ? 4 : 8;
                    if (refresh)
                    {
                        // sweep 150 Hz .. ~150 * 2^(1 + 5 * depth) Hz, exponential (as with an LDR/FET sweep)
                        const double sweep = 0.5 + 0.5 * Lfo::sine (p);
                        s.apCoef = Allpass1::coef (fs, 150.0 * std::pow (2.0, (1.0 + 5.0 * d) * sweep));
                    }
                    double v = x + s.fb * f * 0.85;
                    for (int i = 0; i < stages; ++i) v = s.ap[i].process (v, s.apCoef);
                    s.fb = v;
                    y = x * (1.0 - 0.5 * m) + v * 0.5 * m;
                    y *= 1.0 + 0.3 * m;
                    break;
                }
                case 2:
                {
                    const double sweep = 0.5 + 0.5 * Lfo::tri (p);
                    const double dms = 0.3 + 4.7 * d * sweep;
                    s.delay.push (x + s.fb * (0.95 * f));
                    const double v = tapCubic (s.delay, dms * 0.001 * fs);
                    s.fb = s.fbLp.process (v);
                    y = x * (1.0 - 0.5 * m) + v * 0.5 * m;
                    y *= 1.0 + 0.3 * m;
                    break;
                }
                case 3:
                {
                    if (refresh) s.wander = s.drift.process();
                    else s.drift.process();
                    const double base = c == 0 ? 19.0 : 27.0;
                    const double dms = base + (1.0 + 5.0 * d) * 0.6 * s.wander;
                    s.delay.push (x + s.fb * 0.4 * f);
                    const double v = tapCubic (s.delay, std::max (1.0, dms * 0.001 * fs));
                    s.fb = s.fbLp.process (v);
                    y = x * dryGain (m) + v * wetGain (m) * 0.9;
                    break;
                }
                default:
                {
                    s.delay.push (x);
                    double v = 0;
                    for (int k = 0; k < 3; ++k)
                    {
                        const double dms = 11.0 + 2.0 * k + 5.0 * d * (0.5 + 0.5 * Lfo::sine (p + k / 3.0));
                        v += tapCubic (s.delay, dms * 0.001 * fs);
                    }
                    v *= 0.45;
                    y = x * dryGain (m) + v * wetGain (m);
                    break;
                }
            }
            *io[c] = y;
        }
    }

    double getLfo() const { return lastLfo; }

private:
    struct Chan
    {
        Allpass1 ap[8];
        double apCoef = 0, fb = 0, wander = 0, driftRate = 1;
        DelayLine delay;
        Drift drift;
        OnePoleLP fbLp;
    };
    Chan ch[2];
    double fs = 48000, rateHz = 0.4, depth = 0.6, fb = 0.3, mix = 0.5, lastLfo = 0;
    int type = 0;
    unsigned counter = 0;
    Lfo lfo;
    Smoother sDepth, sFb, sMix;
};
} // namespace pn
