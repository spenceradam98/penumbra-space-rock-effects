#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr float kStart = MathConstants<float>::pi * -0.75f, kEnd = MathConstants<float>::pi * 0.75f;

const Theme kThemes[] = {
    { "Spectrum",   Colour (0xff0b0b0d), Colour (0xff16161a), Colour (0xff121216), Colour (0xff2a2a31), Colour (0xfff2f2f2), Colour (0xff8a8a94),
                    Colour (0xffffffff), Colour (0xff1d1d22), Colour (0xff2c2c33), Colour (0xffffffff), 0, true, false },
    { "Console",    Colour (0xff383b40), Colour (0xff2c2e33), Colour (0xff474b52), Colour (0xff25272b), Colour (0xffefe8d8), Colour (0xffb8b2a4),
                    Colour (0xffe8a33a), Colour (0xff141414), Colour (0xff2a2a2a), Colour (0xffefe8d8), 1, false, false },
    { "Eclipse",    Colour (0xff06070d), Colour (0xff0d0f1d), Colour (0xff0e1020), Colour (0xff252a4a), Colour (0xffe8e6ff), Colour (0xff8a88b0),
                    Colour (0xffff9a3c), Colour (0xff0a0b16), Colour (0xff171a30), Colour (0xffffc88a), 2, false, false },
    { "Moon Dust",  Colour (0xffd5d6da), Colour (0xffc4c6cc), Colour (0xffededf0), Colour (0xffb0b2b9), Colour (0xff1d1e22), Colour (0xff666870),
                    Colour (0xff3a6df0), Colour (0xff1a1a1d), Colour (0xff303035), Colour (0xffffffff), 3, false, true },
    { "Tape Room",  Colour (0xff4a2e1c), Colour (0xff37220f), Colour (0xffefe4cc), Colour (0xffb59c74), Colour (0xff3a2a1a), Colour (0xff7a654a),
                    Colour (0xffb8401f), Colour (0xffd9dadc), Colour (0xff8d9096), Colour (0xff2a2a2a), 4, false, true },
};

// spectral colours for the eight modules (Spectrum theme)
const Colour kSpectrum[8] = { Colour (0xffff4b4b), Colour (0xffff9a3c), Colour (0xffffd83c), Colour (0xff58d66a),
                              Colour (0xff3cc8e8), Colour (0xff4b7bff), Colour (0xff8a5bff), Colour (0xffd25bff) };
} // namespace

const Theme& themeAt (int i) { return kThemes[jlimit (0, numThemes() - 1, i)]; }
int numThemes() { return (int) (sizeof (kThemes) / sizeof (kThemes[0])); }

// ============================================================================ LookAndFeel
void PenumbraLookAndFeel::setTheme (int index)
{
    th = &themeAt (index);
    const auto& t = *th;
    const Colour boxBg = t.light ? t.panel.darker (0.06f) : t.bg.brighter (0.06f);
    setColour (ComboBox::backgroundColourId, boxBg);
    setColour (ComboBox::textColourId, t.text);
    setColour (ComboBox::outlineColourId, t.panelEdge);
    setColour (ComboBox::arrowColourId, t.dim);
    setColour (PopupMenu::backgroundColourId, t.light ? t.panel : t.bg2);
    setColour (PopupMenu::textColourId, t.text);
    setColour (PopupMenu::headerTextColourId, t.accent);
    setColour (PopupMenu::highlightedBackgroundColourId, t.accent.withAlpha (0.3f));
    setColour (PopupMenu::highlightedTextColourId, t.text);
    setColour (TextButton::textColourOffId, t.text);
    setColour (TextButton::textColourOnId, t.text);
    setColour (Slider::textBoxTextColourId, t.text);
}

void PenumbraLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    const auto& t = *th;
    const Colour acc = s.findColour (Slider::rotarySliderFillColourId);
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float d = jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float r = d * 0.5f;
    const float angle = start + pos * (end - start);
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroAngle = bipolar ? start + (float) s.valueToProportionOfLength (0.0) * (end - start) : start;

    auto arc = [&] (float radius, float a0, float a1, Colour col, float thick)
    {
        Path p; p.addCentredArc (c.x, c.y, radius, radius, 0.0f, a0, a1, true);
        g.setColour (col); g.strokePath (p, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
    };
    auto pointer = [&] (float r0, float r1, Colour col, float thick)
    {
        g.setColour (col);
        g.drawLine (Line<float> (c.getPointOnCircumference (r0, angle), c.getPointOnCircumference (r1, angle)), thick);
    };

    switch (t.style)
    {
        case 0:   // Spectrum: flat disc, coloured value arc
        {
            arc (r - 2.0f, start, end, t.panelEdge, 3.0f);
            arc (r - 2.0f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc, 3.0f);
            auto body = Rectangle<float> (d * 0.68f, d * 0.68f).withCentre (c);
            g.setColour (t.knob); g.fillEllipse (body);
            g.setColour (t.knobCap); g.drawEllipse (body, 1.0f);
            pointer (d * 0.08f, d * 0.32f, t.pointer, 2.2f);
            break;
        }
        case 1:   // Console: black bakelite with a coloured cap
        {
            g.setColour (Colours::black.withAlpha (0.45f)); g.fillEllipse (Rectangle<float> (d * 0.86f, d * 0.86f).withCentre (c.translated (1.5f, 3.0f)));
            for (int i = 0; i <= 10; ++i)
            {
                const float a = start + (end - start) * i / 10.0f;
                g.setColour (t.dim.withAlpha (0.8f));
                g.drawLine (Line<float> (c.getPointOnCircumference (r, a), c.getPointOnCircumference (r - 3.5f, a)), 1.0f);
            }
            auto skirt = Rectangle<float> (d * 0.82f, d * 0.82f).withCentre (c);
            g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), skirt.getX(), skirt.getY(), Colour (0xff050505), skirt.getRight(), skirt.getBottom(), false));
            g.fillEllipse (skirt);
            auto cap = skirt.reduced (d * 0.17f);
            g.setGradientFill (ColourGradient (acc.brighter (0.3f), cap.getX(), cap.getY(), acc.darker (0.6f), cap.getRight(), cap.getBottom(), false));
            g.fillEllipse (cap);
            pointer (d * 0.12f, d * 0.40f, t.pointer, 2.5f);
            break;
        }
        case 2:   // Eclipse: dark disc, glowing corona arc
        {
            for (int i = 3; i >= 1; --i) arc (r - 3.0f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc.withAlpha (0.12f), 3.0f + i * 3.0f);
            arc (r - 3.0f, start, end, t.panelEdge, 2.0f);
            arc (r - 3.0f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc, 2.5f);
            auto body = Rectangle<float> (d * 0.64f, d * 0.64f).withCentre (c);
            g.setGradientFill (ColourGradient (t.knobCap, body.getCentreX(), body.getY(), t.knob, body.getCentreX(), body.getBottom(), false));
            g.fillEllipse (body);
            g.setColour (acc.withAlpha (0.35f)); g.drawEllipse (body, 1.0f);
            g.setColour (t.pointer); g.fillEllipse (Rectangle<float> (4.0f, 4.0f).withCentre (c.getPointOnCircumference (d * 0.24f, angle)));
            break;
        }
        case 3:   // Moon Dust: knurled black knob, white line, blue value arc
        {
            arc (r - 1.5f, start, end, t.panelEdge, 2.0f);
            arc (r - 1.5f, jmin (zeroAngle, angle), jmax (zeroAngle, angle), acc, 2.0f);
            auto body = Rectangle<float> (d * 0.76f, d * 0.76f).withCentre (c);
            g.setColour (Colours::black.withAlpha (0.25f)); g.fillEllipse (body.translated (0.0f, 2.0f));
            g.setColour (t.knob); g.fillEllipse (body);
            g.setColour (Colours::white.withAlpha (0.08f));
            for (int i = 0; i < 24; ++i)
            {
                const float a = angle + MathConstants<float>::twoPi * i / 24.0f;
                g.drawLine (Line<float> (c.getPointOnCircumference (d * 0.38f, a), c.getPointOnCircumference (d * 0.32f, a)), 1.2f);
            }
            pointer (d * 0.06f, d * 0.36f, t.pointer, 2.2f);
            break;
        }
        default:  // Tape Room: chrome skirt knob with black centre
        {
            for (int i = 0; i <= 10; ++i)
            {
                const float a = start + (end - start) * i / 10.0f;
                g.setColour (t.dim);
                g.drawLine (Line<float> (c.getPointOnCircumference (r, a), c.getPointOnCircumference (r - 3.0f, a)), 1.0f);
            }
            auto skirt = Rectangle<float> (d * 0.80f, d * 0.80f).withCentre (c);
            g.setColour (Colours::black.withAlpha (0.35f)); g.fillEllipse (skirt.translated (1.0f, 2.5f));
            g.setGradientFill (ColourGradient (Colour (0xffffffff), skirt.getX(), skirt.getY(), Colour (0xff6a6c70), skirt.getRight(), skirt.getBottom(), false));
            g.fillEllipse (skirt);
            g.setColour (Colour (0xff1a1a1a));
            g.drawLine (Line<float> (c.getPointOnCircumference (d * 0.40f, angle), c.getPointOnCircumference (d * 0.29f, angle)), 2.5f);
            auto cap = skirt.reduced (d * 0.16f);
            g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), cap.getX(), cap.getY(), Colour (0xff0a0a0a), cap.getRight(), cap.getBottom(), false));
            g.fillEllipse (cap);
            pointer (d * 0.04f, d * 0.22f, Colour (0xffefe4cc), 2.0f);
            break;
        }
    }
}

void PenumbraLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    const auto& t = *th;
    const Colour acc = b.findColour (ToggleButton::tickColourId);
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const bool isPower = b.getProperties()["power"];
    if (isPower)
    {
        // round LED power button
        auto led = Rectangle<float> (14.0f, 14.0f).withCentre (r.getCentre());
        if (on)
        {
            g.setColour (acc.withAlpha (0.25f)); g.fillEllipse (led.expanded (4.0f));
            g.setColour (acc); g.fillEllipse (led);
            g.setColour (Colours::white.withAlpha (0.5f)); g.fillEllipse (led.reduced (4.0f).translated (-1.0f, -1.0f));
        }
        else
        {
            g.setColour (t.light ? t.panelEdge : t.bg); g.fillEllipse (led);
            g.setColour (t.dim.withAlpha (highlighted ? 0.9f : 0.5f)); g.drawEllipse (led, 1.2f);
        }
        return;
    }
    // pill switch with label
    g.setColour (on ? acc.withAlpha (0.22f) : (t.light ? t.panel.darker (0.05f) : t.bg.withAlpha (0.6f)));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? acc : t.panelEdge.brighter (highlighted ? 0.3f : 0.0f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    auto dot = Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getX() + 9.0f, r.getCentreY() });
    g.setColour (on ? acc : t.dim.withAlpha (0.5f)); g.fillEllipse (dot);
    g.setColour (on ? t.text : t.dim);
    g.setFont (FontOptions (11.5f, Font::bold));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (16.0f).withTrimmedRight (3.0f), Justification::centred);
}

void PenumbraLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    const auto& t = *th;
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour ((t.light ? t.panel.darker (0.05f) : t.bg.brighter (0.06f)).brighter (down ? 0.15f : 0.0f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (highlighted ? t.accent : t.panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void PenumbraLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    const auto& t = *th;
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (false) || box.isMouseOver (true) ? t.accent.withAlpha (0.8f) : t.panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    Path arrow;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (t.dim); g.fillPath (arrow);
}

Font PenumbraLookAndFeel::getComboBoxFont (ComboBox&) { return FontOptions (13.0f); }
Font PenumbraLookAndFeel::getPopupMenuFont() { return FontOptions (14.0f); }

// ============================================================================ Knob
Knob::Knob (PenumbraProcessor& p, const String& id, const String& lbl) : label (lbl)
{
    param = p.apvts.getParameter (id);
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (kStart, kEnd, true);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    addAndMakeVisible (slider);
    attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, slider);
    if (param != nullptr)
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
    slider.setTooltip (param != nullptr ? param->getName (64) : lbl);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (28);
    slider.setBounds (r);
}

void Knob::paint (Graphics& g)
{
    auto& lf = dynamic_cast<PenumbraLookAndFeel&> (getLookAndFeel());
    const auto& t = lf.theme();
    auto r = getLocalBounds().removeFromBottom (28);
    const bool onWood = onHeader && t.style == 4;   // header knobs sit on the walnut in Tape Room
    g.setColour (onWood ? t.panel : t.text);
    g.setFont (FontOptions (11.5f, Font::bold));
    g.drawText (label.toUpperCase(), r.removeFromTop (14), Justification::centred, false);
    g.setColour (slider.isMouseOverOrDragging() ? accent : (onWood ? t.panel.withAlpha (0.75f) : t.dim));
    g.setFont (FontOptions (11.0f));
    g.drawText (param != nullptr ? param->getCurrentValueAsText() : String(), r, Justification::centred, false);
}

// ============================================================================ ModulePanel
ModulePanel::ModulePanel (PenumbraProcessor& p, int m, const String& t, const String& onId, int cols)
    : columns (cols), module (m), proc (p), title (t)
{
    power.getProperties().set ("power", true);
    power.setTooltip ("Switch " + t + " on or off");
    addAndMakeVisible (power);
    powerAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, onId, power);
    power.onStateChange = [this] { repaint(); };
}

