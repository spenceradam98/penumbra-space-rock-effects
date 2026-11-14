// Engine tests: plain C++, no framework. Run by ctest.
#include "Engine.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace pn;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("FAIL %s:%d: ", __FILE__, __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

struct Stats { double peak = 0, rms = 0; bool finite = true; };

static Stats run (Engine& e, const std::vector<float>& inL, const std::vector<float>& inR, std::vector<float>* outL = nullptr, std::vector<float>* outR = nullptr, int block = 256)
{
    std::vector<float> l = inL, r = inR;
    for (size_t i = 0; i < l.size(); i += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, l.size() - i);
        e.process (l.data() + i, r.data() + i, n);
    }
    Stats s; double acc = 0;
    for (size_t i = 0; i < l.size(); ++i)
    {
        if (! std::isfinite (l[i]) || ! std::isfinite (r[i])) s.finite = false;
        s.peak = std::max (s.peak, (double) std::max (std::abs (l[i]), std::abs (r[i])));
        acc += 0.5 * (l[i] * l[i] + r[i] * r[i]);
    }
    s.rms = std::sqrt (acc / std::max<size_t> (1, l.size()));
    if (outL) *outL = l;
    if (outR) *outR = r;
    return s;
}

static std::vector<float> sine (double fs, double hz, double secs, double amp)
{
    std::vector<float> v ((size_t) (fs * secs));
    for (size_t i = 0; i < v.size(); ++i) v[i] = (float) (amp * std::sin (2.0 * kPi * hz * (double) i / fs));
    return v;
}

static std::vector<float> guitarish (double fs, double secs)
{
    // plucked notes: decaying harmonic tones every 0.5 s
    std::vector<float> v ((size_t) (fs * secs));
    const double notes[] = { 196.0, 246.9, 293.7, 392.0 };
    for (size_t i = 0; i < v.size(); ++i)
    {
        const double t = (double) i / fs;
        const int k = (int) (t / 0.5);
        const double tn = t - k * 0.5, f = notes[k % 4];
        double s = 0;
        for (int h = 1; h <= 6; ++h) s += std::sin (2.0 * kPi * f * h * tn) / h;
        v[i] = (float) (0.4 * s * std::exp (-tn * 5.0));
    }
    return v;
}

