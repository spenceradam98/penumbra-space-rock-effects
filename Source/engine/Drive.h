// Drive stage: four voicings of fuzz / overdrive. Each is a pre-filter that shapes what hits
// the clipper, a biased (asymmetric) tanh clipper with ADAA anti-aliasing, and a post tone
// filter, which is roughly how the classic pedals and amps differ from each other:
//   Silicon Fuzz   - two-transistor silicon fuzz: high gain, tight bass, near-square clipping
//   Germanium Fuzz - softer, rounder, strongly asymmetric (sputters as notes decay)
//   Overdrive      - mid-hump into soft clipping (bass rolled off before the clipper)
//   Valve Amp      - low gain, warm asymmetric tube-style breakup with a presence lift
#pragma once

#include "Blocks.h"

namespace pn
{
class Drive
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        gain.set (fs, 20.0);
        lastType = -1; lastTone = -1;
        reset();
    }
    void reset()
    {
        for (auto& c : ch) { c.pre1.reset(); c.pre2.reset(); c.post1.reset(); c.post2.reset(); c.clip.reset(); c.dc.reset(); }
    }

    void setParams (int type, double drive, double tone, double levelDb)
    {
        if (type != lastType || std::abs (tone - lastTone) > 1e-4)
        {
            const double toneHz = std::min (17000.0, 700.0 * std::pow (2.0, tone * 4.6));
            for (auto& c : ch)
            {
                switch (type)
                {
                    case 0: c.pre1.highpass (fs, 140.0, 0.6); c.pre2.peak (fs, 1200.0, 0.6, 3.0); c.clip.setBias (0.12); break;
                    case 1: c.pre1.highpass (fs, 60.0, 0.6);  c.pre2.lowShelf (fs, 200.0, 3.0);   c.clip.setBias (0.55); break;
                    case 2: c.pre1.highpass (fs, 300.0, 0.5); c.pre2.peak (fs, 850.0, 0.7, 6.0);  c.clip.setBias (0.05); break;
                    default: c.pre1.highpass (fs, 35.0, 0.6); c.pre2.peak (fs, 3200.0, 0.8, 3.0); c.clip.setBias (0.35); break;
                }
                c.post1.lowpass (fs, toneHz, 0.707);
                c.post2.peak (fs, type == 2 ? 720.0 : 400.0, 0.8, type == 0 ? -4.0 : (type == 1 ? -1.5 : 0.0));   // fuzz mid scoop
                c.dc.set (fs, 10.0);
            }
            lastType = type; lastTone = tone;
        }
        static const double minDb[4] = { 14.0, 8.0, 6.0, 0.0 }, rangeDb[4] = { 46.0, 34.0, 30.0, 26.0 };
        static const double makeup[4] = { 0.55, 0.65, 0.8, 0.9 };
        const int t = type < 0 ? 0 : (type > 3 ? 3 : type);
        targetGain = dbToGain (minDb[t] + rangeDb[t] * drive);
        outGain = makeup[t] * dbToGain (levelDb);
    }

    inline double process (double x, int c)
    {
        auto& s = ch[c];
        const double g = c == 0 ? gain.next (targetGain) : gain.y;
        double v = s.pre2.process (s.pre1.process (x));
        v = s.clip.process (v * g);
        v = s.dc.process (v);
        v = s.post2.process (s.post1.process (v));
        return v * outGain;
    }

private:
    struct Chan { Biquad pre1, pre2, post1, post2; AdaaTanh clip; DcBlocker dc; };
    Chan ch[2];
    double fs = 48000, targetGain = 1, outGain = 1, lastTone = -1;
    int lastType = -1;
    Smoother gain;
};
} // namespace pn