void ModulePanel::addKnob (const String& id, const String& label)
{
    knobs.push_back (std::make_unique<Knob> (proc, id, label));
    addAndMakeVisible (*knobs.back());
}

void ModulePanel::addChoice (const String& id)
{
    auto box = std::make_unique<ComboBox>();
    if (auto* prm = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
        box->addItemList (prm->choices, 1);
    addAndMakeVisible (*box);
    comboAttach.push_back (std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *box));
    combos.push_back (std::move (box));
}

void ModulePanel::addToggle (const String& id, const String& label)
{
    auto b = std::make_unique<ToggleButton> (label);
    addAndMakeVisible (*b);
    toggleAttach.push_back (std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *b));
    toggles.push_back (std::move (b));
}

void ModulePanel::setAccent (Colour c)
{
    accent = c;
    power.setColour (ToggleButton::tickColourId, c);
    for (auto& k : knobs) { k->accent = c; k->slider.setColour (Slider::rotarySliderFillColourId, c); }
    for (auto& t : toggles) t->setColour (ToggleButton::tickColourId, c);
    repaint();
}

void ModulePanel::resized()
{
    auto r = getLocalBounds().reduced (8, 6);
    auto header = r.removeFromTop (24);
    power.setBounds (header.removeFromLeft (22));
    r.removeFromTop (4);

    const int nCtl = (int) (combos.size() + toggles.size());
    if (nCtl > 0)
    {
        auto row = r.removeFromTop (24);
        const int w = (row.getWidth() - (nCtl - 1) * 6) / nCtl;
        for (auto& c : combos) { c->setBounds (row.removeFromLeft (w)); row.removeFromLeft (6); }
        for (auto& t : toggles) { t->setBounds (row.removeFromLeft (w).withSizeKeepingCentre (jmin (w, 110), row.getHeight())); row.removeFromLeft (6); }
        r.removeFromTop (6);
    }
    else
        r.removeFromTop (8);

    const int rows = ((int) knobs.size() + columns - 1) / columns;
    if (rows == 0) return;
    const int cw = r.getWidth() / columns, rh = jmin (r.getHeight() / rows, 104);
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int row = (int) i / columns, col = (int) i % columns;
        // centre a short last row
        const int inRow = jmin (columns, (int) knobs.size() - row * columns);
        const int xOff = (columns - inRow) * cw / 2;
        knobs[i]->setBounds (r.getX() + xOff + col * cw, r.getY() + row * rh, cw, rh);
    }
}

