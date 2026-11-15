#include "PluginEditor.h"

using namespace juce;

namespace
{
const char* kIds[12] { "vari", "pitch", "lpitch", "lfpitch", "hiss", "noise", "satmix", "delay", "dlyfb", "dlymix", "input", "output" };
const char* kLab[12] { "VARI", "PITCH", "L.PITCH", "LF PITCH", "HISS", "NOISE", "SAT MIX", "DELAY", "FB", "MIX", "INPUT", "OUTPUT" };
const int kX[12] { 158, 198, 236, 276, 314, 353, 391, 431, 466, 498, 1167, 1208 };
const int kY = 657;
const int kR[12] { 18, 18, 18, 18, 18, 18, 18, 18, 13, 13, 20, 20 };
const int kTbX[9] { 804, 846, 884, 922, 960, 999, 1037, 1075, 1113 };
const char* kTbLab[9] { "REV", "RWD", "F.FWD", "STOP", "PLAY", "REC", "LOOP", "TILT", "SYNC" };

// ajustes de los presets (13 valores): vari pitch lpitch lfpitch hiss noise satmix delay dlyfb dlymix input output speed
const char* kPresetIds[13] { "vari", "pitch", "lpitch", "lfpitch", "hiss", "noise", "satmix", "delay", "dlyfb", "dlymix", "input", "output", "speed" };
struct Preset { const char* name; float v[13]; };
const Preset kPresets[] = {
    { "Clean 15 ips",   { 0, 0, 0,  0,  0,  0, 30, 300, 35,  0, 0, 0, 1 } },
    { "Warm 15 ips",    { 0, 0, 0,  8,  8,  0, 55, 300, 35,  0, 2, 0, 1 } },
    { "Cassette 7.5",   { 0, 0, 0, 30, 30, 15, 70, 300, 35,  0, 3, 0, 0 } },
    { "Wobbly Reel",    { 0, 0, 0, 80, 10, 10, 50, 300, 35,  0, 0, 0, 0 } },
    { "Slow Motion",    { -35, 0, 0, 20, 8,  0, 50, 300, 35, 0, 0, 0, 1 } },
    { "Tape Echo",      { 0, 0, 0, 12,  6,  0, 60, 380, 55, 45, 0, 0, 1 } },
    { "Lo-Fi Dust",     { 0, 0, 0, 45, 45, 50, 85, 300, 35,  0, 4, 0, 0 } },
    { "Octave Down",    { 0, -12, 0, 6, 4,  0, 40, 300, 35,  0, 0, 0, 1 } }
};
}

//==============================================================================
ExtrasPanel::ExtrasPanel (TapermateAudioProcessor& p, int lang, std::function<void()> onClose) : proc (p)
{
    static const char* ids[4] { "reverb", "locut", "midcut", "hicut" };
    for (size_t i = 0; i < 4; ++i)
    {
        sliders[i].setSliderStyle (Slider::LinearHorizontal);
        sliders[i].setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (sliders[i]);
        atts[i] = std::make_unique<SliderAtt> (proc.apvts, ids[i], sliders[i]);
    }
    closeBtn.setButtonText (lang == 1 ? "CERRAR" : "CLOSE");
    closeBtn.onClick = std::move (onClose);
    addAndMakeVisible (closeBtn);
}

void ExtrasPanel::resized()
{
    for (int i = 0; i < 4; ++i) sliders[(size_t) i].setBounds (160, 70 + i * 40, 220, 24);
    closeBtn.setBounds (getWidth() - 110, getHeight() - 46, 90, 30);
}

void ExtrasPanel::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.f, 5.f), 14.f);
    g.setGradientFill (ColourGradient (Colour (0xff2c2c2e), 0.f, 0.f, Colour (0xff161617), 0.f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 14.f);
    g.setColour (Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (r.reduced (1.f), 14.f, 1.2f);
    g.setColour (Colours::white);
    g.setFont (Font (FontOptions (18.f)).boldened());
    g.drawText ("EXTRAS", 20, 14, 200, 28, Justification::centredLeft);
    static const char* names[4] { "REVERB", "LOW CUT", "MID CUT", "HIGH CUT" };
    static const char* ids[4] { "reverb", "locut", "midcut", "hicut" };
    g.setFont (Font (FontOptions (11.f)).boldened());
    for (int i = 0; i < 4; ++i)
    {
        g.setColour (Colour (0xffd2d2ce));
        g.drawText (names[i], 20, 70 + i * 40, 130, 24, Justification::centredLeft);
        const float v = proc.apvts.getRawParameterValue (ids[i])->load();
        g.setColour (Colour (0xffe8a33a));
        const String t = i == 0 ? String ((int) std::round (v)) + " %" : (i == 2 ? String (v, 1) + " dB" : String ((int) std::round (v)) + " Hz");
        g.drawText (t, 390, 70 + i * 40, 70, 24, Justification::centredLeft);
    }
}

//==============================================================================
TapeContent::TapeContent (TapermateAudioProcessor& p, Profile& pr, std::function<void (float)> sc)
    : proc (p), profile (pr), onScale (std::move (sc))
{
    setLookAndFeel (&laf);
    for (size_t i = 0; i < 12; ++i)
    {
        auto& k = knobs[i];
        k.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        k.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k);
        atts[i] = std::make_unique<SliderAtt> (proc.apvts, kIds[i], k);
        if (auto* ap = proc.apvts.getParameter (kIds[i]))
            k.setDoubleClickReturnValue (true, (double) ap->convertFrom0to1 (ap->getDefaultValue()));   // doble clic = valor por defecto
    }
    setSize (1530, 722);
    startTimerHz (30);
    if (! profile.done) startIntro (true);
}

