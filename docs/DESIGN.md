# Penumbra: design plan (v0.1)

*Working name. Penumbra is the half-shadow at the edge of an eclipse; it says "dark side of the
moon" without using the album's name. Easy to change: product name, company and codes sit in
`CMakeLists.txt`, and the UI title in `PluginEditor.cpp`.*

## Goal

One plugin that covers the studio effects palette of early-1970s space-rock production, for
guitars and synths: tape delay and tape-speed tricks, phasing/flanging/ADT, rotary speaker,
plate and spring reverb, fuzz and overdrive, tone shaping, VCS3-style ring-mod and filter
mangling, auto-pan / swirl, and reversed and looped tape. Presets in that spirit, named
descriptively (no song titles), and several UI looks.

## Signal chain

```
in -> Input -> Drive -> EQ -> Synthi -> Modulation -> Rotary -> Tape Delay -> Reverb -> Motion -> Output
```

Fixed order, the way the studio would have patched it (pedal-type effects into the amp/cab
stage, then the desk's tape and echo sends, then panning at the mix). Each module has its own
power LED; switching crossfades over ~15 ms, and a module that is off costs no CPU.

| Module | Modes / controls | What it models |
|---|---|---|
| Drive | Silicon Fuzz, Germanium Fuzz, Overdrive, Valve Amp; Drive, Tone, Level | Pre-filter, biased (asymmetric) tanh clipper with anti-aliasing (ADAA), post tone filter; voicing per pedal/amp type |
| EQ | Bass shelf, Mid bell + freq, Treble shelf, Low Cut (24 dB/oct) | Console channel EQ |
| Synthi | Ring Freq, Ring Mix, Cutoff, Resonance, LFO Rate/Depth, Envelope | VCS3-style use of a suitcase synth as an effects box: ring modulator into a self-oscillating 4-pole ladder filter, LFO sweep, envelope follower |
| Modulation | Phaser 4, Phaser 8, Flanger, ADT Doubler, Chorus; Rate, Depth, Feedback, Mix | Phasers with stereo LFO offset; tape-style flanging; ADT as a second "tape machine" with random speed wander, different per side |
| Rotary | Fast/Slow, Drive, Spread, Mix | Leslie-type: valve preamp, 800 Hz crossover, horn and drum rotors with separate inertia (the staggered speed-up/slow-down), Doppler + directivity, two virtual mics |
| Tape Delay | Tape / Ping-Pong / Reverse; Single / Dual / Triple heads; Time or tempo Sync (1/16 to 1 bar, incl. dotted and triplets); Feedback, Mix, Tone, Wow/Flutter, Glide; Freeze | Multi-head tape echo with saturating, darkening feedback (self-oscillates at the top); reverse tape grains; **Glide** = varispeed (moving Time bends the pitch of the repeats); **Freeze** = loop what is on the tape |
| Reverb | Plate / Spring / Hall; Decay, Pre-Delay, Damping, Mix | Dattorro-style plate tank, the spring tank from Tapehead Echo (two different springs for L/R), 8-line FDN hall |
| Motion | Auto-Pan / Tremolo / Swirl; Rate, Depth, Shape | Swirl gives a quad-joystick-style circle: pan plus darker, quieter, slightly delayed "behind" half |

Stereo in/out (mono in works), zero latency, all DSP in plain scalar C++17 in `Source/engine`
(no JUCE, no SIMD intrinsics, no `-march` flags), so it builds for macOS 11 Intel and arm64
and for Windows with the static runtime. About 2 % of one core with every module on at 48 kHz.

## Presets (25)

- **Basic:** Init (everything off, transparent)
- **Guitar (12):** Lunar Solo Lead, Breathing Clean Arpeggio, Rotating Speaker Solo, Doubled Rhythm, Slide Across the Sky, Clockwork Triple Echo, Phased Twelve-String, Wall of Fuzz, Spring Twang Blues, Flanged Jangle, Backwards Tape Solo, Odd-Meter Bass Throb
- **Synth (12):** Sequencer Swirl Run, Ring Mod Clatter, Tonewheel on Rotor, Choir Pad Plate, Filter Sweep Drone, Reverse Tape Swell, Tape Loop Bed, Heartbeat Pulse, Deep Space Hall, Varispeed Warp Lead, Hi-Hat Pan Run, Phaser Wash Pad

Presets are host programs too, so they show up in the DAW's preset menu.

## UI looks (5)

Spectrum (black, each module in its own spectral colour), Console (brushed-steel 70s desk,
bakelite knobs), Eclipse (night sky, orange corona glow), Moon Dust (light silver), Tape Room
(walnut cabinet, cream panels, chrome knobs). The look and window size are saved with the
session; the window scales freely from 50 % to 200 %.

Live indicators: spinning horn/drum on the Rotary panel, LFO dots on Synthi / Modulation /
Motion, the current tape delay time on Tape Delay, input/output meters.

## Deliberately avoided

- The album name, track titles and its cover art motif (the prism and beam) anywhere in the
  product. The Spectrum theme uses a plain colour rule, not that image.
- Real brand/model names in the UI (Leslie, VCS3, Binson, EMT, Big Muff and so on) are kept
  out of the plugin; the docs describe them only as "type of".

## Not yet done / next steps

- Listening pass and voicing tweaks on real guitar and synth material (presets were tuned by
  measurement, not by ear).
- User preset save/load, and A/B compare.
- Module order switching (e.g. delay before modulation), and a global dry/wet.
- Oversampling option for the drive and rotary stages (ADAA already removes most aliasing).
- GitHub repo, CI builds and installers for macOS and Windows (on request).
