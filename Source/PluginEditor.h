#pragma once
#include "PluginProcessor.h"
#include "Profile.h"
#include "IntroOverlay.h"

// Perillas de aluminio con marca negra, como en el panel de la maquina
struct TapeLAF : public juce::LookAndFeel_V4
{
    TapeLAF()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1c1d));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffc0392b));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float s, float e, juce::Slider&) override
    {
        const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float ang = s + pos * (e - s);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (c.x - r, c.y - r + 2.f, 2.f * r, 2.f * r);
        g.setColour (juce::Colour (0xff202021));
        g.fillEllipse (c.x - r, c.y - r, 2.f * r, 2.f * r);
        const float rc = r * 0.82f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f4f2), c.x - rc * 0.5f, c.y - rc * 0.6f, juce::Colour (0xff85857f), c.x + rc * 0.7f, c.y + rc * 0.8f, true));
        g.fillEllipse (c.x - rc, c.y - rc, 2.f * rc, 2.f * rc);
        g.setColour (juce::Colours::black.withAlpha (0.1f));
        for (int k = 1; k <= 4; ++k) { const float q = rc * (float) k / 5.f; g.drawEllipse (c.x - q, c.y - q, 2.f * q, 2.f * q, 0.8f); }
        juce::Path p;
        p.addRectangle (-1.3f, -rc * 0.96f, 2.6f, rc * 0.55f);
        g.setColour (juce::Colour (0xff181818));
        g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override
    {
        const float cy = (float) y + (float) h * 0.5f;
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle ((float) x, cy - 3.f, (float) w, 6.f, 3.f);
        g.setColour (juce::Colour (0xffc0392b));
        g.fillRoundedRectangle ((float) x, cy - 3.f, pos - (float) x, 6.f, 3.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f4f2), pos - 6.f, cy - 8.f, juce::Colour (0xff85857f), pos + 6.f, cy + 8.f, false));
        g.fillEllipse (pos - 8.f, cy - 8.f, 16.f, 16.f);
    }
};

// Panel de extras de la v1.0: reverb y cortes de EQ
class ExtrasPanel : public juce::Component
{
public:
    ExtrasPanel (TapermateAudioProcessor& p, int lang, std::function<void()> onClose);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    TapermateAudioProcessor& proc;
    std::array<juce::Slider, 4> sliders;
    std::array<std::unique_ptr<SliderAtt>, 4> atts;
    juce::TextButton closeBtn;
};

// Contenido de la interfaz (tamano base 1530 x 722, proporcion de la maquina original)
class TapeContent : public juce::Component, private juce::Timer
{
public:
    TapeContent (TapermateAudioProcessor&, Profile&, std::function<void (float)> onScaleChange);
    ~TapeContent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;
    void setParam (const juce::String& id, float v);
    float getParam (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void togglePar (const char* id) { setParam (id, getParam (id) > 0.5f ? 0.f : 1.f); }
    juce::String knobText (int i) const;
    void showSizeMenu (juce::Rectangle<int> target, bool full);
    void showPresets();
    void applyPreset (int idx);
    void resetAll();
    void startIntro (bool firstRun);
    void finishIntro();

    void paintReels (juce::Graphics&);
    void paintHeadBlock (juce::Graphics&);
    void paintPanel (juce::Graphics&);
    void paintTransport (juce::Graphics&);
    void paintMeters (juce::Graphics&);

    TapermateAudioProcessor& proc;
    Profile& profile;
    std::function<void (float)> onScale;
    TapeLAF laf;

    std::array<juce::Slider, 12> knobs;
    std::array<std::unique_ptr<SliderAtt>, 12> atts;
    std::unique_ptr<IntroOverlay> intro;
    std::unique_ptr<ExtrasPanel> extras;

    juce::Rectangle<int> menuRect, presetRect, nameRect, powerRect, speedRect, resetRect, meterSwRect;
    std::array<juce::Rectangle<int>, 9> tbRects;
    float angL = 0.f, angR = 0.f, angRoll = 0.f, packFrac = 0.f;
    int hoverKnob = -1;
    juce::String message;
    juce::uint32 messageUntil = 0;
};

// Ventana del plugin: contiene el contenido y lo escala (50% - 400%); el tamano se recuerda
class TapermateAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TapermateAudioProcessorEditor (TapermateAudioProcessor&);
    void resized() override {}

private:
    static constexpr int kBaseW = 1530, kBaseH = 722;
    void applyScale (float s);

    Profile profile;
    TapeContent content;
    float scale = 1.f;
};
