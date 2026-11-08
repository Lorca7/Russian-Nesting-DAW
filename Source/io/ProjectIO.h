#pragma once
#include "../engine/Project.h"
#include <juce_core/juce_core.h>

namespace mf
{
std::unique_ptr<juce::XmlElement> projectToXml (const Project& p);
/** Reads attributes present in the XML on top of `p` (missing values keep their current value). */
void projectFromXml (const juce::XmlElement& xml, Project& p);
} // namespace mf
