// Rotary speaker (Leslie-type cabinet). The input is summed to mono, driven through a valve
// preamp, then split at 800 Hz (Linkwitz-Riley, 4th order) into a horn (treble) and a drum
// (bass) rotor. Each rotor has its own speed and inertia: the light horn reaches speed in
// under a second, the heavy drum takes several seconds, so switching speeds gives the
// familiar staggered ramp. Each rotor produces Doppler pitch shift (a modulated delay) and
// amplitude/directivity modulation, picked up by two virtual microphones whose angular
// spacing is the Spread control.
#pragma once

#include "Blocks.h"

namespace pn
{
class Rotary
{
public:
    static constexpr double kHornSlow = 0.83, kHornFast = 6.75, kDrumSlow = 0.67, kDrumFast = 5.9;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        hornDelay.allocate ((int) (0.01 * fs) + 8);
        drumDelay.allocate ((int) (0.01 * fs) + 8);
        lp1.lowpass (fs, 800.0, 0.7071); lp2.lowpass (fs, 800.0, 0.7071);
        hp1.highpass (fs, 800.0, 0.7071); hp2.highpass (fs, 800.0, 0.7071);
        hornShade.set (fs, 3500.0);
        sMix.set (fs, 30.0); sDrive.set (fs, 30.0);
        hornRate = kHornSlow; drumRate = kDrumSlow;
        reset();
    }
    void reset()
    {
        hornDelay.clear(); drumDelay.clear();
        lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset();
        hornShadeL = hornShadeR = 0;
        sMix.reset (mix); sDrive.reset (drive);
    }

    void setParams (bool fastIn, double driveIn, double mixIn, double spreadIn)
    {
        fast = fastIn; drive = driveIn; mix = mixIn; spread = spreadIn;
    }

    inline void process (double& l, double& r)
    {
        // rotor inertia: the horn accelerates in ~0.7 s, the drum in ~3.5 s (braking is slower)
        const double hT = fast ? kHornFast : kHornSlow, dT = fast ? kDrumFast : kDrumSlow;
        const double hTau = (hT > hornRate ? 0.45 : 0.75) * fs, dTau = (dT > drumRate ? 2.4 : 3.6) * fs;
        hornRate += (hT - hornRate) / hTau;
        drumRate += (dT - drumRate) / dTau;
        hornPhase += hornRate / fs; if (hornPhase >= 1.0) hornPhase -= 1.0;
        drumPhase += drumRate / fs; if (drumPhase >= 1.0) drumPhase -= 1.0;

        const double m = sMix.next (mix);
        const double dr = sDrive.next (drive);
        double x = 0.5 * (l + r);
        const double pg = 1.0 + 9.0 * dr * dr;
        x = std::tanh (x * pg + 0.1 * dr) - std::tanh (0.1 * dr);
        x /= std::max (1.0, 0.6 * pg);

        const double lo = lp2.process (lp1.process (x));
        const double hi = hp2.process (hp1.process (x));
        hornDelay.push (hi);
        drumDelay.push (lo);

        const double off = kPi * (0.15 + 0.85 * spread);   // mic angular spacing
        const double ha = kTwoPi * hornPhase, da = kTwoPi * drumPhase;
        // horn: ~0.35 ms Doppler excursion, strong directivity, darker when facing away
        auto horn = [&] (double a, double& shade)
        {
            const double dms = 1.2 + 0.35 * std::sin (a);
            const double v = hornDelay.tapFrac (dms * 0.001 * fs);
            const double facing = 0.5 + 0.5 * std::cos (a);
            shade = v + hornShade.a * (shade - v);   // one-pole: duller when the horn faces away
            return (facing * v + (1.0 - facing) * shade) * (0.35 + 0.65 * facing);
        };
        auto drum = [&] (double a)
        {
            const double dms = 1.0 + 0.12 * std::sin (a);
            return drumDelay.tapFrac (dms * 0.001 * fs) * (0.78 + 0.22 * std::cos (a));
        };
        const double wl = horn (ha - 0.5 * off, hornShadeL) * 1.25 + drum (da - 0.5 * off);
        const double wr = horn (ha + 0.5 * off, hornShadeR) * 1.25 + drum (da + 0.5 * off);
        l = l * dryGain (m) + wl * wetGain (m);
        r = r * dryGain (m) + wr * wetGain (m);
    }

    double getHornAngle() const { return hornPhase; }
    double getDrumAngle() const { return drumPhase; }

private:
    double fs = 48000, drive = 0.2, mix = 1, spread = 0.8;
    bool fast = false;
    double hornRate = kHornSlow, drumRate = kDrumSlow, hornPhase = 0, drumPhase = 0.3;
    double hornShadeL = 0, hornShadeR = 0;
    DelayLine hornDelay, drumDelay;
    Biquad lp1, lp2, hp1, hp2;
    OnePoleLP hornShade;
    Smoother sMix, sDrive;
};
} // namespace pn