void ModulePanel::paint (Graphics& g)
{
    auto& lf = dynamic_cast<PenumbraLookAndFeel&> (getLookAndFeel());
    const auto& t = lf.theme();
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = power.getToggleState();

    if (t.style == 1)   // console: brushed steel panel with screws
    {
        g.setGradientFill (ColourGradient (t.panel.brighter (0.06f), 0, b.getY(), t.panel.darker (0.08f), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (Colours::white.withAlpha (0.025f));
        for (float y = b.getY() + 2; y < b.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, b.getX() + 2, b.getRight() - 2);
        for (auto p : { b.getTopLeft().translated (7, 7), b.getTopRight().translated (-7, 7), b.getBottomLeft().translated (7, -7), b.getBottomRight().translated (-7, -7) })
        {
            g.setColour (Colour (0xff8a8d93)); g.fillEllipse (Rectangle<float> (6, 6).withCentre (p));
            g.setColour (Colour (0xff3a3c40)); g.drawLine (p.x - 2.5f, p.y, p.x + 2.5f, p.y, 1.0f);
        }
    }
    else
    {
        g.setColour (t.panel); g.fillRoundedRectangle (b, t.style == 0 ? 6.0f : 8.0f);
        if (t.style == 2)
        {
            g.setGradientFill (ColourGradient (accent.withAlpha (on ? 0.10f : 0.03f), b.getCentreX(), b.getY(), Colours::transparentBlack, b.getCentreX(), b.getY() + 90, false));
            g.fillRoundedRectangle (b, 8.0f);
        }
    }
    g.setColour (on ? accent.withAlpha (t.style == 0 ? 0.55f : 0.35f) : t.panelEdge);
    g.drawRoundedRectangle (b, t.style == 0 ? 6.0f : 8.0f, 1.0f);
    if (t.style == 0)   // spectral strip along the top of the panel
    {
        g.setColour (accent.withAlpha (on ? 1.0f : 0.35f));
        g.fillRoundedRectangle (b.getX() + 10, b.getY() + 1, b.getWidth() - 20, 2.0f, 1.0f);
    }

    auto header = getLocalBounds().reduced (8, 6).removeFromTop (24).withTrimmedLeft (28);
    g.setColour (on ? t.text : t.dim);
    g.setFont (FontOptions (14.0f, Font::bold));
    g.drawText (title.toUpperCase(), header, Justification::centredLeft, false);

    paintIndicator (g, indicatorArea().toFloat());
}

void ModulePanel::paintIndicator (Graphics& g, Rectangle<float> r)
{
    auto& lf = dynamic_cast<PenumbraLookAndFeel&> (getLookAndFeel());
    const auto& t = lf.theme();
    const auto& e = proc.getEngine();
    const float act = e.getModuleActivity (module);
    if (act < 0.01f) return;
    const Colour col = accent.withAlpha (0.35f + 0.65f * act);
    g.setColour (col);
    switch (module)
    {
        case pn::kRotary:
        {
            // top view: drum (circle) and horn (bar), both spinning
            const auto c = r.getCentre().translated (14.0f, 0.0f);
            const float ha = MathConstants<float>::twoPi * e.getHornAngle(), da = MathConstants<float>::twoPi * e.getDrumAngle();
            g.drawEllipse (Rectangle<float> (18, 18).withCentre (c), 1.0f);
            g.drawLine (Line<float> (c.getPointOnCircumference (9.0f, da), c), 1.0f);
            g.drawLine (Line<float> (c.getPointOnCircumference (9.0f, ha), c.getPointOnCircumference (9.0f, ha + MathConstants<float>::pi)), 2.5f);
            break;
        }
        case pn::kMod: case pn::kSynthi: case pn::kMotion:
        {
            const float v = module == pn::kMod ? e.getModLfo() : (module == pn::kSynthi ? e.getSynLfo() : e.getPanPosition());
            g.setColour (t.panelEdge); g.drawLine (r.getX() + 6, r.getCentreY(), r.getRight() - 4, r.getCentreY(), 1.0f);
            g.setColour (col);
            g.fillEllipse (Rectangle<float> (7, 7).withCentre ({ jmap (v, -1.0f, 1.0f, r.getX() + 6, r.getRight() - 4), r.getCentreY() }));
            break;
        }
        case pn::kDelay:
            g.setFont (FontOptions (11.0f, Font::bold));
            g.drawText (String (roundToInt (e.getDelayMs())) + " ms", r, Justification::centredRight, false);
            break;
        default:
            g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ r.getRight() - 6, r.getCentreY() }));
            break;
    }
}

