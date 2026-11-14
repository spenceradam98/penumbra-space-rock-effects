// Reverb stage: plate, spring and hall, behind a shared pre-delay.
//   Plate  - a figure-of-eight allpass/delay tank in the style of Dattorro's published plate
//            topology: dense, bright, with a slowly modulated tank (the 1970s studio plate).
//   Spring - two copies of the spring tank model from Tapehead Echo, with different spring
//            lengths for left and right (the amp / outboard spring unit).
//   Hall   - an 8-line feedback delay network with a Householder matrix, per-line damping and
//            slight modulation (a big room or echo chamber).
#pragma once

#include "Blocks.h"
#include "SpringReverb.h"
#include <array>

namespace pn
{
// Allpass with a modulated (fractional) delay, for the plate tank.
struct ModAllpass
{
    DelayLine line; double len = 100, exc = 8;
    void allocate (double n, double excursion) { len = n; exc = excursion; line.allocate ((int) (n + excursion) + 4); }
    void clear() { line.clear(); }
    inline double process (double x, double g, double mod)
    {
        const double d = line.tapFrac (std::max (1.0, len + exc * mod - 1.0));
        const double v = x - g * d;
        line.push (v);
        return d + g * v;
    }
};

class PlateReverb
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const double r = fs / 29761.0;
        auto n = [r] (double v) { return std::max (1, (int) std::lround (v * r)); };
        static const int inLen[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i) inAp[i].allocate (n (inLen[i]));
        apL1.allocate (672 * r, 16 * r); apR1.allocate (908 * r, 16 * r);
        dL1.allocate (n (4453)); dR1.allocate (n (4217));
        apL2.allocate (n (1800)); apR2.allocate (n (2656));
        dL2.allocate (n (3720)); dR2.allocate (n (3163));
        lenL1 = n (4453); lenR1 = n (4217); lenL2 = n (3720); lenR2 = n (3163);
        for (int i = 0; i < 14; ++i) taps[i] = n (kTapsRaw[i]);
        loopSec = (672.0 + 4453 + 1800 + 3720 + 908 + 4217 + 2656 + 3163) / 29761.0;
        bw.set (fs, 9000.0);
        modL.setRate (fs, 0.6); modR.setRate (fs, 0.71); modR.phase = 0.25;
        reset();
    }
    void reset()
    {
        for (auto& a : inAp) a.clear();
        apL1.clear(); apR1.clear(); apL2.clear(); apR2.clear();
        dL1.clear(); dR1.clear(); dL2.clear(); dR2.clear();
        bw.reset(); dampL = dampR = 0; endL = endR = 0;
    }
    void setDecay (double rt60, double damp)
    {
        decay = std::pow (10.0, -3.0 * loopSec / (4.0 * std::max (0.2, rt60)));
        decay = std::min (decay, 0.9995);
        damping = 0.05 + 0.75 * damp;   // one-pole coefficient in the tank
    }
    inline void process (double x, double& outL, double& outR)
    {
        double v = bw.process (x);
        v = inAp[0].process (v, 0.75);
        v = inAp[1].process (v, 0.75);
        v = inAp[2].process (v, 0.625);
        v = inAp[3].process (v, 0.625);

        const double mL = Lfo::sine (modL.tick()), mR = Lfo::sine (modR.tick());
        const double dd2 = clampd (decay + 0.15, 0.25, 0.5);

        // left half
        double a = apL1.process (v + decay * endR, -0.7, mL);
        dL1.push (a);
        double b = dL1.tap (lenL1 - 1);
        dampL = b + damping * (dampL - b);
        b = apL2.process (dampL * decay, dd2);
        dL2.push (b);
        const double newEndL = dL2.tap (lenL2 - 1);
        // right half
        double c = apR1.process (v + decay * endL, -0.7, mR);
        dR1.push (c);
        double d = dR1.tap (lenR1 - 1);
        dampR = d + damping * (dampR - d);
        d = apR2.process (dampR * decay, dd2);
        dR2.push (d);
        endR = dR2.tap (lenR2 - 1);
        endL = newEndL;

        outL = 0.6 * (dR1.tap (taps[0]) + dR1.tap (taps[1]) - apR2.tap (taps[2]) + dR2.tap (taps[3])
                      - dL1.tap (taps[4]) - apL2.tap (taps[5]) - dL2.tap (taps[6]));
        outR = 0.6 * (dL1.tap (taps[7]) + dL1.tap (taps[8]) - apL2.tap (taps[9]) + dL2.tap (taps[10])
                      - dR1.tap (taps[11]) - apR2.tap (taps[12]) - dR2.tap (taps[13]));
    }

private:
    static constexpr int kTapsRaw[14] = { 266, 2974, 1913, 1996, 1990, 187, 1066, 353, 3627, 1228, 2673, 2111, 335, 121 };
    double fs = 48000, decay = 0.5, damping = 0.3, loopSec = 0.7, dampL = 0, dampR = 0, endL = 0, endR = 0;
    DiffuserAP inAp[4], apL2, apR2;
    ModAllpass apL1, apR1;
    DelayLine dL1, dR1, dL2, dR2;
    int lenL1 = 1, lenR1 = 1, lenL2 = 1, lenR2 = 1;
    int taps[14] {};
    OnePoleLP bw;
    Lfo modL, modR;
};

