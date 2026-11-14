// Tape delay with the 1970s studio tape tricks:
//   - Tape mode: one, two or three playback heads (1, 3/4 + 1, or 1/3 + 2/3 + 1 of the time),
//     feedback from the last head through a band-limited, saturating record path, so repeats
//     darken and the loop can be driven into self-oscillation that the tape holds in check.
//   - Ping-Pong: the loop alternates between the left and right machines.
//   - Reverse: the delayed signal is played backwards in overlapping windowed grains as long as
//     the delay time (a reversed tape loop), with feedback, so swells repeat backwards.
//   - Glide: the transport speed follows the Time knob slowly, so moving Time (or tempo) bends
//     the pitch of everything on the tape, as with a varispeed machine.
//   - Wow & flutter: slow random speed drift plus capstan flutter.
//   - Freeze: stop recording and loop exactly what is on the tape (a tape loop).
#pragma once

#include "Blocks.h"
#include "Params.h"

namespace pn
{
class TapeDelay
{
public:
    static constexpr double kMaxSeconds = 4.5;
    static constexpr double kMaxTimeMs = 1900.0;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (int c = 0; c < 2; ++c)
        {
            ch[c].line.allocate ((int) (kMaxSeconds * fs) + 16);
            ch[c].hp.highpass (fs, 90.0, 0.6);
            ch[c].wow.set (fs, 0.9, 0x7EEDu + 31u * (uint32_t) c);
        }
        sMix.set (fs, 30.0); sFb.set (fs, 30.0); sFreeze.set (fs, 15.0);
        lastTone = -1;
        reset();
    }
    void reset()
    {
        for (auto& c : ch) { c.line.clear(); c.hp.reset(); c.lp.reset(); c.fbSig = 0; }
        sTime.reset (targetSamples()); sMix.reset (mix); sFb.reset (fb); sFreeze.reset (freeze ? 1 : 0);
        flutterPhase = 0; grainPhase = 0;
    }

    void setParams (int modeIn, int headsIn, double timeMsIn, double fbIn, double mixIn, double wowIn, double toneIn,
                    double glideIn, bool freezeIn)
    {
        if (modeIn != mode) { mode = modeIn; for (auto& c : ch) c.fbSig = 0; }
        heads = headsIn;
        timeMs = clampd (timeMsIn, 20.0, kMaxTimeMs);
        fb = fbIn; mix = mixIn; wow = wowIn; freeze = freezeIn;
        if (std::abs (toneIn - lastTone) > 1e-4)
        {
            const double hz = 1200.0 * std::pow (2.0, toneIn * 3.4);   // 1.2 kHz .. 12.7 kHz
            for (auto& c : ch) c.lp.lowpass (fs, hz, 0.6);
            lastTone = toneIn;
        }
        // glide: 30 ms (tight) .. ~2.5 s (big varispeed swoops)
        sTime.set (fs, 30.0 + glideIn * glideIn * 2500.0);
    }

