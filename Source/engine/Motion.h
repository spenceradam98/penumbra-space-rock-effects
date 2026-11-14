// Motion stage: stereo movement at the end of the chain.
//   Auto-Pan - balance swings left/right (equal-power), Shape morphs sine to square.
//   Tremolo  - amplitude modulation.
//   Swirl    - a circular path around the listener, as with the quadraphonic joystick panner:
//              left/right pan plus front/back cues (darker, slightly quieter and a few hundred
//              microseconds of inter-channel delay when the sound is "behind").
#pragma once

#include "Blocks.h"

namespace pn
{
class Motion
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (auto& d : haas) d.allocate ((int) (0.002 * fs) + 4);
        sDepth.set (fs, 30.0);
        reset();
    }
    void reset()
    {
        for (auto& d : haas) d.clear();
        lpL = lpR = 0; lfo.phase = 0; sDepth.reset (depth);
    }
    void setParams (int modeIn, double rate, double depthIn, double shapeIn)
    {
        mode = modeIn; lfo.setRate (fs, rate); depth = depthIn; shape = shapeIn;
    }

    inline void process (double& l, double& r)
    {
        const double ph = lfo.tick();
        const double d = sDepth.next (depth);
        const double k = 1.0 + 12.0 * shape * shape;
        auto shaped = [k] (double s) { return std::tanh (k * s) / std::tanh (k); };
        const double s = shaped (Lfo::sine (ph));
        position = s;

        if (mode == 1)
        {
            const double g = 1.0 - d * (0.5 + 0.5 * s);
            l *= g; r *= g;
            return;
        }
        // pan position -1..+1; equal-power gains, normalised to unity at centre
        const double p = d * s;
        const double ang = (p + 1.0) * 0.25 * kPi;
        const double gl = std::cos (ang) * 1.41421356, gr = std::sin (ang) * 1.41421356;
        if (mode == 0)
        {
            const double mono = 0.5 * (l + r), side = 0.5 * (l - r);
            l = (mono + side * (1.0 - d)) * gl;
            r = (mono - side * (1.0 - d)) * gr;
            return;
        }
        // swirl: back half of the circle is duller, a little quieter and slightly smeared
        const double back = 0.5 - 0.5 * Lfo::sine (ph + 0.25);   // 0 front .. 1 back (cosine of the path)
        const double coef = std::exp (-kTwoPi * (16000.0 * std::pow (2.0, -2.6 * back * d)) / fs);
        const double mono = 0.5 * (l + r);
        haas[0].push (mono); haas[1].push (mono);
        const double dl = std::max (0.0, -p) * back * 0.0007 * fs, dr = std::max (0.0, p) * back * 0.0007 * fs;
        double xl = haas[0].tapFrac (dl), xr = haas[1].tapFrac (dr);
        lpL = xl + coef * (lpL - xl); lpR = xr + coef * (lpR - xr);
        const double g = 1.0 - 0.25 * back * d;
        l = lpL * gl * g;
        r = lpR * gr * g;
    }

    double getPosition() const { return position; }

private:
    double fs = 48000, depth = 0.7, shape = 0, position = 0, lpL = 0, lpR = 0;
    int mode = 0;
    Lfo lfo;
    DelayLine haas[2];
    Smoother sDepth;
};
} // namespace pn