TapeContent::~TapeContent()
{
    stopTimer();
    extras.reset();
    intro.reset();
    setLookAndFeel (nullptr);
}

void TapeContent::setParam (const String& id, float v)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

String TapeContent::knobText (int i) const
{
    const float v = getParam (kIds[i]);
    switch (i)
    {
        case 0:  return String (v >= 0.f ? "+" : "") + String (v, 1) + " %";
        case 1:  return String (v >= 0.f ? "+" : "") + String ((int) std::round (v)) + " st";
        case 2:  return String (v >= 0.f ? "+" : "") + String ((int) std::round (v)) + " ct";
        case 7:  return String ((int) std::round (v)) + " ms";
        case 10: case 11: return String (v, 1) + " dB";
        default: return String ((int) std::round (v)) + " %";
    }
}

void TapeContent::resized()
{
    for (int i = 0; i < 12; ++i) knobs[(size_t) i].setBounds (kX[i] - kR[i], kY - kR[i], 2 * kR[i], 2 * kR[i]);
    menuRect = { 10, 10, 54, 32 };
    presetRect = { 1340, 12, 170, 30 };
    nameRect = { 612, 440, 273, 46 };                       // placa con el nombre (clic: tamano de la ventana)
    powerRect = { 28, 638, 46, 46 };
    speedRect = { 82, 646, 52, 30 };
    resetRect = { 519, 627, 76, 57 };
    meterSwRect = { 1238, 636, 32, 46 };
    for (int i = 0; i < 9; ++i) tbRects[(size_t) i] = { kTbX[i] - 16, 655 - 16, 32, 32 };
    if (extras != nullptr) extras->setBounds (getWidth() / 2 - 240, 120, 480, 270);
    if (intro != nullptr) intro->setBounds (getLocalBounds());
}

//==============================================================================
// Menus, presets, intro
void TapeContent::showSizeMenu (Rectangle<int> target, bool full)
{
    PopupMenu m, sizes;
    static const int pct[] { 50, 60, 70, 80, 90, 100, 110, 125, 150, 175, 200, 250, 300, 350, 400 };
    const int cur = (int) std::round (profile.scale * 100.f);
    const int lang = profile.lang;
    if (full)
    {
        for (int p : pct) sizes.addItem (1000 + p, String (p) + "%", true, p == cur);
        m.addSubMenu (tr (S::MenuSize, lang), sizes);
        m.addItem (1, tr (S::MenuExtras, lang));
        m.addItem (2, tr (S::MenuIntro, lang));
        m.addItem (3, tr (S::MenuReset, lang));
    }
    else for (int p : pct) m.addItem (1000 + p, String (p) + "%", true, p == cur);

    m.showMenuAsync (PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (target)), [this] (int r)
    {
        if (r >= 1050 && r <= 1400) { if (onScale) onScale ((float) (r - 1000) / 100.f); }
        else if (r == 1)
        {
            if (extras != nullptr) { extras.reset(); return; }
            Component::SafePointer<TapeContent> sp (this);
            extras = std::make_unique<ExtrasPanel> (proc, profile.lang, [sp] { MessageManager::callAsync ([sp] { if (sp != nullptr) sp->extras.reset(); }); });
            addAndMakeVisible (*extras);
            extras->setBounds (getWidth() / 2 - 240, 120, 480, 270);
            extras->toFront (true);
        }
        else if (r == 2) startIntro (false);
        else if (r == 3) resetAll();
    });
}