    inline void process (double& l, double& r)
    {
        const double D = sTime.next (targetSamples());
        const double m = sMix.next (mix);
        const double fz = sFreeze.next (freeze ? 1.0 : 0.0);
        const double g = sFb.next (fb) * 1.04;

        flutterPhase += 5.7 / fs; if (flutterPhase >= 1.0) flutterPhase -= 1.0;
        const double flutter = std::sin (kTwoPi * flutterPhase);
        double modS[2];
        for (int c = 0; c < 2; ++c)
            modS[c] = wow * (1.6 * ch[c].wow.process() + 0.22 * flutter) * 0.001 * fs * std::min (1.0, D / (0.1 * fs));

        double wet[2] = { 0, 0 };
        const double in[2] = { l, r };

        if (mode == 2)
        {
            // reverse: two grains, half a grain apart, each reading backwards through the last
            // D samples; sin^2 windows sum to one.
            const double len = std::max (64.0, D);
            grainPhase += 1.0 / len; if (grainPhase >= 1.0) grainPhase -= 1.0;
            for (int c = 0; c < 2; ++c)
            {
                auto& s = ch[c];
                double v = 0;
                for (int k = 0; k < 2; ++k)
                {
                    const double p = Lfo::wrap (grainPhase + 0.5 * k);
                    const double w = std::sin (kPi * p);
                    v += w * w * s.line.tapFrac (std::max (1.0, 2.0 * len * p + modS[c]));
                }
                wet[c] = v;
                const double rec = in[c] + g * shape (s, v);
                const double loop = s.line.tap ((int) (2.0 * len) - 1);
                s.line.push (fz * loop + (1.0 - fz) * rec);
            }
        }
        else
        {
            static const double headPos[3][3] = { { 1.0, 0, 0 }, { 0.75, 1.0, 0 }, { 1.0 / 3.0, 2.0 / 3.0, 1.0 } };
            const int nh = mode == 1 ? 1 : heads + 1;
            const double norm = nh == 1 ? 1.0 : (nh == 2 ? 0.8 : 0.66);
            double last[2];
            for (int c = 0; c < 2; ++c)
            {
                auto& s = ch[c];
                double sum = 0;
                for (int h = 0; h < nh; ++h)
                {
                    const double pos = (mode == 1 ? 1.0 : headPos[heads][h]) * D + modS[c];
                    const double v = s.line.tapFrac (std::max (1.0, pos));
                    sum += v;
                    if (h == nh - 1) last[c] = v;
                }
                wet[c] = sum * norm;
            }
            const int loopLen = std::max (1, (int) std::lround (D)) - 1;
            if (mode == 1)
            {
                const double mono = 0.5 * (l + r);
                const double recL = mono + g * shape (ch[0], last[1]);
                const double recR = g * shape (ch[1], last[0]);
                const double loopL = ch[0].line.tap (loopLen), loopR = ch[1].line.tap (loopLen);
                ch[0].line.push (fz * loopR + (1.0 - fz) * recL);   // frozen ping-pong keeps alternating
                ch[1].line.push (fz * loopL + (1.0 - fz) * recR);
            }
            else
            {
                for (int c = 0; c < 2; ++c)
                {
                    auto& s = ch[c];
                    const double rec = in[c] + g * shape (s, last[c]);
                    s.line.push (fz * s.line.tap (loopLen) + (1.0 - fz) * rec);
                }
            }
        }

        l = l * dryGain (m) + wet[0] * wetGain (m);
        r = r * dryGain (m) + wet[1] * wetGain (m);
        lastWet = 0.5 * (std::abs (wet[0]) + std::abs (wet[1]));
    }

    double currentDelayMs() const { return sTime.y / fs * 1000.0; }
    void setTempo (double bpmIn) { bpm = bpmIn > 20 ? bpmIn : 120.0; }
    void setSync (bool s, int div) { sync = s; division = div < 0 ? 0 : (div >= kNumDelayDivisions ? kNumDelayDivisions - 1 : div); }
    double effectiveTimeMs() const
    {
        if (! sync) return timeMs;
        return clampd (60000.0 / bpm * kDelayDivisionBeats[division], 20.0, kMaxTimeMs);
    }

private:
    struct Chan { DelayLine line; Biquad hp, lp; Drift wow; double fbSig = 0; };

    // record path for the feedback signal: band limit and tape saturation
    inline double shape (Chan& s, double v)
    {
        v = s.lp.process (s.hp.process (v));
        return std::tanh (v * 1.25) * 0.8;
    }
    double targetSamples() const { return effectiveTimeMs() * 0.001 * fs; }

    Chan ch[2];
    double fs = 48000, timeMs = 400, fb = 0.35, mix = 0.35, wow = 0.25, lastTone = -1, bpm = 120;
    double flutterPhase = 0, grainPhase = 0, lastWet = 0;
    int mode = 0, heads = 0, division = 5;
    bool freeze = false, sync = false;
    Smoother sTime, sMix, sFb, sFreeze;
};
} // namespace pn
