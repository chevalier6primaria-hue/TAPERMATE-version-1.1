#include "Profile.h"

juce::File Profile::file()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("TapeLab").getChildFile ("TAPERMATE").getChildFile ("profile.xml");
}

void Profile::load()
{
    lang = juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("es") ? 1 : 0;
    const auto f = file();
    if (! f.existsAsFile()) return;
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName ("TAPERMATE_PROFILE"))
        {
            done  = xml->getBoolAttribute ("done", false);
            name  = xml->getStringAttribute ("name");
            lang  = juce::jlimit (0, 1, xml->getIntAttribute ("lang", lang));
            scale = juce::jlimit (0.5f, 4.f, (float) xml->getDoubleAttribute ("scale", 0.8));
        }
}

void Profile::save() const
{
    juce::XmlElement xml ("TAPERMATE_PROFILE");
    xml.setAttribute ("done", done);
    xml.setAttribute ("name", name);
    xml.setAttribute ("lang", lang);
    xml.setAttribute ("scale", (double) scale);
    const auto f = file();
    f.getParentDirectory().createDirectory();
    xml.writeTo (f);
}