void TapeContent::showPresets()
{
    PopupMenu m;
    for (int i = 0; i < (int) (sizeof (kPresets) / sizeof (kPresets[0])); ++i) m.addItem (100 + i, kPresets[i].name);
    m.showMenuAsync (PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (presetRect)), [this] (int r) { if (r >= 100) applyPreset (r - 100); });
}

void TapeContent::applyPreset (int idx)
{
    for (int i = 0; i < 13; ++i) setParam (kPresetIds[i], kPresets[idx].v[i]);
    message = kPresets[idx].name;
    messageUntil = Time::getMillisecondCounter() + 2500;
}

void TapeContent::resetAll()
{
    for (int i = 0; i < 13; ++i)
        if (auto* p = proc.apvts.getParameter (kPresetIds[i])) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); }
}

void TapeContent::startIntro (bool firstRun)
{
    Component::SafePointer<TapeContent> sp (this);
    intro = std::make_unique<IntroOverlay> (profile, firstRun, [sp] { if (sp != nullptr) sp->finishIntro(); });
    addAndMakeVisible (*intro);
    intro->setBounds (getLocalBounds());
    intro->toFront (true);
}

void TapeContent::finishIntro()
{
    intro.reset();
    message = fmt (tr (S::Welcome, profile.lang), profile.name.toUpperCase());
    messageUntil = Time::getMillisecondCounter() + 4000;
    repaint();
}

//==============================================================================
void TapeContent::timerCallback()
{
    const float motor = proc.machine.motorOut.load();
    angL -= motor * 0.10f;
    angR -= motor * 0.10f;
    angRoll += motor * 0.22f;
    const float rec = proc.machine.recSec.load(), ps = proc.machine.posSec.load();
    const float target = rec > 0.1f ? jlimit (0.f, 1.f, ps / rec) : 0.f;
    packFrac += (target - packFrac) * 0.2f;
    if (messageUntil != 0 && Time::getMillisecondCounter() > messageUntil) { message = {}; messageUntil = 0; }
    repaint();
}

void TapeContent::mouseMove (const MouseEvent& e)
{
    hoverKnob = -1;
    for (int i = 0; i < 12; ++i)
        if (e.position.getDistanceFrom (Point<float> ((float) kX[i], (float) kY)) <= (float) kR[i] + 4.f) hoverKnob = i;
}

void TapeContent::mouseDown (const MouseEvent& e)
{
    const auto pt = e.getPosition();
    if (menuRect.contains (pt))        { showSizeMenu (menuRect, true); return; }
    if (nameRect.contains (pt))        { showSizeMenu (nameRect, false); return; }
    if (presetRect.contains (pt))      { showPresets(); return; }
    if (powerRect.contains (pt))       { togglePar ("power"); return; }
    if (speedRect.contains (pt))       { setParam ("speed", getParam ("speed") > 0.5f ? 0.f : 1.f); return; }
    if (resetRect.contains (pt))       { resetAll(); return; }
    if (meterSwRect.contains (pt))     { togglePar ("metersrc"); return; }

    for (int i = 0; i < 9; ++i)
        if (tbRects[(size_t) i].contains (pt))
        {
            const int cur = proc.transport.load();
            auto go = [&] (int t) { proc.requestTransport (cur == t && t != tapermate_dsp::Stop && t != tapermate_dsp::Rec ? (int) tapermate_dsp::Stop : t); };
            switch (i)
            {
                case 0: go (tapermate_dsp::Rev);  break;
                case 1: go (tapermate_dsp::Rwd);  break;
                case 2: go (tapermate_dsp::FFwd); break;
                case 3: proc.requestTransport (tapermate_dsp::Stop); break;
                case 4: go (tapermate_dsp::Play); break;
                case 5: proc.requestTransport (tapermate_dsp::Rec); break;
                case 6: togglePar ("loop"); break;
                case 7: togglePar ("tilt"); break;
                default: togglePar ("sync"); break;
            }
            return;
        }
}

