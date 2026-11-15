#include "IntroOverlay.h"

using namespace juce;

namespace
{
const Colour kDark  { 0xff2e1d0f };
const Colour kMid   { 0xff684322 };
const Colour kLight { 0xffc08f55 };
inline float smooth (float x) { x = jlimit (0.f, 1.f, x); return x * x * (3.f - 2.f * x); }
}

IntroOverlay::IntroOverlay (Profile& p, bool firstRun, std::function<void()> onDone)
    : profile (p), first (firstRun), done (std::move (onDone))
{
    setInterceptsMouseClicks (true, true);

    Random r (20240611);                       // cintas dispersas: posiciones y curvas fijas pero distintas entre si
    for (auto& b : rib)
    {
        b.cx = 0.15f + r.nextFloat() * 0.7f;  b.cy = 0.15f + r.nextFloat() * 0.7f;
        b.ax = 0.18f + r.nextFloat() * 0.34f; b.ay = 0.14f + r.nextFloat() * 0.3f;
        b.fx = 0.8f + r.nextFloat() * 1.6f;   b.fy = 0.8f + r.nextFloat() * 1.6f;
        b.p1 = r.nextFloat() * 6.28f; b.p2 = r.nextFloat() * 6.28f; b.p3 = r.nextFloat() * 6.28f;
        b.thick = 4.f + r.nextFloat() * 7.f;
        b.speed = (0.05f + r.nextFloat() * 0.08f) * (r.nextBool() ? 1.f : -1.f);
        b.len = 3.2f + r.nextFloat() * 3.f;
    }

    nameEdit.setFont (Font (FontOptions (16.f)));
    nameEdit.setText (profile.name, dontSendNotification);
    nameEdit.setTextToShowWhenEmpty (T (S::IntroHint), Colours::white.withAlpha (0.4f));
    nameEdit.setColour (TextEditor::backgroundColourId, Colours::white.withAlpha (0.08f));
    nameEdit.setColour (TextEditor::textColourId, Colours::white);
    nameEdit.setColour (TextEditor::outlineColourId, Colours::white.withAlpha (0.4f));
    nameEdit.setColour (TextEditor::focusedOutlineColourId, kLight);
    nameEdit.setInputRestrictions (24);
    nameEdit.onReturnKey = [this] { enter(); };
    addAndMakeVisible (nameEdit);

    enterBtn.setButtonText (T (first ? S::IntroEnter : S::IntroSave));
    enterBtn.setColour (TextButton::buttonColourId, kMid);
    enterBtn.setColour (TextButton::textColourOffId, Colours::white);
    enterBtn.onClick = [this] { enter(); };
    addAndMakeVisible (enterBtn);

    nameEdit.setVisible (! first);
    enterBtn.setVisible (! first);
    if (! first) t = 3.2f;
    startTimerHz (30);
}

void IntroOverlay::resized()
{
    card = getLocalBounds().withSizeKeepingCentre (460, 200).translated (0, getHeight() / 5 + 40);
    nameEdit.setBounds (card.getX() + 30, card.getY() + 34, card.getWidth() - 60, 34);
    langRects[0] = { card.getX() + 30,  card.getY() + 100, 130, 32 };
    langRects[1] = { card.getX() + 170, card.getY() + 100, 130, 32 };
    enterBtn.setBounds (card.getCentreX() - 90, card.getBottom() - 52, 180, 38);
}

void IntroOverlay::enter()
{
    if (exiting) return;
    profile.name = nameEdit.getText().trim();
    if (profile.name.isEmpty()) profile.name = T (S::IntroDefault);
    profile.done = true;
    profile.save();
    exiting = true;
    exitT = 0.f;
    nameEdit.setVisible (false);
    enterBtn.setVisible (false);
}

void IntroOverlay::mouseDown (const MouseEvent& e)
{
    if (exiting || (first && t < 2.9f)) return;
    for (int i = 0; i < 2; ++i)
        if (langRects[(size_t) i].contains (e.getPosition()))
        {
            profile.lang = i;
            nameEdit.setTextToShowWhenEmpty (T (S::IntroHint), Colours::white.withAlpha (0.4f));
            enterBtn.setButtonText (T (first ? S::IntroEnter : S::IntroSave));
        }
}

void IntroOverlay::timerCallback()
{
    t += 1.f / 30.f;
    if (first && t > 2.9f && ! nameEdit.isVisible() && ! exiting)
    {
        nameEdit.setVisible (true);
        enterBtn.setVisible (true);
        nameEdit.grabKeyboardFocus();
    }
    if (exiting)
    {
        exitT += 1.f / 30.f / (first ? 2.0f : 1.1f);
        if (exitT >= 1.f && ! finished)
        {
            finished = true;
            stopTimer();
            auto cb = done;
            MessageManager::callAsync ([cb] { if (cb) cb(); });
        }
    }
    repaint();
}