// ============================================================================ Editor
PenumbraEditor::PenumbraEditor (PenumbraProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (rack);

    auto add = [this] (int m, const char* title, const char* onId, int cols) -> ModulePanel&
    {
        panels.push_back (std::make_unique<ModulePanel> (proc, m, title, onId, cols));
        rack.addAndMakeVisible (*panels.back());
        return *panels.back();
    };

    auto& drv = add (pn::kDrive, "Drive", "drv_on", 3);
    drv.addChoice ("drv_type");
    drv.addKnob ("drv_drive", "Drive"); drv.addKnob ("drv_tone", "Tone"); drv.addKnob ("drv_level", "Level");

    auto& eq = add (pn::kEq, "EQ", "eq_on", 3);
    eq.addKnob ("eq_low", "Bass"); eq.addKnob ("eq_mid", "Mid"); eq.addKnob ("eq_midf", "Mid Freq");
    eq.addKnob ("eq_high", "Treble"); eq.addKnob ("eq_lowcut", "Low Cut");

    auto& syn = add (pn::kSynthi, "Synthi", "syn_on", 4);
    syn.addKnob ("syn_ring", "Ring Freq"); syn.addKnob ("syn_ringmix", "Ring Mix"); syn.addKnob ("syn_cut", "Cutoff"); syn.addKnob ("syn_res", "Resonance");
    syn.addKnob ("syn_lfo", "LFO Rate"); syn.addKnob ("syn_lfodepth", "LFO Depth"); syn.addKnob ("syn_env", "Envelope");

    auto& mod = add (pn::kMod, "Modulation", "mod_on", 2);
    mod.addChoice ("mod_type");
    mod.addKnob ("mod_rate", "Rate"); mod.addKnob ("mod_depth", "Depth"); mod.addKnob ("mod_fb", "Feedback"); mod.addKnob ("mod_mix", "Mix");

    auto& rot = add (pn::kRotary, "Rotary", "rot_on", 2);
    rot.addToggle ("rot_fast", "FAST");
    rot.addKnob ("rot_drive", "Drive"); rot.addKnob ("rot_spread", "Spread"); rot.addKnob ("rot_mix", "Mix");

    auto& dly = add (pn::kDelay, "Tape Delay", "dly_on", 4);
    dly.addChoice ("dly_mode"); dly.addChoice ("dly_heads"); dly.addChoice ("dly_div");
    dly.addToggle ("dly_sync", "SYNC"); dly.addToggle ("dly_freeze", "FREEZE");
    dly.addKnob ("dly_time", "Time"); dly.addKnob ("dly_fb", "Feedback"); dly.addKnob ("dly_mix", "Mix");
    dly.addKnob ("dly_tone", "Tone"); dly.addKnob ("dly_wow", "Wow/Flutter"); dly.addKnob ("dly_glide", "Glide");

    auto& rev = add (pn::kReverb, "Reverb", "rev_on", 2);
    rev.addChoice ("rev_type");
    rev.addKnob ("rev_decay", "Decay"); rev.addKnob ("rev_pre", "Pre-Delay"); rev.addKnob ("rev_damp", "Damping"); rev.addKnob ("rev_mix", "Mix");

    auto& mot = add (pn::kMotion, "Motion", "mot_on", 2);
    mot.addChoice ("mot_mode");
    mot.addKnob ("mot_rate", "Rate"); mot.addKnob ("mot_depth", "Depth"); mot.addKnob ("mot_shape", "Shape");

    inKnob = std::make_unique<Knob> (proc, "in_gain", "Input");
    outKnob = std::make_unique<Knob> (proc, "out_gain", "Output");
    inKnob->onHeader = outKnob->onHeader = true;
    rack.addAndMakeVisible (*inKnob);
    rack.addAndMakeVisible (*outKnob);

    rack.addAndMakeVisible (presetBox);
    rack.addAndMakeVisible (prevPreset);
    rack.addAndMakeVisible (nextPreset);
    rack.addAndMakeVisible (themeBox);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0 && id - 1 != proc.getCurrentProgram()) proc.setCurrentProgram (id - 1);
    };
    auto step = [this] (int dir)
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + dir + n) % n);
    };
    prevPreset.onClick = [step] { step (-1); };
    nextPreset.onClick = [step] { step (1); };

    for (int i = 0; i < numThemes(); ++i) themeBox.addItem (String ("Theme: ") + themeAt (i).name, i + 1);
    themeBox.onChange = [this] { applyTheme (themeBox.getSelectedId() - 1); };
    themeBox.setSelectedId (proc.getTheme() + 1, dontSendNotification);
    applyTheme (proc.getTheme());

    setResizable (true, true);
    setResizeLimits (kDesignW / 2, kDesignH / 2, kDesignW * 2, kDesignH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kDesignW / kDesignH);
    const float s = jlimit (0.5f, 2.0f, proc.getUiScale());
    setSize (roundToInt (kDesignW * s), roundToInt (kDesignH * s));
    startTimerHz (30);
}

PenumbraEditor::~PenumbraEditor()
{
    setLookAndFeel (nullptr);
}