//==============================================================================
// Dibujo: maquina de cinta de carretes abiertos
void TapeContent::paintReels (Graphics& g)
{
    const Point<float> cL (313.f, 200.f), cR (1203.f, 200.f);
    const float RL = 168.f, RR = 160.f;
    const float packL = 46.f + (RL - 22.f - 46.f) * (0.12f + 0.88f * (1.f - packFrac));   // la cinta pasa de un carrete al otro
    const float packR = 46.f + (RR - 22.f - 46.f) * (0.12f + 0.88f * packFrac);

    auto reel = [&] (Point<float> c, float R, float pack, float ang)
    {
        auto P = [&] (float rr, float a) { return Point<float> (c.x + std::cos (a) * rr, c.y + std::sin (a) * rr); };
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - R, c.y - R + 8.f, 2.f * R, 2.f * R);
        g.setGradientFill (ColourGradient (Colour (0xfffdfdfc), c.x - R * 0.5f, c.y - R * 0.6f, Colour (0xffb2b2ae), c.x + R * 0.7f, c.y + R * 0.8f, true));
        g.fillEllipse (c.x - R, c.y - R, 2.f * R, 2.f * R);
        g.setColour (Colour (0xff8a8a86));
        g.drawEllipse (c.x - R, c.y - R, 2.f * R, 2.f * R, 2.f);
        for (int i = 0; i < 3; ++i)                       // 3 ventanas por las que se ve la cinta enrollada
        {
            const float a = ang + (float) i * MathConstants<float>::twoPi / 3.f;
            Path w;
            w.addTriangle (P (R * 0.86f, a - 0.45f), P (R * 0.86f, a + 0.45f), P (R * 0.32f, a));
            w = w.createPathWithRoundedCorners (R * 0.08f);
            g.setColour (Colour (0xff0b0b0b));
            g.fillPath (w);
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (w);
            g.setGradientFill (ColourGradient (Colour (0xff75522f), c.x, c.y, Colour (0xff35240f), c.x + pack, c.y, true));
            g.fillEllipse (c.x - pack, c.y - pack, 2.f * pack, 2.f * pack);
        }
        g.setColour (Colour (0xff2a2a2b));                // cubo
        g.fillEllipse (c.x - R * 0.2f, c.y - R * 0.2f, R * 0.4f, R * 0.4f);
        g.setColour (Colour (0xff9c9c98));
        g.drawEllipse (c.x - R * 0.2f, c.y - R * 0.2f, R * 0.4f, R * 0.4f, 2.f);
        g.setColour (Colour (0xff0b0b0b));
        for (int i = 0; i < 3; ++i) { const auto q = P (R * 0.12f, ang + 0.5f + (float) i * 2.0944f); g.fillEllipse (q.x - 4.f, q.y - 4.f, 8.f, 8.f); }
        g.setColour (Colour (0xffd0d0cc));
        g.fillEllipse (c.x - 6.f, c.y - 6.f, 12.f, 12.f);
        for (int i = 0; i < 6; ++i) { const auto q = P (R * 0.25f, ang + (float) i * 1.0472f); g.setColour (Colour (0xff3a3a3a)); g.fillEllipse (q.x - 2.5f, q.y - 2.5f, 5.f, 5.f); }
    };
    reel (cL, RL, packL, angL);
    reel (cR, RR, packR, angR);

    // etiquetas de los carretes
    {
        Graphics::ScopedSaveState ss (g);
        const Point<float> q (cL.x + std::cos (2.45f) * RL * 0.9f, cL.y + std::sin (2.45f) * RL * 0.9f);
        g.addTransform (AffineTransform::rotation (2.45f + MathConstants<float>::halfPi, q.x, q.y));
        g.setColour (Colour (0xff202020));
        g.setFont (Font (FontOptions (11.f)).boldened());
        g.drawText ("TAPELAB   RE-1100", Rectangle<float> (q.x - 70.f, q.y - 7.f, 140.f, 14.f), Justification::centred);
    }
    {
        Graphics::ScopedSaveState ss (g);
        const Point<float> q (cR.x - RR * 0.58f, cR.y - RR * 0.6f);
        g.addTransform (AffineTransform::rotation (-0.2f, q.x, q.y));
        g.setColour (Colour (0xff0b0b0b));
        g.fillRoundedRectangle (q.x - 56.f, q.y - 10.f, 112.f, 20.f, 3.f);
        g.setColour (Colours::white);
        g.setFont (Font (FontOptions (11.f)).boldened());
        g.drawText ("MASTER  TAPE", Rectangle<float> (q.x - 56.f, q.y - 10.f, 112.f, 20.f), Justification::centred);
    }

    // camino de la cinta
    auto tapeLine = [&] (std::initializer_list<Point<float>> pts)
    {
        Path p; bool first = true;
        for (auto& q : pts) { if (first) { p.startNewSubPath (q); first = false; } else p.lineTo (q); }
        g.setColour (Colours::white.withAlpha (0.92f));
        g.strokePath (p.createPathWithRoundedCorners (14.f), PathStrokeType (2.2f));
    };
    tapeLine ({ { cL.x + std::cos (2.1f) * RL * 0.97f, cL.y + std::sin (2.1f) * RL * 0.97f }, { 240.f, 455.f }, { 244.f, 480.f }, { 272.f, 503.f },
                { 376.f, 492.f }, { 408.f, 466.f }, { 612.f, 486.f } });
    tapeLine ({ { 885.f, 486.f }, { 986.f, 466.f }, { 1018.f, 490.f }, { 1129.f, 501.f }, { 1160.f, 478.f },
                { cR.x + std::cos (0.95f) * RR * 0.97f, cR.y + std::sin (0.95f) * RR * 0.97f } });

    // rodillos guia
    for (const auto& c : { Point<float> (272.f, 471.f), Point<float> (376.f, 460.f), Point<float> (1018.f, 458.f), Point<float> (1129.f, 469.f) })
    {
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillEllipse (c.x - 32.f, c.y - 30.f, 64.f, 64.f);
        g.setGradientFill (ColourGradient (Colours::white, c.x - 14.f, c.y - 18.f, Colour (0xffb9b9b5), c.x + 20.f, c.y + 24.f, true));
        g.fillEllipse (c.x - 32.f, c.y - 32.f, 64.f, 64.f);
        g.setColour (Colour (0xff77777a));
        g.fillEllipse (c.x - 14.f, c.y - 14.f, 28.f, 28.f);
        g.setColour (Colour (0xff2b2b2c));
        for (int i = 0; i < 2; ++i) { const float a = angRoll + (float) i * MathConstants<float>::pi; g.fillEllipse (c.x + std::cos (a) * 8.f - 2.5f, c.y + std::sin (a) * 8.f - 2.5f, 5.f, 5.f); }
    }
}

