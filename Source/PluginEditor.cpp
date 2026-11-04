#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour cStutter  (0xff00e5ff);   // cyan
    const juce::Colour cReverse  (0xffff2bd6);   // magenta
    const juce::Colour cTape     (0xffff8a00);   // orange
    const juce::Colour cCrush    (0xffb6ff00);   // lime
    const juce::Colour cGate     (0xffff1744);   // red
    const juce::Colour cMaster   (0xffffffff);
    const juce::Colour cFeedback (0xffff4fa3);

    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    juce::Font mono (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }
}

//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xffcfcfcf));
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,  juce::Colour (0x55ffffff));
    setColour (juce::Label::textColourId,               juce::Colour (0xffcfcfcf));
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colours::black);
}

juce::Font NeonLookAndFeel::getLabelFont (juce::Label&) { return mono (12.0f); }

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float start, float end, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float radius = (float) juce::jmin (w, h) * 0.5f - 6.0f;
    if (radius < 4.0f) return;
    const float cx = (float) x + (float) w * 0.5f;
    const float cy = (float) y + (float) h * 0.5f;
    const float angle = start + pos * (end - start);
    const juce::PathStrokeType thin (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (cx, cy, radius, radius, 0.0f, start, end, true);
    g.setColour (juce::Colour (0xff1e1e1e));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (cx, cy, radius, radius, 0.0f, start, angle, true);
    g.setColour (accent.withAlpha (0.22f));
    g.strokePath (value, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (value, thin);

    const float body = radius * 0.68f;
    g.setColour (juce::Colours::black);
    g.fillEllipse (cx - body, cy - body, body * 2.0f, body * 2.0f);
    g.setColour (juce::Colour (0xff2c2c2c));
    g.drawEllipse (cx - body, cy - body, body * 2.0f, body * 2.0f, 1.0f);

    const float sx = std::sin (angle), sy = -std::cos (angle);
    g.setColour (juce::Colours::white);
    g.drawLine (cx + sx * body * 0.25f, cy + sy * body * 0.25f,
                cx + sx * body * 0.9f,  cy + sy * body * 0.9f, 2.0f);
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                        bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();

    if (on)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (1.0f), 5.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (r, 4.0f);
    }
    else
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (accent.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }
    if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (r, 4.0f);
    }

    auto led = r.removeFromRight (r.getHeight()).reduced (8.0f);
    g.setColour (on ? juce::Colours::black : accent.withAlpha (0.35f));
    g.fillEllipse (led);

    g.setColour (on ? juce::Colours::black : accent);
    g.setFont (mono (13.0f, true));
    g.drawText (b.getButtonText(), r.reduced (8.0f, 0.0f), juce::Justification::centredLeft, false);
}

//==============================================================================
FukdupAudioProcessorEditor::FukdupAudioProcessorEditor (FukdupAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    const char* names[] = { "STUTTER", "REVERSE", "TAPE STOP", "CRUSH", "GATE" };
    const char* ids[]   = { "stutterOn", "reverseOn", "tapeOn", "crushOn", "gateOn" };
    const juce::Colour cols[] = { cStutter, cReverse, cTape, cCrush, cGate };

    for (size_t i = 0; i < modules.size(); ++i)
    {
        auto& m = modules[i];
        m.name = names[i];
        m.colour = cols[i];
        m.toggle.setButtonText (names[i]);
        m.toggle.setColour (juce::ToggleButton::tickColourId, cols[i]);
        addAndMakeVisible (m.toggle);
        m.attachment = std::make_unique<ButtonAttachment> (proc.apvts, ids[i], m.toggle);
    }

    modules[0].knobs = { &makeKnob ("stutterDiv", "DIV", cStutter),
                         &makeKnob ("stutterRepeats", "REPEATS", cStutter) };
    modules[1].knobs = { &makeKnob ("reverseSize", "SIZE", cReverse) };
    modules[2].knobs = { &makeKnob ("tapeTime", "TIME", cTape) };
    modules[3].knobs = { &makeKnob ("crushBits", "BITS", cCrush),
                         &makeKnob ("crushDown", "RATE DIV", cCrush) };
    modules[4].knobs = { &makeKnob ("gateDiv", "DIV", cGate),
                         &makeKnob ("gateDensity", "DENSITY", cGate),
                         &makeKnob ("gateSmooth", "SMOOTH", cGate) };

    mixKnob = &makeKnob ("mix", "MIX", cMaster);
    fbKnob  = &makeKnob ("feedback", "FEEDBACK", cFeedback);
    outKnob = &makeKnob ("output", "OUTPUT", cMaster);

    setSize (940, 680);
    startTimerHz (30);
}