void PenumbraEditor::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    int n = 0; auto* ps = pn::factoryPresets (n);
    String cat;
    for (int i = 0; i < n; ++i)
    {
        if (cat != ps[i].category) { cat = ps[i].category; presetBox.addSectionHeading (cat); }
        presetBox.addItem (ps[i].name, i + 1);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    lastProgram = proc.getCurrentProgram();
}

void PenumbraEditor::applyTheme (int index)
{
    index = jlimit (0, numThemes() - 1, index);
    proc.setTheme (index);
    lnf.setTheme (index);
    const auto& t = themeAt (index);
    for (auto& p : panels) p->setAccent (t.spectrumAccents ? kSpectrum[p->module] : t.accent);
    for (auto* k : { inKnob.get(), outKnob.get() })
    {
        k->accent = t.accent;
        k->slider.setColour (Slider::rotarySliderFillColourId, t.spectrumAccents ? Colours::white : t.accent);
    }
    // re-apply the look so child components pick up new colours
    rack.sendLookAndFeelChange();
    rack.repaint();
}

void PenumbraEditor::resized()
{
    const float s = (float) getWidth() / (float) kDesignW;
    proc.setUiScale (s);
    rack.setBounds (0, 0, kDesignW, kDesignH);
    rack.setTransform (AffineTransform::scale (s));
}

void PenumbraEditor::Rack::resized() { ed.layoutRack(); }
void PenumbraEditor::Rack::paint (Graphics& g) { ed.paintRack (g); }

void PenumbraEditor::layoutRack()
{
    const int margin = 14, gap = 10, top = 84, rowH = (kDesignH - top - margin - gap) / 2;
    // header controls
    presetBox.setBounds (330, 24, 300, 30);
    prevPreset.setBounds (636, 24, 30, 30);
    nextPreset.setBounds (670, 24, 30, 30);
    themeBox.setBounds (716, 24, 170, 30);
    inKnob->setBounds (kDesignW - 250, 2, 76, 76);
    outKnob->setBounds (kDesignW - 110, 2, 76, 76);

    auto layoutRow = [&] (int first, int count, int y)
    {
        int cols = 0;
        for (int i = first; i < first + count; ++i) cols += panels[(size_t) i]->columns;
        const int avail = kDesignW - 2 * margin - (count - 1) * gap;
        int x = margin;
        for (int i = first; i < first + count; ++i)
        {
            const int w = (i == first + count - 1) ? kDesignW - margin - x : avail * panels[(size_t) i]->columns / cols;
            panels[(size_t) i]->setBounds (x, y, w, rowH);
            x += w + gap;
        }
    };
    layoutRow (0, 4, top);
    layoutRow (4, 4, top + rowH + gap);
}

