#pragma once
#include <JuceHeader.h>

// Perfil del usuario (global, una vez por instalacion): %APPDATA%\TapeLab\TAPERMATE\profile.xml
struct Profile
{
    bool done = false;          // false = primera vez -> pantalla de inicio
    juce::String name;
    int lang = 0;               // 0 ingles, 1 espanol
    float scale = 0.8f;         // tamano de la ventana 0.5 .. 4.0 (50% .. 400%)

    Profile() { load(); }
    static juce::File file();
    void load();
    void save() const;
};

// Textos (ingles / espanol)
#define TM_STRINGS(X) \
 X(MenuSize,     "Window size", "Tamaño de ventana") \
 X(MenuExtras,   "Extras (reverb / EQ cuts)...", "Extras (reverb / cortes de EQ)...") \
 X(MenuIntro,    "Show intro / profile...", "Mostrar intro / perfil...") \
 X(MenuReset,    "Reset all controls", "Restablecer todos los controles") \
 X(IntroSub,     "TAPE MACHINE", "MÁQUINA DE CINTA") \
 X(IntroName,    "YOUR NAME", "TU NOMBRE") \
 X(IntroHint,    "type your name...", "escribe tu nombre...") \
 X(IntroLang,    "LANGUAGE", "IDIOMA") \
 X(IntroEnter,   "ENTER", "ENTRAR") \
 X(IntroSave,    "SAVE", "GUARDAR") \
 X(IntroWelcome, "WELCOME, {0}", "BIENVENIDO, {0}") \
 X(IntroBy,      "A TAPELAB PLUGIN", "UN PLUGIN DE TAPELAB") \
 X(IntroDefault, "Producer", "Productor") \
 X(Welcome,      "WELCOME {0}", "BIENVENIDO {0}")

enum class S
{
#define X(id, en, es) id,
    TM_STRINGS(X)
#undef X
    Count
};

inline juce::String tr (S id, int lang)
{
    static const char* t[][2] = {
#define X(id, en, es) { en, es },
        TM_STRINGS(X)
#undef X
    };
    return juce::String (juce::CharPointer_UTF8 (t[(int) id][lang != 0 ? 1 : 0]));
}

inline juce::String fmt (juce::String t, const juce::String& a = {})
{
    return t.replace ("{0}", a);
}