FukdupAudioProcessorEditor::~FukdupAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

FukdupAudioProcessorEditor::Knob& FukdupAudioProcessorEditor::makeKnob (const juce::String& paramID,
                                                                        const juce::String& name,
                                                                        juce::Colour colour)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 16);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setColour (juce::Label::textColourId, colour.withAlpha (0.85f));
    addAndMakeVisible (k->slider);
    addAndMakeVisible (k->label);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void FukdupAudioProcessorEditor::layoutKnob (Knob& k, juce::Rectangle<int> cell)
{
    k.label.setBounds (cell.removeFromTop (14));
    k.slider.setBounds (cell);
}

float FukdupAudioProcessorEditor::param (const char* id) const
{
    if (auto* v = proc.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

//==============================================================================
void FukdupAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (14);

    logoArea = r.removeFromTop (60);
    r.removeFromTop (6);
    scopeArea = r.removeFromTop (150);
    r.removeFromTop (12);
    masterArea = r.removeFromBottom (96);
    r.removeFromBottom (12);

    const int gap = 10;
    const int mw = (r.getWidth() - gap * 4) / 5;

    for (size_t i = 0; i < modules.size(); ++i)
    {
        auto& m = modules[i];
        m.bounds = r.removeFromLeft (mw);
        if (i < modules.size() - 1)
            r.removeFromLeft (gap);

        auto inner = m.bounds.reduced (8);
        m.toggle.setBounds (inner.removeFromTop (26));
        inner.removeFromTop (6);
        m.visual = inner.removeFromTop (70);
        inner.removeFromTop (6);

        const int n = (int) m.knobs.size();
        const int kw = inner.getWidth() / 2;
        const int kh = 88;
        for (int k = 0; k < n; ++k)
        {
            const int col = k % 2, row = k / 2;
            int x = inner.getX() + col * kw;
            if (n % 2 == 1 && k == n - 1)
                x = inner.getX() + (inner.getWidth() - kw) / 2;   // centre a lone knob
            layoutKnob (*m.knobs[(size_t) k], { x, inner.getY() + row * kh, kw, kh });
        }
    }

    auto m = masterArea.reduced (10, 6);
    m.removeFromLeft (90);   // "MASTER" title space
    for (auto* k : { mixKnob, fbKnob, outKnob })
        layoutKnob (*k, m.removeFromLeft (100));
    m.removeFromLeft (24);
    meterArea = m.reduced (0, 24);
}

void FukdupAudioProcessorEditor::timerCallback()
{
    ++frame;
    tapeAngle += proc.visTapeSpeed.load() * 0.25f;
    if (tapeAngle > twoPi) tapeAngle -= twoPi;

    const int w = proc.scopeWrite.load (std::memory_order_relaxed);
    const int N = FukdupAudioProcessor::scopeSize;
    for (int i = 0; i < N; ++i)
        scopeData[(size_t) i] = proc.scope[(size_t) ((w + i) % N)].load (std::memory_order_relaxed);

    level = juce::jmax (proc.visLevel.exchange (0.0f), level * 0.85f);
    repaint();
}

//==============================================================================
void FukdupAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    bool anyOn = false;
    for (auto& m : modules)
        anyOn = anyOn || m.toggle.getToggleState();

    drawLogo (g, anyOn);
    drawScope (g, anyOn);
    for (int i = 0; i < (int) modules.size(); ++i)
        drawModule (g, i);
    drawMaster (g);
}