void PenumbraEditor::paintRack (Graphics& g)
{
    const auto& t = lnf.theme();
    auto bounds = Rectangle<float> (0, 0, (float) kDesignW, (float) kDesignH);

    // ---- background ----
    switch (t.style)
    {
        case 4:   // walnut cabinet
        {
            g.setGradientFill (ColourGradient (t.bg.brighter (0.15f), 0, 0, t.bg2, (float) kDesignW, (float) kDesignH, false));
            g.fillAll();
            Random rng (7);
            for (int i = 0; i < 140; ++i)
            {
                const float y = rng.nextFloat() * kDesignH;
                Path grain; grain.startNewSubPath (0, y);
                for (float x = 0; x <= kDesignW; x += 40) grain.lineTo (x, y + 3.0f * std::sin (x * 0.01f + i));
                g.setColour (Colours::black.withAlpha (0.04f + 0.05f * rng.nextFloat()));
                g.strokePath (grain, PathStrokeType (0.6f + rng.nextFloat()));
            }
            break;
        }
        case 2:   // eclipse: black sky, faint stars
        {
            g.fillAll (t.bg);
            Random rng (3);
            for (int i = 0; i < 160; ++i)
            {
                g.setColour (Colours::white.withAlpha (0.05f + 0.25f * rng.nextFloat()));
                g.fillEllipse (rng.nextFloat() * kDesignW, rng.nextFloat() * kDesignH, 1.2f, 1.2f);
            }
            break;
        }
        default:
            g.setGradientFill (ColourGradient (t.bg2, 0, 0, t.bg, 0, (float) kDesignH, false));
            g.fillAll();
            break;
    }

    // ---- header ----
    auto header = bounds.removeFromTop (74.0f).reduced (14.0f, 6.0f);
    const float lx = header.getX() + (t.style == 2 ? 56.0f : 0.0f);
    if (t.style == 2)
    {
        // eclipse emblem: dark disc with a corona
        const auto c = Point<float> (header.getX() + 24.0f, header.getCentreY());
        for (int i = 6; i >= 1; --i)
        {
            g.setColour (t.accent.withAlpha (0.05f * (7 - i) * 0.5f));
            g.fillEllipse (Rectangle<float> (40.0f + i * 4.0f, 40.0f + i * 4.0f).withCentre (c));
        }
        g.setColour (t.accent); g.drawEllipse (Rectangle<float> (40, 40).withCentre (c), 1.5f);
        g.setColour (t.bg); g.fillEllipse (Rectangle<float> (38, 38).withCentre (c));
    }
    const Colour headText = t.style == 4 ? t.panel : t.text, headDim = t.style == 4 ? t.panel.withAlpha (0.75f) : t.dim;
    g.setColour (headText);
    g.setFont (FontOptions (30.0f, Font::bold));
    g.drawText ("PENUMBRA", Rectangle<float> (lx, header.getY() + 4.0f, 250.0f, 34.0f), Justification::centredLeft, false);
    g.setColour (headDim);
    g.setFont (FontOptions (11.5f));
    g.drawText ("SPACE-ROCK STUDIO EFFECTS RACK", Rectangle<float> (lx + 2.0f, header.getY() + 38.0f, 260.0f, 16.0f), Justification::centredLeft, false);

    if (t.style == 0)   // thin spectral rule under the header
    {
        ColourGradient grad (kSpectrum[0], 14.0f, 0, kSpectrum[7], (float) kDesignW - 14.0f, 0, false);
        for (int i = 1; i < 7; ++i) grad.addColour (i / 7.0, kSpectrum[i]);
        g.setGradientFill (grad);
        g.fillRect (14.0f, 78.0f, (float) kDesignW - 28.0f, 2.0f);
    }
    else
    {
        g.setColour (t.style == 4 ? Colours::black.withAlpha (0.3f) : t.panelEdge);
        g.fillRect (14.0f, 78.0f, (float) kDesignW - 28.0f, 1.0f);
    }

    // header labels
    g.setColour (headDim);
    g.setFont (FontOptions (10.5f, Font::bold));
    g.drawText ("PRESET", 330, 8, 100, 14, Justification::centredLeft, false);
    g.drawText ("LOOK", 716, 8, 100, 14, Justification::centredLeft, false);

    // meters between the gain knobs
    auto meter = [&] (float x, float level)
    {
        const float h = 52.0f, y = 12.0f;
        g.setColour (t.light ? t.panelEdge : t.bg2.brighter (0.1f));
        g.fillRoundedRectangle (x, y, 8.0f, h, 2.0f);
        const float db = level > 1e-5f ? 20.0f * std::log10 (level) : -100.0f;
        const float frac = jlimit (0.0f, 1.0f, (db + 48.0f) / 54.0f);
        g.setColour (db > -0.5f ? Colour (0xffff4b4b) : (t.spectrumAccents ? kSpectrum[3] : t.accent));
        g.fillRoundedRectangle (x, y + h * (1.0f - frac), 8.0f, h * frac, 2.0f);
    };
    meter ((float) kDesignW - 164.0f, inMeter);
    meter ((float) kDesignW - 150.0f, outMeter);

}

void PenumbraEditor::timerCallback()
{
    const auto& e = proc.getEngine();
    const float ni = e.getInputPeak(), no = e.getOutputPeak();
    if (std::abs (ni - inMeter) > 1e-4f || std::abs (no - outMeter) > 1e-4f)
    {
        inMeter = ni; outMeter = no;
        rack.repaint (kDesignW - 170, 8, 30, 64);
    }
    for (auto& p : panels)
        if (p->module == pn::kRotary || p->module == pn::kMod || p->module == pn::kSynthi || p->module == pn::kMotion || p->module == pn::kDelay)
            p->repaint (p->indicatorArea());
    if (proc.getCurrentProgram() != lastProgram)
    {
        lastProgram = proc.getCurrentProgram();
        presetBox.setSelectedId (lastProgram + 1, dontSendNotification);
    }
}
