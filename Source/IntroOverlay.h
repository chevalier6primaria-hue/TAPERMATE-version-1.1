#pragma once
#include <JuceHeader.h>
#include "Profile.h"

// Logo "TL" de TapeLab: dos letras de bloques (se dibuja en vectores, no hace falta ningun archivo de imagen)
inline void drawTLLogo (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour c)
{
    const float s = juce::jmin (area.getWidth() / 410.f, area.getHeight() / 254.f);
    const float w = 410.f * s, h = 254.f * s;
    const float x = area.getCentreX() - w * 0.5f, y = area.getCentreY() - h * 0.5f;
    g.setColour (c);
    g.fillRect (x,                 y,                 210.f * s, 45.f * s);     // barra de la T
    g.fillRect (x + 80.f * s,      y + 45.f * s,      50.f * s,  209.f * s);    // tronco de la T
    g.fillRect (x + 241.f * s,     y,                 50.f * s,  254.f * s);    // tronco de la L
    g.fillRect (x + 241.f * s,     y + 209.f * s,     169.f * s, 45.f * s);     // pie de la L
}

// Pantalla de inicio: cintas dispersas sobre fondo negro que se mueven lento, como si se rebobinaran.
// Muestra el logo y el nombre del creador (TapeLab). Pide nombre e idioma (solo la primera vez).
class IntroOverlay : public juce::Component, private juce::Timer
{
public:
    IntroOverlay (Profile& p, bool firstRun, std::function<void()> onDone);
    ~IntroOverlay() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    struct Ribbon { float cx, cy, ax, ay, fx, fy, p1, p2, p3, thick, speed, len; };

    void timerCallback() override;
    void enter();
    juce::String T (S id) const { return tr (id, profile.lang); }

    Profile& profile;
    bool first;
    std::function<void()> done;
    juce::TextEditor nameEdit;
    juce::TextButton enterBtn;
    juce::Rectangle<int> card;
    std::array<juce::Rectangle<int>, 2> langRects;
    std::array<Ribbon, 18> rib;
    float t = 0.f, exitT = 0.f;
    bool exiting = false, finished = false;
};