void FukdupAudioProcessorEditor::drawLogo (juce::Graphics& g, bool anyOn)
{
    const auto la = logoArea.toFloat();
    const auto ta = la.withWidth (300.0f);
    const juce::String word ("FUKDUP");
    const float j = anyOn ? 4.0f : 1.0f;
    auto off = [&] { return (rnd.nextFloat() * 2.0f - 1.0f) * j; };

    g.setFont (juce::Font (juce::FontOptions (52.0f, juce::Font::bold)));
    g.setColour (cGate.withAlpha (0.8f));
    g.drawText (word, ta.translated (-2.0f + off(), off() * 0.5f), juce::Justification::centredLeft, false);
    g.setColour (cStutter.withAlpha (0.8f));
    g.drawText (word, ta.translated (2.0f + off(), off() * 0.5f), juce::Justification::centredLeft, false);
    g.setColour (juce::Colours::white);
    g.drawText (word, ta, juce::Justification::centredLeft, false);

    // horizontal slice tearing when anything is engaged
    if (anyOn)
    {
        for (int s = 0; s < 3; ++s)
        {
            if (rnd.nextFloat() > 0.5f) continue;
            const float y = ta.getY() + rnd.nextFloat() * ta.getHeight();
            const float hh = 2.0f + rnd.nextFloat() * 8.0f;
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<int> ((int) ta.getX(), (int) y, (int) ta.getWidth(), (int) hh));
            g.setColour (juce::Colours::black);
            g.fillRect (ta);
            g.setColour (rnd.nextBool() ? juce::Colours::white : modules[(size_t) rnd.nextInt (5)].colour);
            g.drawText (word, ta.translated ((rnd.nextFloat() * 2.0f - 1.0f) * 18.0f, 0.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    g.setFont (mono (11.0f));
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawText ("glitch destruction unit", la.withTrimmedLeft (260.0f), juce::Justification::centredLeft, false);
    g.drawText (juce::String (proc.visBpm.load(), 1) + " bpm", la, juce::Justification::centredRight, false);
}

void FukdupAudioProcessorEditor::drawScope (juce::Graphics& g, bool anyOn)
{
    const auto r = scopeArea.toFloat();
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    auto inner = r.reduced (10.0f, 8.0f);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (int i = 1; i < 16; ++i)
        g.drawVerticalLine ((int) (inner.getX() + inner.getWidth() * (float) i / 16.0f), inner.getY(), inner.getBottom());
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (inner.getY() + inner.getHeight() * (float) i / 4.0f), inner.getX(), inner.getRight());

    juce::Path wave;
    const int N = FukdupAudioProcessor::scopeSize;
    for (int i = 0; i < N; ++i)
    {
        const float x = inner.getX() + inner.getWidth() * (float) i / (float) (N - 1);
        const float v = juce::jlimit (-1.0f, 1.0f, scopeData[(size_t) i]);
        const float y = inner.getCentreY() - v * inner.getHeight() * 0.46f;
        if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
    }

    if (anyOn)   // chromatic split
    {
        g.setColour (cGate.withAlpha (0.55f));
        g.strokePath (wave, juce::PathStrokeType (1.5f), juce::AffineTransform::translation (-3.0f + rnd.nextFloat() * 2.0f, 0.0f));
        g.setColour (cStutter.withAlpha (0.55f));
        g.strokePath (wave, juce::PathStrokeType (1.5f), juce::AffineTransform::translation (3.0f - rnd.nextFloat() * 2.0f, 0.0f));
    }
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.strokePath (wave, juce::PathStrokeType (5.0f));
    g.setColour (juce::Colours::white);
    g.strokePath (wave, juce::PathStrokeType (1.4f));

    // scanlines
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    for (float y = inner.getY(); y < inner.getBottom(); y += 3.0f)
        g.drawHorizontalLine ((int) y, inner.getX(), inner.getRight());

    // tags for active modules
    g.setFont (mono (11.0f, true));
    float tx = inner.getX() + 2.0f;
    for (auto& m : modules)
    {
        if (! m.toggle.getToggleState()) continue;
        const juce::String t = "[" + m.name + "]";
        const float tw = (float) t.length() * 7.6f;
        g.setColour (m.colour);
        g.drawText (t, juce::Rectangle<float> (tx, inner.getY(), tw, 14.0f), juce::Justification::centredLeft, false);
        tx += tw + 6.0f;
    }
}

void FukdupAudioProcessorEditor::drawModule (juce::Graphics& g, int index)
{
    auto& m = modules[(size_t) index];
    const bool on = m.toggle.getToggleState();
    const auto col = m.colour;
    const auto b = m.bounds.toFloat();

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (b, 8.0f);
    if (on)
    {
        g.setColour (col.withAlpha (0.12f));
        g.drawRoundedRectangle (b.expanded (2.0f), 10.0f, 5.0f);
    }
    g.setColour (col.withAlpha (on ? 0.9f : 0.22f));
    g.drawRoundedRectangle (b, 8.0f, 1.2f);

    const auto v = m.visual.toFloat();
    g.setColour (col.withAlpha (on ? 0.06f : 0.02f));
    g.fillRoundedRectangle (v, 4.0f);
    g.setColour (col.withAlpha (0.18f));
    g.drawRoundedRectangle (v, 4.0f, 1.0f);

    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (m.visual.reduced (1));
    const auto r = v.reduced (6.0f);
    const float alpha = on ? 1.0f : 0.3f;

    switch (index)
    {
        case 0: // STUTTER: repeated slices, current one lit
        {
            const int reps = juce::jmax (1, (int) param ("stutterRepeats"));
            const float prog = on ? proc.visStutterProg.load() : 0.0f;
            const int cur = juce::jlimit (0, reps - 1, (int) (prog * (float) reps));
            const float bw = r.getWidth() / (float) reps;

            for (int k = 0; k < reps; ++k)
            {
                auto cell = juce::Rectangle<float> (r.getX() + (float) k * bw, r.getY(), bw, r.getHeight()).reduced (1.0f, 0.0f);
                const float a = ! on ? 0.28f : (k == cur ? 1.0f : (k < cur ? 0.45f : 0.16f));
                g.setColour (col.withAlpha (a * 0.18f));
                g.fillRect (cell);

                juce::Path w;
                for (int s = 0; s <= 24; ++s)
                {
                    const float t = (float) s / 24.0f;
                    const float y = cell.getCentreY() - std::sin (t * twoPi * 3.0f) * (1.0f - t) * cell.getHeight() * 0.42f;
                    const float x = cell.getX() + t * cell.getWidth();
                    if (s == 0) w.startNewSubPath (x, y); else w.lineTo (x, y);
                }
                g.setColour (col.withAlpha (a));
                g.strokePath (w, juce::PathStrokeType (1.3f));
            }
            break;
        }

        case 1: // REVERSE: swelling backwards waveform + right-to-left playhead
        {
            const float size = param ("reverseSize");
            const float cycles = juce::jmap (size, 40.0f, 1000.0f, 14.0f, 4.0f);
            juce::Path w;
            for (int s = 0; s <= 120; ++s)
            {
                const float t = (float) s / 120.0f;
                const float y = r.getCentreY() - std::sin (t * twoPi * cycles) * t * t * r.getHeight() * 0.45f;
                const float x = r.getX() + t * r.getWidth();
                if (s == 0) w.startNewSubPath (x, y); else w.lineTo (x, y);
            }
            g.setColour (col.withAlpha (0.9f * alpha));
            g.strokePath (w, juce::PathStrokeType (1.3f));

            if (on)
            {
                const float px = r.getRight() - proc.visRevProg.load() * r.getWidth();
                g.setColour (col.withAlpha (0.2f));
                g.fillRect (juce::Rectangle<float> (px, r.getY(), r.getRight() - px, r.getHeight()));
                g.setColour (juce::Colours::white);
                g.drawVerticalLine ((int) px, r.getY(), r.getBottom());
            }
            g.setFont (mono (11.0f, true));
            g.setColour (col.withAlpha (alpha));
            g.drawText ("<<", r, juce::Justification::topLeft, false);
            break;
        }

        case 2: // TAPE STOP: spinning reels that slow down
        {
            const float sp = on ? proc.visTapeSpeed.load() : 1.0f;
            const float rad = juce::jmin (r.getHeight() * 0.40f, r.getWidth() * 0.19f);
            const juce::Point<float> c1 (r.getX() + r.getWidth() * 0.28f, r.getCentreY() - 3.0f);
            const juce::Point<float> c2 (r.getX() + r.getWidth() * 0.72f, r.getCentreY() - 3.0f);

            g.setColour (col.withAlpha (alpha));
            for (auto c : { c1, c2 })
            {
                g.drawEllipse (c.x - rad, c.y - rad, rad * 2.0f, rad * 2.0f, 2.0f);
                g.fillEllipse (c.x - rad * 0.18f, c.y - rad * 0.18f, rad * 0.36f, rad * 0.36f);
                for (int s = 0; s < 3; ++s)
                {
                    const float a = tapeAngle + (float) s * twoPi / 3.0f;
                    g.drawLine (c.x + std::cos (a) * rad * 0.25f, c.y + std::sin (a) * rad * 0.25f,
                                c.x + std::cos (a) * rad * 0.85f, c.y + std::sin (a) * rad * 0.85f, 1.5f);
                }
            }
            g.drawLine (c1.x, c1.y + rad, c2.x, c2.y + rad, 1.0f);

            g.setFont (mono (10.0f, true));
            g.setColour (juce::Colours::white.withAlpha (alpha * 0.8f));
            g.drawText (juce::String (juce::roundToInt (sp * 100.0f)) + "%", r, juce::Justification::bottomRight, false);
            break;
        }

        case 3: // CRUSH: stair-stepped wave driven by bits + rate
        {
            const float bits = on ? param ("crushBits") : 16.0f;
            const float down = on ? param ("crushDown") : 1.0f;
            const float levels = std::pow (2.0f, bits - 1.0f);
            const float hold = juce::jmap (down, 1.0f, 64.0f, 1.0f, 20.0f);

            juce::Path w;
            bool first = true;
            for (float x = 0.0f; x < r.getWidth(); x += hold)
            {
                const float t = x / r.getWidth();
                float val = std::sin (t * twoPi * 2.0f + (float) frame * 0.06f);
                val = std::round (val * levels) / levels;
                const float y = r.getCentreY() - val * r.getHeight() * 0.42f;
                if (first) { w.startNewSubPath (r.getX() + x, y); first = false; }
                else         w.lineTo (r.getX() + x, y);
                w.lineTo (r.getX() + juce::jmin (x + hold, r.getWidth()), y);
            }
            g.setColour (col.withAlpha (alpha));
            g.strokePath (w, juce::PathStrokeType (1.5f));

            g.setFont (mono (10.0f, true));
            g.setColour (juce::Colours::white.withAlpha (alpha * 0.8f));
            g.drawText (juce::String (param ("crushBits"), 1) + "b /" + juce::String ((int) param ("crushDown")),
                        r, juce::Justification::bottomRight, false);
            break;
        }

        case 4: // GATE: last 16 dice rolls + live gain bar
        {
            const auto bitsVal = proc.visGateBits.load();
            const float gain = on ? proc.visGateGain.load() : 1.0f;
            const auto grid = r.withTrimmedBottom (10.0f);
            const int steps = 16;
            const float cw = grid.getWidth() / (float) steps;

            for (int s = 0; s < steps; ++s)
            {
                const bool open = ((bitsVal >> (steps - 1 - s)) & 1u) != 0;
                const auto c = juce::Rectangle<float> (grid.getX() + (float) s * cw + 1.0f, grid.getY(), cw - 2.0f, grid.getHeight());
                if (! on)
                {
                    g.setColour (col.withAlpha (0.15f));
                    g.drawRect (c, 1.0f);
                }
                else if (open)
                {
                    g.setColour (col.withAlpha (s == steps - 1 ? 1.0f : 0.25f + 0.6f * (float) s / (float) steps));
                    g.fillRect (c);
                }
                else
                {
                    g.setColour (col.withAlpha (0.25f));
                    g.drawRect (c, 1.0f);
                }
            }
            g.setColour (col.withAlpha (on ? 0.9f : 0.2f));
            g.fillRect (r.getX(), r.getBottom() - 5.0f, r.getWidth() * gain, 5.0f);
            break;
        }

        default: break;
    }
}

void FukdupAudioProcessorEditor::drawMaster (juce::Graphics& g)
{
    const auto r = masterArea.toFloat();
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    g.setFont (mono (13.0f, true));
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.drawText ("MASTER", masterArea.withWidth (100).reduced (16, 0), juce::Justification::centredLeft, false);

    // segmented level meter
    const auto m = meterArea.toFloat();
    const int segs = 32;
    const float sw = m.getWidth() / (float) segs;
    const float lvlDb = juce::Decibels::gainToDecibels (level, -60.0f);
    const int lit = (int) (juce::jmap (lvlDb, -60.0f, 6.0f, 0.0f, 1.0f) * (float) segs);

    for (int s = 0; s < segs; ++s)
    {
        const auto c = s < (int) (segs * 0.7f) ? juce::Colours::white
                     : (s < (int) (segs * 0.9f) ? cTape : cGate);
        g.setColour (s < lit ? c : c.withAlpha (0.08f));
        g.fillRect (juce::Rectangle<float> (m.getX() + (float) s * sw + 1.0f, m.getY(), sw - 2.0f, m.getHeight()));
    }

    g.setFont (mono (11.0f));
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawText ("output level", juce::Rectangle<float> (m.getX(), m.getY() - 18.0f, m.getWidth(), 14.0f),
                juce::Justification::centredLeft, false);
    g.drawText (level > 0.001f ? juce::String (lvlDb, 1) + " dB" : juce::String ("-inf"),
                juce::Rectangle<float> (m.getX(), m.getY() - 18.0f, m.getWidth(), 14.0f),
                juce::Justification::centredRight, false);
}