void TapeContent::paintHeadBlock (Graphics& g)
{
    const Rectangle<float> r (612.f, 361.f, 273.f, 156.f);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0.f, 8.f), 18.f);
    g.setGradientFill (ColourGradient (Colour (0xfff4f4f2), r.getX(), r.getY(), Colour (0xffc2c2bf), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 18.f);
    g.setColour (Colour (0xff8a8a86));
    g.drawRoundedRectangle (r, 18.f, 1.5f);
    g.setColour (Colour (0xff4a4a4a));
    for (int i = 0; i < 6; ++i) g.fillEllipse (r.getX() + (i % 3 == 0 ? 12.f : (i % 3 == 1 ? r.getWidth() * 0.5f - 4.f : r.getWidth() - 20.f)), r.getY() + (i < 3 ? 12.f : r.getHeight() - 20.f), 8.f, 8.f);
    const Rectangle<float> disp (654.f, 446.f, 192.f, 33.f);
    g.setColour (Colour (0xff0b0b0c));
    g.fillRoundedRectangle (disp, 5.f);
    g.setColour (Colours::white);
    g.setFont (Font (FontOptions (21.f)).boldened());
    g.drawText ("TAPERMATE", disp, Justification::centred);
}

void TapeContent::paintPanel (Graphics& g)
{
    const Rectangle<float> pr (0.f, 576.f, (float) getWidth(), (float) getHeight() - 576.f);
    g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.4f), 0.f, 560.f, Colours::transparentBlack, 0.f, 576.f, false));
    g.fillRect (0.f, 560.f, (float) getWidth(), 16.f);
    g.setGradientFill (ColourGradient (Colour (0xffdddddb), 0.f, pr.getY(), Colour (0xffb5b5b2), 0.f, pr.getBottom(), false));
    g.fillRect (pr);
    for (int y = 578; y < getHeight(); y += 3) { g.setColour ((y / 3) % 2 ? Colours::white.withAlpha (0.07f) : Colours::black.withAlpha (0.03f)); g.drawHorizontalLine (y, 0.f, (float) getWidth()); }
    g.setColour (Colours::white.withAlpha (0.8f));
    g.drawHorizontalLine (576, 0.f, (float) getWidth());

    const Colour ink (0xff383838);
    g.setFont (Font (FontOptions (9.f)).boldened());
    g.setColour (ink);

    // POWER
    const bool on = getParam ("power") > 0.5f;
    g.drawText ("POWER", 28, 604, 46, 12, Justification::centred);
    g.setColour (on ? Colour (0xffff3b30) : Colour (0xff6a2a26));
    g.fillEllipse (46.f, 622.f, 10.f, 10.f);
    if (on) { g.setGradientFill (ColourGradient (Colour (0x66ff3b30), 51.f, 627.f, Colour (0x00ff3b30), 66.f, 627.f, true)); g.fillEllipse (36.f, 612.f, 30.f, 30.f); }
    const Rectangle<float> pb (33.f, 643.f, 36.f, 36.f);
    g.setGradientFill (ColourGradient (on ? Colour (0xfff2564a) : Colour (0xff9a3a32), pb.getX(), pb.getY(), on ? Colour (0xffc22d22) : Colour (0xff6e2620), pb.getX(), pb.getBottom(), false));
    g.fillRoundedRectangle (pb, 7.f);
    g.setColour (Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (pb.reduced (1.f), 7.f, 1.f);

    // SPEED
    g.setColour (ink);
    g.drawText ("SPEED", 82, 602, 52, 12, Justification::centred);
    g.drawText ("7.5", 82, 620, 26, 10, Justification::centred);
    g.drawText ("15", 108, 620, 26, 10, Justification::centred);
    const bool fast = getParam ("speed") > 0.5f;
    g.setColour (Colour (0xff1c1c1d));
    g.fillRoundedRectangle (92.f, 658.f, 34.f, 13.f, 6.f);
    g.setGradientFill (ColourGradient (Colours::white, 0.f, 658.f, Colour (0xff9c9c98), 0.f, 671.f, false));
    g.fillRoundedRectangle (fast ? 110.f : 93.f, 659.f, 15.f, 11.f, 5.f);

    // etiquetas de perillas
    g.setColour (ink);
    for (int i = 0; i < 12; ++i)
    {
        const bool io = i >= 10;
        g.drawText (kLab[i], kX[i] - 32, io ? 598 : kY - kR[i] - 14, 64, 11, Justification::centred);
        if (io)
        {
            g.setColour (Colour (0xff555555));
            g.drawText (String (getParam (kIds[i]), 1), kX[i] - 32, 618, 64, 11, Justification::centred);
            g.setColour (ink);
        }
    }

    // RESET
    const auto rr = resetRect.toFloat();
    g.setGradientFill (ColourGradient (Colour (0xfff7f7f5), rr.getX(), rr.getY(), Colour (0xffc9c9c6), rr.getX(), rr.getBottom(), false));
    g.fillRoundedRectangle (rr, 7.f);
    g.setColour (Colour (0xff8c8c88));
    g.drawRoundedRectangle (rr, 7.f, 1.f);
    g.setColour (ink);
    g.setFont (Font (FontOptions (11.f)).boldened());
    g.drawText ("RESET", resetRect, Justification::centred);

    // pantalla LCD: contador y estado
    const Rectangle<float> lcd (633.f, 636.f, 114.f, 44.f);
    g.setColour (Colour (0xff1b1c1e));
    g.fillRoundedRectangle (lcd, 5.f);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (lcd, 5.f, 1.5f);
    const float ps = proc.machine.posSec.load();
    const int st = proc.transport.load();
    static const char* names[6] { "STOP", "PLAY", "REC", "REV", "F.FWD", "RWD" };
    g.setColour (Colour (0xffeaeae6));
    g.setFont (Font (FontOptions (Font::getDefaultMonospacedFontName(), 17.f, Font::bold)));
    g.drawText (String::formatted ("%02d:%04.1f", (int) (ps / 60.f), std::fmod (ps, 60.f)), lcd.withHeight (26.f).translated (0.f, 3.f), Justification::centred);
    g.setFont (Font (FontOptions (10.f)).boldened());
    const bool armed = proc.recArmed.load();
    g.setColour (st == tapermate_dsp::Rec || (armed && (int) (Time::getMillisecondCounter() / 450) % 2) ? Colour (0xffff7a2f) : Colour (0xffb8b8b3));
    g.drawText (armed && st != tapermate_dsp::Rec ? "REC ARMED" : names[jlimit (0, 5, st)], lcd.withTrimmedTop (26.f), Justification::centred);
}

