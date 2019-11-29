#pragma once
#include "componentlibrary.hpp"

using namespace std;

namespace rack {

  struct HoleKnob : RoundKnob {
    HoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/RampLibrary/HoleKnob.svg")));
    }
  };

}