void IntroOverlay::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float W = b.getWidth(), H = b.getHeight();

    Graphics::ScopedSaveState ss (g);
    if (exiting)                                    // iris circular: se ve el plugin por dentro
    {
        const float e = smooth ((exitT - 0.35f) / 0.65f);
        const float rad = e * std::sqrt (W * W + H * H) * 0.62f;
        Path p;
        p.setUsingNonZeroWinding (false);
        p.addRectangle (b);
        p.addEllipse (c.x - rad, c.y - rad, 2.f * rad, 2.f * rad);
        g.reduceClipRegion (p);
    }

    g.fillAll (Colour (0xff030303));
    g.setGradientFill (ColourGradient (Colour (0xff17120d), c.x, c.y * 0.9f, Colour (0xff030303), 0.f, 0.f, true));
    g.fillAll();

    // ---- cintas: se deslizan despacio por sus curvas (rebobinado) y, al entrar, se enrollan hacia el centro ----
    const float collapse = exiting ? 1.f - 0.93f * smooth (exitT / 0.85f) : 1.f;
    const float spin = exiting ? 1.f + 7.f * smooth (exitT) : 1.f;
    for (const auto& r : rib)
    {
        Path p;
        const float u0 = t * r.speed * spin * 6.28f * 0.35f;
        const int N = 90;
        for (int k = 0; k <= N; ++k)
        {
            const float u = u0 + (float) k / (float) N * r.len;
            float x = r.cx * W + r.ax * W * (std::sin (r.fx * u + r.p1) + 0.35f * std::sin (2.3f * r.fx * u + r.p2));
            float y = r.cy * H + r.ay * H * (std::sin (r.fy * u + r.p3) + 0.35f * std::cos (1.7f * r.fy * u + r.p1));
            x = c.x + (x - c.x) * collapse; y = c.y + (y - c.y) * collapse;
            if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        const PathStrokeType wide (r.thick, PathStrokeType::curved, PathStrokeType::rounded);
        g.setColour (kDark.withAlpha (0.9f));
        g.strokePath (p, wide);
        g.setColour (kMid.withAlpha (0.85f));
        g.strokePath (p, PathStrokeType (r.thick * 0.62f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (kLight.withAlpha (0.28f));
        g.strokePath (p, PathStrokeType (jmax (1.f, r.thick * 0.14f), PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation (0.f, -r.thick * 0.18f));
    }
    g.setGradientFill (ColourGradient (Colours::transparentBlack, c.x, c.y, Colour (0xcc000000), c.x + W * 0.62f, c.y, true));   // viñeta
    g.fillRect (b);

    // ---- logo TL + nombre del creador ----
    const float a = smooth ((t - 0.6f) / 1.4f) * (exiting ? 1.f - smooth (exitT / 0.4f) : 1.f);
    const float ty = first ? H * 0.12f : H * 0.08f;
    drawTLLogo (g, Rectangle<float> (c.x - 70.f, ty, 140.f, 87.f), Colours::white.withAlpha (a));
    g.setColour (Colours::white.withAlpha (a));
    g.setFont (Font (FontOptions (26.f)).boldened());
    {
        const String s = "TAPELAB";
        float x = c.x - 0.5f * (Font (FontOptions (26.f)).boldened().getStringWidthFloat (s) + 6.f * (float) (s.length() - 1));
        for (int i = 0; i < s.length(); ++i)
        {
            const String ch = s.substring (i, i + 1);
            g.drawText (ch, (int) x, (int) (ty + 98.f), 40, 32, Justification::centredLeft, false);
            x += Font (FontOptions (26.f)).boldened().getStringWidthFloat (ch) + 6.f;
        }
    }
    g.setFont (Font (FontOptions (13.f)));
    g.setColour (kLight.withAlpha (a));
    g.drawText ("TAPERMATE  v1.1   -   " + T (S::IntroSub), Rectangle<float> (0.f, ty + 138.f, W, 18.f), Justification::centred);

    // ---- formulario ----
    if (t >= 2.9f && ! exiting)
    {
        const float fa = smooth ((t - 2.9f) / 0.5f);
        const auto cr = card.toFloat().translated (0.f, (1.f - fa) * 20.f);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.12f * fa), cr.getX(), cr.getY(), Colours::white.withAlpha (0.04f * fa), cr.getX(), cr.getBottom(), false));
        g.fillRoundedRectangle (cr, 18.f);
        g.setColour (kLight.withAlpha (0.55f * fa));
        g.drawRoundedRectangle (cr, 18.f, 1.3f);
        g.setColour (kLight.withAlpha (fa));
        g.setFont (Font (FontOptions (11.f)).boldened());
        g.drawText (T (S::IntroName), card.getX() + 30, card.getY() + 14, 300, 16, Justification::centredLeft);
        g.drawText (T (S::IntroLang), card.getX() + 30, card.getY() + 80, 300, 16, Justification::centredLeft);
        static const char* langs[2] { "ENGLISH", "ESPAÑOL" };
        for (int i = 0; i < 2; ++i)
        {
            const auto r = langRects[(size_t) i].toFloat();
            const bool sel = (profile.lang == i);
            g.setColour ((sel ? kMid : Colours::white).withAlpha ((sel ? 0.8f : 0.08f) * fa));
            g.fillRoundedRectangle (r, 16.f);
            g.setColour (Colours::white.withAlpha ((sel ? 0.9f : 0.35f) * fa));
            g.drawRoundedRectangle (r, 16.f, 1.f);
            g.setFont (Font (FontOptions (12.f)).boldened());
            g.drawText (String (CharPointer_UTF8 (langs[i])), r, Justification::centred);
        }
    }

    if (exiting && exitT < 0.75f)                    // saludo al entrar
    {
        const float sa = std::sin (jlimit (0.f, 1.f, exitT / 0.75f) * MathConstants<float>::pi);
        g.setColour (Colours::white.withAlpha (sa));
        g.setFont (Font (FontOptions (30.f)).boldened());
        g.drawText (fmt (T (S::IntroWelcome), profile.name.toUpperCase()), Rectangle<float> (0.f, c.y - 20.f, W, 40.f), Justification::centred);
    }
}