void TapeContent::paintTransport (Graphics& g)
{
    const int st = proc.transport.load();
    const bool armed = proc.recArmed.load();
    const bool lit[9] { st == tapermate_dsp::Rev, st == tapermate_dsp::Rwd, st == tapermate_dsp::FFwd, st == tapermate_dsp::Stop, st == tapermate_dsp::Play, st == tapermate_dsp::Rec || armed,
                        getParam ("loop") > 0.5f, getParam ("tilt") > 0.5f, getParam ("sync") > 0.5f };
    const Colour ink (0xff383838);
    for (int i = 0; i < 9; ++i)
    {
        const auto r = tbRects[(size_t) i].toFloat();
        g.setColour (ink);
        g.setFont (Font (FontOptions (9.f)).boldened());
        g.drawText (kTbLab[i], (int) r.getX() - 8, 623, 48, 11, Justification::centred);

        Colour top = Colour (0xfff7f7f5), bot = Colour (0xffd6d6d3);
        if (lit[i]) { top = i == 5 ? Colour (0xffffd9c2) : (i == 4 ? Colour (0xffd5f5d5) : Colour (0xffffeabf)); bot = top.darker (0.15f); }
        g.setGradientFill (ColourGradient (top, r.getX(), r.getY(), bot, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.f);
        g.setColour (lit[i] ? Colour (0xffe8892b) : Colour (0xff9a9a96));
        g.drawRoundedRectangle (r, 6.f, lit[i] ? 1.8f : 1.f);

        const auto c = r.getCentre();
        g.setColour (Colour (0xff2f2f30));
        auto tri = [&] (float cx, float dir, float s)    // triangulo apuntando a la derecha (dir=1) o izquierda (dir=-1)
        {
            Path p; p.addTriangle (cx - dir * s * 0.6f, c.y - s, cx - dir * s * 0.6f, c.y + s, cx + dir * s * 0.8f, c.y);
            g.fillPath (p);
        };
        switch (i)
        {
            case 0: tri (c.x, -1.f, 7.f); break;                                           // REV
            case 1: tri (c.x - 6.f, -1.f, 6.f); tri (c.x + 5.f, -1.f, 6.f); break;         // RWD
            case 2: tri (c.x - 5.f, 1.f, 6.f);  tri (c.x + 6.f, 1.f, 6.f);  break;         // F.FWD
            case 3: g.fillRect (c.x - 6.f, c.y - 6.f, 12.f, 12.f); break;                  // STOP
            case 4: tri (c.x, 1.f, 8.f); break;                                            // PLAY
            case 5: g.setColour (Colour (0xffee7a1f)); g.fillEllipse (c.x - 7.f, c.y - 7.f, 14.f, 14.f); break;   // REC
            case 6: case 8:                                                                // LOOP / SYNC: flechas circulares
            {
                Path a;
                a.addCentredArc (c.x, c.y, 8.f, 8.f, 0.f, 0.5f, i == 6 ? 5.6f : 5.0f, true);
                g.strokePath (a, PathStrokeType (2.f));
                const float ea = i == 6 ? 5.6f : 5.0f;
                Path h; const Point<float> e (c.x + std::sin (ea) * 8.f, c.y - std::cos (ea) * 8.f);
                h.addTriangle (e.x - 4.f, e.y - 3.f, e.x + 4.f, e.y - 3.f, e.x, e.y + 5.f);
                g.fillPath (h, AffineTransform::rotation (ea + 0.2f, e.x, e.y));
                if (i == 6) { Path b; b.addCentredArc (c.x, c.y, 8.f, 8.f, 0.f, 3.65f, 8.7f, true); g.strokePath (b, PathStrokeType (2.f)); }
                break;
            }
            default:                                                                       // TILT: simbolo de tres sectores
                g.drawEllipse (c.x - 8.f, c.y - 8.f, 16.f, 16.f, 1.6f);
                for (int k = 0; k < 3; ++k)
                {
                    Path w; w.addPieSegment (c.x - 7.f, c.y - 7.f, 14.f, 14.f, (float) k * 2.0944f + 0.2f, (float) k * 2.0944f + 1.1f, 0.25f);
                    g.fillPath (w);
                }
                break;
        }
    }
}

void TapeContent::paintMeters (Graphics& g)
{
    const bool in = getParam ("metersrc") > 0.5f;
    // interruptor IN / OUT
    const Rectangle<float> sw (1246.f, 641.f, 16.f, 32.f);
    g.setColour (Colour (0xff383838));
    g.setFont (Font (FontOptions (9.f)).boldened());
    g.drawText ("IN", 1230, 624, 48, 11, Justification::centred);
    g.drawText ("OUT", 1230, 688, 48, 11, Justification::centred);
    g.setColour (Colour (0xff1c1c1d));
    g.fillRoundedRectangle (sw, 8.f);
    g.setGradientFill (ColourGradient (Colours::white, 0.f, 0.f, Colour (0xff9c9c98), 0.f, 12.f, false));
    g.fillRoundedRectangle (sw.getX() + 1.f, in ? sw.getY() + 2.f : sw.getBottom() - 16.f, 14.f, 14.f, 6.f);

    for (int c = 0; c < 2; ++c)
    {
        const Rectangle<float> r (c == 0 ? 1284.f : 1402.f, 629.f, 111.f, 59.f);
        g.setColour (Colour (0xff0a0a0b));
        g.fillRoundedRectangle (r, 6.f);
        const float lin = (in ? proc.machine.vuIn[c] : proc.machine.vuOut[c]).load();
        const float vu = Decibels::gainToDecibels (lin, -60.f) + 18.f;           // 0 VU = -18 dBFS RMS
        const Point<float> piv (r.getCentreX(), r.getBottom() + 22.f);
        const float rad = 58.f;
        for (int k = 0; k <= 10; ++k)
        {
            const float a = -0.62f + (float) k * 1.24f / 10.f;
            g.setColour (k >= 8 ? Colour (0xffd9453a) : Colour (0xffd9d9d4));
            g.drawLine (piv.x + std::sin (a) * rad, piv.y - std::cos (a) * rad, piv.x + std::sin (a) * (rad - (k % 5 == 0 ? 9.f : 5.f)), piv.y - std::cos (a) * (rad - (k % 5 == 0 ? 9.f : 5.f)), 1.f);
        }
        Path arc;
        arc.addCentredArc (piv.x, piv.y, rad, rad, 0.f, -0.62f, 0.62f, true);
        g.setColour (Colour (0xffd9d9d4).withAlpha (0.7f));
        g.strokePath (arc, PathStrokeType (1.f));
        const float a = jmap (jlimit (-20.f, 3.f, vu), -20.f, 3.f, -0.62f, 0.62f);
        g.setColour (Colour (0xffeeeeea));
        g.drawLine (piv.x, piv.y, piv.x + std::sin (a) * (rad + 2.f), piv.y - std::cos (a) * (rad + 2.f), 1.4f);
        g.setColour (Colour (0xffd9d9d4));
        g.setFont (Font (FontOptions (10.f)).boldened());
        g.drawText ("VU", r.withTrimmedTop (36.f), Justification::centred);
    }
}

void TapeContent::paint (Graphics& g)
{
    g.fillAll (Colour (0xff1b1c1d));
    for (int y = 0; y < 576; y += 4) { g.setColour (Colours::white.withAlpha (0.018f)); g.drawHorizontalLine (y, 0.f, (float) getWidth()); }

    paintReels (g);
    paintHeadBlock (g);
    paintPanel (g);
    paintTransport (g);
    paintMeters (g);

    // marca del creador (TapeLab) y menus
    drawTLLogo (g, Rectangle<float> (748.f, 12.f, 34.f, 21.f), Colours::white.withAlpha (0.85f));
    g.setColour (Colours::white.withAlpha (0.55f));
    g.setFont (Font (FontOptions (10.f)).boldened());
    g.drawText ("TAPELAB", 700, 36, 130, 12, Justification::centred);
    g.setColour (Colours::white.withAlpha (0.75f));
    for (int i = 0; i < 3; ++i) g.fillEllipse (22.f + (float) i * 10.f, 22.f, 5.f, 5.f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.fillRoundedRectangle (presetRect.toFloat(), 8.f);
    g.setColour (Colours::white.withAlpha (0.4f));
    g.drawRoundedRectangle (presetRect.toFloat(), 8.f, 1.f);
    g.setColour (Colours::white);
    g.setFont (Font (FontOptions (12.f)).boldened());
    g.drawText ("PRESETS", presetRect.withTrimmedRight (26), Justification::centred);
    Path chev;
    chev.addTriangle ((float) presetRect.getRight() - 24.f, (float) presetRect.getCentreY() - 3.f, (float) presetRect.getRight() - 12.f, (float) presetRect.getCentreY() - 3.f,
                      (float) presetRect.getRight() - 18.f, (float) presetRect.getCentreY() + 4.f);
    g.fillPath (chev);

    if (message.isNotEmpty())
    {
        g.setColour (Colours::white.withAlpha (0.85f));
        g.setFont (Font (FontOptions (16.f)).boldened());
        g.drawText (message, 0, 70, getWidth(), 24, Justification::centred);
    }
    if (hoverKnob >= 0 && hoverKnob < 10)
    {
        const auto r = Rectangle<float> ((float) kX[hoverKnob] - 40.f, (float) kY - 62.f, 80.f, 18.f);
        g.setColour (Colours::black.withAlpha (0.85f));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (Colour (0xffe8a33a));
        g.setFont (Font (FontOptions (11.f)).boldened());
        g.drawText (knobText (hoverKnob), r, Justification::centred);
    }
}

//==============================================================================
TapermateAudioProcessorEditor::TapermateAudioProcessorEditor (TapermateAudioProcessor& p)
    : AudioProcessorEditor (&p), content (p, profile, [this] (float s) { applyScale (s); })
{
    addAndMakeVisible (content);
    content.setBounds (0, 0, kBaseW, kBaseH);
    applyScale (profile.scale);
}

void TapermateAudioProcessorEditor::applyScale (float s)
{
    scale = jlimit (0.5f, 4.f, s);
    content.setTransform (AffineTransform::scale (scale));
    setSize (roundToInt ((float) kBaseW * scale), roundToInt ((float) kBaseH * scale));
    profile.scale = scale;
    profile.save();
}