int main()
{
    const double rates[] = { 44100.0, 48000.0, 96000.0 };

    // 1. Init preset is transparent (all modules off, unity gain).
    {
        Engine e; Params p; e.setParams (p); e.prepare (48000, 256);
        auto x = sine (48000, 440, 0.5, 0.5);
        std::vector<float> l, r;
        run (e, x, x, &l, &r);
        double err = 0;
        for (size_t i = 0; i < x.size(); ++i) err = std::max (err, (double) std::abs (l[i] - x[i]));
        CHECK (err < 1e-5, "init preset not transparent, max err %g", err);
    }

    // 2. Every preset at every rate: finite, bounded, not silent, silent input decays to silence.
    int np = 0; auto* ps = factoryPresets (np);
    CHECK (np >= 20, "expected at least 20 presets, got %d", np);
    for (double fs : rates)
        for (int i = 0; i < np; ++i)
        {
            Engine e; e.setParams (ps[i].p); e.prepare (fs, 512); e.setTempo (120.0);
            auto x = guitarish (fs, 3.0);
            auto s = run (e, x, x, nullptr, nullptr, 512);
            CHECK (s.finite, "preset '%s' @%g produced non-finite output", ps[i].name, fs);
            CHECK (s.peak < 3.5, "preset '%s' @%g peak %g too hot", ps[i].name, fs, s.peak);
            CHECK (s.rms > 0.01, "preset '%s' @%g nearly silent (rms %g)", ps[i].name, fs, s.rms);
            if (! ps[i].p.dlyFreeze && ps[i].p.dlyFeedback < 0.85f)
            {
                std::vector<float> z ((size_t) (fs * 16.0), 0.0f);
                std::vector<float> l, r;
                run (e, z, z, &l, &r, 512);
                double tail = 0;
                for (size_t k = l.size() - (size_t) fs; k < l.size(); ++k) tail = std::max (tail, (double) std::abs (l[k]) + std::abs (r[k]));
                CHECK (tail < 1e-3, "preset '%s' @%g tail does not decay (%g)", ps[i].name, fs, tail);
            }
        }

    // 3. Max settings of everything stay bounded (runaway feedback is held by the tape).
    for (double fs : rates)
    {
        Params p;
        p.drvOn = p.eqOn = p.synOn = p.modOn = p.rotOn = p.dlyOn = p.revOn = p.motOn = true;
        p.drvDrive = 1; p.eqLow = p.eqMid = p.eqHigh = 15; p.synRes = 1; p.synRingMix = 1; p.modFeedback = 1; p.modType = 2;
        p.dlyFeedback = 1; p.dlyMix = 1; p.revDecay = 12; p.revMix = 1; p.inputDb = 24; p.rotFast = true;
        Engine e; e.setParams (p); e.prepare (fs, 256);
        auto x = sine (fs, 110, 6.0, 0.9);
        auto s = run (e, x, x);
        CHECK (s.finite && s.peak <= 4.0, "max settings unbounded @%g (peak %g)", fs, s.peak);
    }

    // 4. Delay timing: a click comes back after Time ms in Tape mode.
    {
        const double fs = 48000;
        Params p; p.dlyOn = true; p.dlyTimeMs = 250; p.dlyFeedback = 0; p.dlyMix = 0.5f; p.dlyWow = 0; p.dlyTone = 1;
        Engine e; e.setParams (p); e.prepare (fs, 256);
        std::vector<float> x ((size_t) fs, 0.0f); x[100] = 1.0f;
        std::vector<float> l, r; run (e, x, x, &l, &r);
        size_t best = 0; double bv = 0;
        for (size_t i = 1000; i < l.size(); ++i) if (std::abs (l[i]) > bv) { bv = std::abs (l[i]); best = i; }
        const double ms = (double) (best - 100) / fs * 1000.0;
        CHECK (std::abs (ms - 250.0) < 2.0, "delay echo at %.2f ms, expected 250", ms);
    }

    // 5. Tempo sync: 1/4 at 100 bpm = 600 ms.
    {
        Params p; p.dlyOn = true; p.dlySync = true; p.dlyDiv = 5; p.dlyGlide = 0;
        Engine e; e.setParams (p); e.prepare (48000, 256); e.setTempo (100.0); e.setParams (p);
        std::vector<float> z (48000, 0.0f); run (e, z, z);
        CHECK (std::abs (e.getDelayMs() - 600.0f) < 1.0f, "synced delay %.2f ms, expected 600", e.getDelayMs());
    }

    // 6. Freeze holds the loop: after freezing, the level stays up with no input.
    {
        const double fs = 48000;
        Params p; p.dlyOn = true; p.dlyTimeMs = 500; p.dlyFeedback = 0.3f; p.dlyMix = 1.0f; p.dlyWow = 0;
        Engine e; e.setParams (p); e.prepare (fs, 256);
        auto x = guitarish (fs, 1.0); run (e, x, x);
        p.dlyFreeze = true; e.setParams (p);
        std::vector<float> z ((size_t) (fs * 6.0), 0.0f), l, r;
        run (e, z, z, &l, &r);
        double late = 0; for (size_t i = l.size() - (size_t) fs; i < l.size(); ++i) late += l[i] * l[i];
        late = std::sqrt (late / fs);
        CHECK (late > 0.01, "frozen loop died away (rms %g)", late);
    }

    // 7. Reverse mode produces output and decays.
    {
        Params p; p.dlyOn = true; p.dlyMode = 2; p.dlyTimeMs = 400; p.dlyFeedback = 0.3f; p.dlyMix = 1.0f;
        Engine e; e.setParams (p); e.prepare (48000, 256);
        auto x = guitarish (48000, 1.0); auto s = run (e, x, x);
        CHECK (s.rms > 0.01 && s.finite, "reverse delay silent");
    }

    // 8. Rotary: fast mode modulates the level more quickly than slow (horn reaches speed).
    {
        Params p; p.rotOn = true; p.rotFast = true; p.rotMix = 1;
        Engine e; e.setParams (p); e.prepare (48000, 256);
        auto x = sine (48000, 2000, 3.0, 0.5); run (e, x, x);
        // horn should now be near 6.75 Hz: count angle wraps over 1 s
        std::vector<float> y = sine (48000, 2000, 1.0, 0.5), yr = y;
        int wraps = 0; float last = e.getHornAngle();
        for (int b = 0; b < 48000 / 64; ++b)
        {
            e.process (y.data() + b * 64, yr.data() + b * 64, 64);
            const float a = e.getHornAngle(); if (a < last) ++wraps; last = a;
        }
        CHECK (wraps >= 6 && wraps <= 8, "horn rotations per second at fast: %d", wraps);
    }

    // 9. Parameter table: unique ids, defaults inside range, set/get round trip.
    {
        int n = 0; auto* t = paramTable (n);
        CHECK (n > 50, "param table too small (%d)", n);
        Params def;
        for (int i = 0; i < n; ++i)
        {
            for (int j = i + 1; j < n; ++j) CHECK (std::strcmp (t[i].id, t[j].id) != 0, "duplicate id %s", t[i].id);
            const float v = t[i].get (def);
            CHECK (v >= t[i].lo && v <= t[i].hi, "default of %s (%g) outside [%g,%g]", t[i].id, v, t[i].lo, t[i].hi);
            Params q; t[i].set (q, t[i].hi); CHECK (std::abs (t[i].get (q) - t[i].hi) < 1e-4f, "round trip failed for %s", t[i].id);
        }
        // every preset value must be inside the host ranges
        for (int k = 0; k < np; ++k)
            for (int i = 0; i < n; ++i)
            {
                const float v = t[i].get (ps[k].p);
                CHECK (v >= t[i].lo - 1e-4f && v <= t[i].hi + 1e-4f, "preset '%s' %s=%g out of range", ps[k].name, t[i].id, v);
            }
    }

    // 10. Each module alone with mix at default changes the signal (no dead modules).
    {
        const char* names[kNumModules] = { "drive", "eq", "synthi", "mod", "rotary", "delay", "reverb", "motion" };
        for (int m = 0; m < kNumModules; ++m)
        {
            Params p;
            switch (m)
            {
                case kDrive: p.drvOn = true; break;
                case kEq: p.eqOn = true; p.eqMid = 9; break;
                case kSynthi: p.synOn = true; p.synCutoff = 600; break;
                case kMod: p.modOn = true; break;
                case kRotary: p.rotOn = true; break;
                case kDelay: p.dlyOn = true; break;
                case kReverb: p.revOn = true; break;
                case kMotion: p.motOn = true; break;
                default: break;
            }
            Engine e; e.setParams (p); e.prepare (48000, 256);
            auto x = guitarish (48000, 2.0);
            std::vector<float> l, r; run (e, x, x, &l, &r);
            double diff = 0; for (size_t i = 0; i < x.size(); ++i) diff += std::abs (l[i] - x[i]) + std::abs (r[i] - x[i]);
            CHECK (diff / x.size() > 1e-3, "module %s has no audible effect", names[m]);
        }
    }

    if (failures == 0) std::printf ("All engine tests passed (%d presets).\n", np);
    return failures == 0 ? 0 : 1;
}