class HallReverb
{
public:
    static constexpr int N = 8;
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        static const double ms[N] = { 31.7, 37.9, 41.3, 47.9, 53.3, 61.1, 67.7, 73.9 };
        for (int i = 0; i < N; ++i)
        {
            len[i] = ms[i] * 0.001 * fs * 1.25;
            lines[i].allocate ((int) (len[i] + 20));
        }
        inAp[0].allocate ((int) (0.0047 * fs)); inAp[1].allocate ((int) (0.0113 * fs)); inAp[2].allocate ((int) (0.0171 * fs));
        modA.setRate (fs, 0.37); modB.setRate (fs, 0.53);
        reset();
    }
    void reset()
    {
        for (auto& l : lines) l.clear();
        for (auto& a : inAp) a.clear();
        for (auto& d : damp) d = 0;
    }
    void setDecay (double rt60, double dampIn)
    {
        for (int i = 0; i < N; ++i) g[i] = std::pow (10.0, -3.0 * (len[i] / fs) / std::max (0.2, rt60));
        const double fc = 13000.0 * std::pow (2.0, -dampIn * 4.0);
        dampCoef = std::exp (-kTwoPi * fc / fs);
    }
    inline void process (double x, double& outL, double& outR)
    {
        double v = inAp[0].process (x, 0.7);
        v = inAp[1].process (v, 0.65);
        v = inAp[2].process (v, 0.6);
        const double ma = Lfo::sine (modA.tick()), mb = Lfo::sine (modB.tick());
        double y[N], sum = 0;
        for (int i = 0; i < N; ++i)
        {
            const double mod = i == 1 ? 9.0 * ma : (i == 5 ? 7.0 * mb : 0.0);
            double s = lines[i].tapFrac (len[i] + mod - 1.0);
            damp[i] = s + dampCoef * (damp[i] - s);
            y[i] = damp[i] * g[i];
            sum += y[i];
        }
        const double h = 2.0 / N * sum;
        outL = outR = 0;
        for (int i = 0; i < N; ++i)
        {
            const double fbv = y[i] - h + v * ((i & 1) ? -0.5 : 0.5);
            lines[i].push (fbv);
            if (i & 1) outR += (i & 2 ? -y[i] : y[i]);
            else outL += (i & 2 ? -y[i] : y[i]);
        }
        outL *= 0.5; outR *= 0.5;
    }

private:
    double fs = 48000, len[N] {}, g[N] {}, damp[N] {}, dampCoef = 0.3;
    DelayLine lines[N];
    DiffuserAP inAp[3];
    Lfo modA, modB;
};

class Reverb
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        plate.prepare (fs); hall.prepare (fs);
        springL.prepare (fs, 1.0, 0); springR.prepare (fs, 1.09, 4242);
        pre.allocate ((int) (0.26 * fs) + 4);
        inHp.highpass (fs, 70.0, 0.6);
        sMix.set (fs, 30.0); sPre.set (fs, 60.0);
        lastDecay = lastDamp = -1;
        reset();
    }
    void reset()
    {
        plate.reset(); hall.reset(); springL.reset(); springR.reset(); pre.clear(); inHp.reset();
        sMix.reset (mix); sPre.reset (preMs); springLpL.reset(); springLpR.reset();
    }
    void setParams (int typeIn, double decayS, double preDelayMs, double damp, double mixIn)
    {
        if (typeIn != type) { type = typeIn; plate.reset(); hall.reset(); springL.reset(); springR.reset(); }
        mix = mixIn; preMs = preDelayMs;
        if (std::abs (decayS - lastDecay) > 1e-3 || std::abs (damp - lastDamp) > 1e-4)
        {
            plate.setDecay (decayS, damp);
            hall.setDecay (decayS, damp);
            springL.setDecay (clampd (decayS, 0.4, 6.0)); springR.setDecay (clampd (decayS, 0.4, 6.0));
            springLpL.set (fs, 7000.0 * std::pow (2.0, -damp * 1.6)); springLpR.set (fs, 7000.0 * std::pow (2.0, -damp * 1.6));
            lastDecay = decayS; lastDamp = damp;
        }
    }
    inline void process (double& l, double& r)
    {
        const double m = sMix.next (mix);
        pre.push (inHp.process (0.5 * (l + r)));
        const double x = pre.tapFrac (std::max (0.0, sPre.next (preMs) * 0.001 * fs));
        double wl = 0, wr = 0;
        switch (type)
        {
            case 0: plate.process (x, wl, wr); break;
            case 1: wl = springLpL.process (springL.process (x)) * 1.6; wr = springLpR.process (springR.process (x)) * 1.6; break;
            default: hall.process (x, wl, wr); break;
        }
        l = l * dryGain (m) + wl * wetGain (m);
        r = r * dryGain (m) + wr * wetGain (m);
    }

private:
    double fs = 48000, mix = 0.25, preMs = 20, lastDecay = -1, lastDamp = -1;
    int type = 0;
    PlateReverb plate;
    HallReverb hall;
    SpringReverb springL, springR;
    OnePoleLP springLpL, springLpR;
    DelayLine pre;
    Biquad inHp;
    Smoother sMix, sPre;
};
} // namespace pn
